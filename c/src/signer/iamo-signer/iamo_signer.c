/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 * 
 * 
 * COMMERCIAL LICENSE USAGE
 * 
 * Licensees holding a valid commercial license may use this file in accordance 
 * with the commercial license agreement provided with the Software or, alternatively, 
 * in accordance with the terms contained in a written agreement between you and 
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
 * information please contact slock.it at in3@slock.it.
 * 	
 * Alternatively, this file may be used under the AGPL license as follows:
 *    
 * AGPL LICENSE USAGE
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

#include "iamo_signer.h"
#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/secp256k1.h"
#include "../../verifier/eth1/nano/serialize.h"
#include <string.h>
#include <time.h>

static void ethereum_address_checksum(const uint8_t* addr, char* address) {
  const char* hex = "0123456789abcdef";
  for (int i = 0; i < 20; i++) {
    address[i * 2]     = hex[(addr[i] >> 4) & 0xF];
    address[i * 2 + 1] = hex[addr[i] & 0xF];
  }
  address[40] = 0;

  SHA3_CTX ctx      = {0};
  uint8_t  hash[32] = {0};
  keccak_256_Init(&ctx);
  keccak_Update(&ctx, (const uint8_t*) address, 40);
  keccak_Final(&ctx, hash);

  for (int i = 0; i < 20; i++) {
    if (hash[i] & 0x80 && address[i * 2] >= 'a' && address[i * 2] <= 'f') address[i * 2] -= 0x20;
    if (hash[i] & 0x08 && address[i * 2 + 1] >= 'a' && address[i * 2 + 1] <= 'f') address[i * 2 + 1] -= 0x20;
  }
}

#define CNF_ERROR(msg)                  \
  {                                     \
    ctx->error_msg = _strdupn(msg, -1); \
    return IN3_EINVAL;                  \
  }
#define CNF_SET_BYTES(dst, token, property, l)                      \
  {                                                                 \
    const bytes_t tmp = d_to_bytes(d_get(token, key(property)));    \
    if (tmp.data) {                                                 \
      if (tmp.len != l) CNF_ERROR(property " must be " #l " bytes") \
      memcpy(dst, tmp.data, l);                                     \
    }                                                               \
  }

static in3_ret_t iamo_free(iamo_signer_config_t* conf) {
  _free(conf->services.account);
  _free(conf->services.sign);
  _free(conf->services.key);
  _free(conf->services.policy_management);
  _free(conf->accounts);
  _free(conf);
  return IN3_OK;
}

static in3_ret_t create_iso_timestamp(in3_req_t* ctx, time_t time, char* dst) {
  struct tm* ptm = gmtime(&time);
  if (!ptm) return req_set_error(ctx, "could not create the local time", IN3_EINVAL);
  sprintf(dst, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, 0);
  return IN3_OK;
}

static void set_string(char* prefix, char* postfix, char** dst) {
  if (*dst) _free(*dst);
  int a = strlen(prefix);
  int b = strlen(postfix);
  *dst  = _malloc(a + b + 1);
  memcpy(*dst, prefix, a);
  memcpy(*dst + a, postfix, b + 1);
}

static in3_ret_t iamo_config_set(iamo_signer_config_t* conf, in3_configure_ctx_t* ctx) {
  if (ctx->token->key == key("iamo")) {
    CNF_SET_BYTES(conf->device_key, ctx->token, "device_key", 32)
    char* service = d_get_stringk(ctx->token, key("service"));
    if (service && strlen(service)) {
      set_string("https://account.", service, &conf->services.account);
      set_string("https://key.", service, &conf->services.key);
      set_string("https://sign.", service, &conf->services.sign);
      set_string("https://policy-management.", service, &conf->services.policy_management);
      set_string("https://policy-processor.", service, &conf->services.policy_processor);
    }

    return IN3_OK;
  }
  return IN3_EIGNORE;
}

NONULL static in3_ret_t check_device_key(in3_req_t* ctx, iamo_signer_config_t* conf) {
  if (memiszero(conf->device_key, 32)) return req_set_error(ctx, "No device key set", IN3_ECONFIG);
  if (memiszero(conf->device_address, 20)) {
    uint8_t public_key[65];
    ecdsa_get_public_key65(&secp256k1, conf->device_key, public_key);
    keccak(bytes(public_key + 1, 64), public_key);
    memcpy(conf->device_address, public_key + 12, 20);
  }
  if (!conf->services.account) {
    in3_configure_ctx_t c = {.client = ctx->client, .json = parse_json("{\"iamo\":{\"service\":\"develop.iamo.dev\"}}"), .error_msg = NULL};
    c.token               = c.json->result + 1;
    in3_ret_t r           = iamo_config_set(conf, &c);
    json_free(c.json);
    if (c.error_msg) {
      req_set_error(ctx, c.error_msg, r);
      _free(c.error_msg);
    }
    if (r < 0) return r;
  }
  return IN3_OK;
}

static in3_req_t* find_ctx_for(in3_req_t* ctx, char* data) {
  for (; ctx; ctx = ctx->required) {
    // only check first entry
    for (cache_entry_t* e = ctx->cache; e; e = e->next) {
      if (e->props & CACHE_PROP_SRC_REQ && strcmp((char*) e->value.data, data) == 0) return ctx;
    }
  }
  return NULL;
}

static in3_ret_t send_api_request(in3_req_t* ctx, iamo_signer_config_t* conf, char* url, char* method, char* path, char* payload, d_token_t** result) {

  bytes32_t hash;
  uint8_t   signature[65];
  sb_t      rp = {0};
  char      timestamp[25];
  char*     rkey = NULL;

  // create proof message
  sb_add_chars(&rp, "{\"host\":\"");
  sb_add_escaped_chars(&rp, url + 8);
  sb_add_chars(&rp, "\",\"body\":\"");
  sb_add_escaped_chars(&rp, payload ? payload : "");
  sb_add_chars(&rp, "\",\"path\":\"");
  sb_add_escaped_chars(&rp, path);

  // look for an existing message
  in3_req_t* found = find_ctx_for(ctx, rp.data);
  if (found) {
    _free(rp.data);
    switch (in3_req_state(found)) {
      case REQ_ERROR:
        return req_set_error(ctx, found->error, found->verification_state ? found->verification_state : IN3_ERPC);
      case REQ_SUCCESS:
        *result = ctx->responses[0];
        if (!*result)
          return req_set_error(ctx, "error executing provider call", IN3_ERPC);
        return IN3_OK;
      case REQ_WAITING_TO_SEND:
      case REQ_WAITING_FOR_RESPONSE:
        return IN3_WAITING;
    }
  }

  TRY(create_iso_timestamp(ctx, time(NULL), timestamp))
  // strcpy(timestamp,"2021-02-08T17:10:21.351Z");
  rkey = _strdupn(rp.data, -1); // copy the request key to be used as cache key laster

  // continue the proof data and add the timestamp
  sb_add_chars(&rp, "\",\"qs\":[],\"headers\":[{\"key\":\"X-AUTH-TS\",\"value\":\""); // TODO [{\"key\":\"abc\",\"value\":\"abc\"}]
  sb_add_chars(&rp, timestamp);
  sb_add_chars(&rp, "\"}]}");

  in3_log_debug("requestProof : %s\n", rp.data);

  // hash it
  SHA256_CTX c;
  sha256_Init(&c);
  sha256_Update(&c, (void*) rp.data, rp.len);
  sha256_Final(&c, hash);

  in3_log_debug("sha256 : ");
  ba_print(hash, 32);

  // add ETH Prefix & sign it
  rp.len = 0;
  sb_add_chars(&rp, "\x19"
                    "Ethereum Signed Message:\n32");
  sb_add_range(&rp, (void*) hash, 0, 32);
  in3_log_debug("\nmessage to sign : %s\n", rp.data);
  ecdsa_sign(&secp256k1, HASHER_SHA3K, conf->device_key, (void*) rp.data, rp.len, signature, signature + 64, NULL);
  signature[64] += 27;

  // now create the request
  rp.len = 0;
  char chk_addr[41];
  ethereum_address_checksum(conf->device_address, chk_addr);
  sb_add_chars(&rp, "{\"method\":\"in3_http\",\"params\":[\"");
  sb_add_chars(&rp, method);
  sb_add_chars(&rp, "\",\"");
  sb_add_chars(&rp, url);
  sb_add_chars(&rp, path);
  sb_add_chars(&rp, "\",");
  sb_add_chars(&rp, payload ? payload : "null");
  sb_add_chars(&rp, ",[\"X-AUTH-TS:");
  sb_add_chars(&rp, timestamp);
  sb_add_rawbytes(&rp, "\",\"X-AUTH-SIGNATURE:0x", bytes(signature, 65), 65);
  sb_add_chars(&rp, "\",\"X-AUTH-ADDRESS:0x");
  sb_add_chars(&rp, chk_addr);
  sb_add_chars(&rp, "\"]]}");

  in3_log_debug("http-request: %s\n", rp.data);

  found = req_new(ctx->client, rp.data);
  if (!found) {
    _free(rkey);
    _free(rp.data);
    return req_set_error(ctx, "Invalid request!", IN3_ERPC);
  }
  in3_cache_add_ptr(&found->cache, rkey)->props = CACHE_PROP_SRC_REQ;
  return req_add_required(ctx, found);
}

static in3_ret_t add_response(in3_rpc_handle_ctx_t* ctx, d_token_t* result) {
  str_range_t r = d_to_json(result);
  r.data[r.len] = 0;
  return in3_rpc_handle_with_string(ctx, r.data);
}

in3_ret_t iamo_add_user(iamo_signer_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  TRY(check_device_key(ctx->ctx, conf))

  d_token_t* params = d_get(ctx->request, K_PARAMS);
  CHECK_PARAMS_LEN(ctx->ctx, params, 2);
  char*      email   = d_get_string_at(params, 0);
  char*      phone   = d_get_string_at(params, 1);
  d_token_t* result  = NULL;
  char*      payload = alloca(strlen(email) + strlen(phone) + 50);
  sprintf(payload, "{\"email\":\"%s\",\"phone\":\"%s\"}", email, phone);
  TRY(send_api_request(ctx->ctx, conf, conf->services.account, "post", "/api/user", payload, &result))
  //return in3_rpc_handle_with_boolean(ctx, true);
  return add_response(ctx, result);
}

static in3_ret_t iamo_rpc(iamo_signer_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  TRY_RPC("iamo_add_user", iamo_add_user(conf, ctx))
  return IN3_EIGNORE;
}

static in3_ret_t iamo_handle(void* data, in3_plugin_act_t action, void* action_ctx) {
  switch (action) {
    case PLGN_ACT_TERM: return iamo_free(data);
    case PLGN_ACT_CONFIG_SET: return iamo_config_set(data, action_ctx);
    case PLGN_ACT_RPC_HANDLE: return iamo_rpc(data, action_ctx);
    case PLGN_ACT_SIGN_ACCOUNT: return IN3_EIGNORE;
    case PLGN_ACT_SIGN: return IN3_EIGNORE;
    default: return IN3_ENOTSUP;
  }
  return IN3_ENOTSUP;
}

in3_ret_t register_iamo_signer(in3_t* in3) {
  return in3_plugin_register(in3, PLGN_ACT_TERM | PLGN_ACT_CONFIG_SET | PLGN_ACT_RPC_HANDLE | PLGN_ACT_SIGN | PLGN_ACT_SIGN_ACCOUNT, iamo_handle, _calloc(1, sizeof(iamo_signer_config_t)), true);
}
