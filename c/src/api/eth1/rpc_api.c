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

#include "../../core/client/context_internal.h"
#include "../../core/client/keys.h"
#include "../../core/client/verifier.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/rand.h"
#include "../../third-party/crypto/secp256k1.h"
#include "../../verifier/eth1/nano/rlp.h"
#include "abi.h"
#include "ens.h"
#include "eth_api.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define ETH_SIGN_PREFIX "\x19" \
                        "Ethereum Signed Message:\n%i"

#define RESPONSE_START()                                                             \
  do {                                                                               \
    *response = _malloc(sizeof(in3_response_t));                                     \
    sb_init(&response[0]->result);                                                   \
    sb_init(&response[0]->error);                                                    \
    sb_add_chars(&response[0]->result, "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":"); \
  } while (0)

#define RESPONSE_END() \
  do { sb_add_char(&response[0]->result, '}'); } while (0)

static in3_verify     parent_verify = NULL;
static in3_pre_handle parent_handle = NULL;

static in3_ret_t in3_abiEncode(in3_ctx_t* ctx, d_token_t* params, in3_response_t** response) {
  RESPONSE_START();
  call_request_t* req = parseSignature(d_get_string_at(params, 0));
  if (!req) return ctx_set_error(ctx, "invalid function signature", IN3_EINVAL);

  d_token_t* para = d_get_at(params, 1);
  if (!para) {
    req_free(req);
    return ctx_set_error(ctx, "invalid json data", IN3_EINVAL);
  }

  if (set_data(req, para, req->in_data) < 0) {
    req_free(req);
    return ctx_set_error(ctx, "invalid input data", IN3_EINVAL);
  }
  sb_add_bytes(&response[0]->result, NULL, &req->call_data->b, 1, false);
  req_free(req);
  RESPONSE_END();
  return IN3_OK;
}
static in3_ret_t in3_abiDecode(in3_ctx_t* ctx, d_token_t* params, in3_response_t** response) {
  RESPONSE_START();
  char *sig = d_get_string_at(params, 0), *full_sig = alloca(strlen(sig) + 10);
  if (strstr(sig, ":"))
    strcpy(full_sig, sig);
  else
    sprintf(full_sig, "test():%s", sig);

  call_request_t* req = parseSignature(full_sig);
  if (!req) return ctx_set_error(ctx, "invalid function signature", IN3_EINVAL);

  json_ctx_t* res = req_parse_result(req, d_to_bytes(d_get_at(params, 1)));
  req_free(req);
  if (!res)
    return ctx_set_error(ctx, "the input data can not be decoded", IN3_EINVAL);
  char* result = d_create_json(res->result);
  sb_add_chars(&response[0]->result, result);
  json_free(res);
  _free(result);

  RESPONSE_END();
  return IN3_OK;
}
static in3_ret_t in3_checkSumAddress(in3_ctx_t* ctx, d_token_t* params, in3_response_t** response) {
  bytes_t* adr = d_get_bytes_at(params, 0);
  if (!adr || adr->len != 20) return ctx_set_error(ctx, "the address must have 20 bytes", IN3_EINVAL);
  char      result[43];
  in3_ret_t res = to_checksum(adr->data, d_get_int_at(params, 1) ? ctx->client->chain_id : 0, result);
  if (res) return ctx_set_error(ctx, "Could not create the checksum address", res);
  RESPONSE_START();
  sb_add_char(&response[0]->result, '\'');
  sb_add_chars(&response[0]->result, result);
  sb_add_char(&response[0]->result, '\'');
  RESPONSE_END();
  return IN3_OK;
}
static in3_ret_t in3_ens(in3_ctx_t* ctx, d_token_t* params, in3_response_t** response) {
  char*        name     = d_get_string_at(params, 0);
  char*        type     = d_get_string_at(params, 1);
  bytes_t      registry = d_to_bytes(d_get_at(params, 2));
  int          res_len  = 20;
  in3_ens_type ens_type = ENS_ADDR;
  bytes32_t    result;

  // verify input
  if (!type) type = "addr";
  if (!name || !strchr(name, '.')) return ctx_set_error(ctx, "the first param msut be a valid domain name", IN3_EINVAL);
  if (strcmp(type, "addr") == 0)
    ens_type = ENS_ADDR;
  else if (strcmp(type, "resolver") == 0)
    ens_type = ENS_RESOLVER;
  else if (strcmp(type, "owner") == 0)
    ens_type = ENS_OWNER;
  else if (strcmp(type, "hash") == 0)
    ens_type = ENS_HASH;
  else
    return ctx_set_error(ctx, "currently only 'hash','addr','owner' or 'resolver' are allowed as type", IN3_EINVAL);
  if (registry.data && registry.len != 20) return ctx_set_error(ctx, "the registry must be a 20 bytes address", IN3_EINVAL);

  in3_ret_t res = ens_resolve(ctx, name, registry.data, ens_type, result, &res_len);
  if (res < 0) return res;
  bytes_t result_bytes = bytes(result, res_len);

  RESPONSE_START();
  sb_add_bytes(&response[0]->result, NULL, &result_bytes, 1, false);
  RESPONSE_END();
  return IN3_OK;
}
static in3_ret_t in3_sha3(in3_ctx_t* ctx, d_token_t* params, in3_response_t** response) {
  if (!params) return ctx_set_error(ctx, "no data", IN3_EINVAL);
  bytes_t data = d_to_bytes(params + 1);
  RESPONSE_START();
  bytes32_t hash;
  bytes_t   hbytes = bytes(hash, 32);
  sha3_to(&data, hash);
  sb_add_bytes(&response[0]->result, NULL, &hbytes, 1, false);
  RESPONSE_END();
  return IN3_OK;
}
static in3_ret_t in3_config(in3_ctx_t* ctx, d_token_t* params, in3_response_t** response) {
  str_range_t r   = d_to_json(d_get_at(params, 0));
  char        old = r.data[r.len];
  r.data[r.len]   = 0;
  char* ret       = in3_configure(ctx->client, r.data);
  r.data[r.len]   = old;
  if (ret) {
    ctx_set_error(ctx, ret, IN3_ECONFIG);
    free(ret);
    return IN3_ECONFIG;
  }
  RESPONSE_START();
  sb_add_chars(&response[0]->result, "true");
  RESPONSE_END();
  return IN3_OK;
}

static in3_ret_t in3_pk2address(in3_ctx_t* ctx, d_token_t* params, in3_response_t** response) {
  bytes_t* pk = d_get_bytes_at(params, 0);
  if (!pk || pk->len != 32) return ctx_set_error(ctx, "Invalid private key! must be 32 bytes long", IN3_EINVAL);

  uint8_t public_key[65], sdata[32];
  bytes_t pubkey_bytes = {.data = public_key + 1, .len = 64}, addr = bytes(sdata + 12, 20);
  ecdsa_get_public_key65(&secp256k1, pk->data, public_key);
  RESPONSE_START();
  if (strcmp(d_get_stringk(ctx->requests[0], K_METHOD), "in3_pk2address") == 0) {
    sha3_to(&pubkey_bytes, sdata);
    sb_add_bytes(&response[0]->result, NULL, &addr, 1, false);
  } else
    sb_add_bytes(&response[0]->result, NULL, &pubkey_bytes, 1, false);
  RESPONSE_END();
  return IN3_OK;
}

static in3_ret_t in3_ecrecover(in3_ctx_t* ctx, d_token_t* params, in3_response_t** response) {
  bytes_t  msg      = d_to_bytes(d_get_at(params, 0));
  bytes_t* sig      = d_get_bytes_at(params, 1);
  char*    sig_type = d_get_string_at(params, 2);
  if (!sig_type) sig_type = "raw";
  if (!sig || sig->len != 65) return ctx_set_error(ctx, "Invalid signature! must be 65 bytes long", IN3_EINVAL);
  if (!msg.data) return ctx_set_error(ctx, "Missing message", IN3_EINVAL);

  bytes32_t hash;
  uint8_t   pub[65];
  bytes_t   pubkey_bytes = {.len = 64, .data = ((uint8_t*) &pub) + 1};
  if (strcmp(sig_type, "eth_sign") == 0) {
    char*     tmp = alloca(msg.len + 30);
    const int l   = sprintf(tmp, ETH_SIGN_PREFIX, msg.len);
    memcpy(tmp + l, msg.data, msg.len);
    msg.data = (uint8_t*) tmp;
    msg.len += l;
  }
  if (strcmp(sig_type, "hash") == 0) {
    if (msg.len != 32) return ctx_set_error(ctx, "The message hash must be 32 byte", IN3_EINVAL);
    memcpy(hash, msg.data, 32);
  } else
    sha3_to(&msg, hash);

  if (ecdsa_recover_pub_from_sig(&secp256k1, pub, sig->data, hash, sig->data[64] >= 27 ? sig->data[64] - 27 : sig->data[64]))
    return ctx_set_error(ctx, "Invalid Signature", IN3_EINVAL);

  RESPONSE_START();
  sb_add_char(&response[0]->result, '{');
  sha3_to(&pubkey_bytes, hash);
  sb_add_bytes(&response[0]->result, "\"publicKey\":", &pubkey_bytes, 1, false);
  sb_add_char(&response[0]->result, ',');
  pubkey_bytes.data = hash + 12;
  pubkey_bytes.len  = 20;
  sb_add_bytes(&response[0]->result, "\"address\":", &pubkey_bytes, 1, false);
  sb_add_char(&response[0]->result, '}');
  RESPONSE_END();
  return IN3_OK;
}
static in3_ret_t in3_sign_data(in3_ctx_t* ctx, d_token_t* params, in3_response_t** response) {
  bytes_t        data     = d_to_bytes(d_get_at(params, 0));
  const bytes_t* pk       = d_get_bytes_at(params, 1);
  char*          sig_type = d_get_string_at(params, 2);
  if (!sig_type) sig_type = "raw";

  if (!pk) return ctx_set_error(ctx, "Invalid sprivate key! must be 32 bytes long", IN3_EINVAL);
  if (!data.data) return ctx_set_error(ctx, "Missing message", IN3_EINVAL);

  if (strcmp(sig_type, "eth_sign") == 0) {
    char*     tmp = alloca(data.len + 30);
    const int l   = sprintf(tmp, ETH_SIGN_PREFIX, data.len);
    memcpy(tmp + l, data.data, data.len);
    data.data = (uint8_t*) tmp;
    data.len += l;
    sig_type = "raw";
  }

  uint8_t sig[65];
  bytes_t sig_bytes = bytes(sig, 65);
  if (pk->len == 20)
    ctx->client->signer->sign(&ctx, strcmp(sig_type, "hash") == 0 ? SIGN_EC_RAW : SIGN_EC_HASH, data, *pk, sig);
  else if (!pk->data)
    ctx->client->signer->sign(&ctx, strcmp(sig_type, "hash") == 0 ? SIGN_EC_RAW : SIGN_EC_HASH, data, bytes(NULL, 0), sig);
  else if (pk->len == 32) {
    if (strcmp(sig_type, "hash") == 0)
      ecdsa_sign_digest(&secp256k1, pk->data, data.data, sig, sig + 64, NULL);
    else if (strcmp(sig_type, "raw") == 0)
      ecdsa_sign(&secp256k1, HASHER_SHA3K, pk->data, data.data, data.len, sig, sig + 64, NULL);
    else
      return ctx_set_error(ctx, "unsupported sigType", IN3_EINVAL);
  } else
    return ctx_set_error(ctx, "Invalid private key! Must be either an address(20 byte) or an raw private key (32 byte)", IN3_EINVAL);
  sig[64] += 27;

  RESPONSE_START();
  sb_add_char(&response[0]->result, '{');
  sb_add_bytes(&response[0]->result, "\"message\":", &data, 1, false);
  sb_add_char(&response[0]->result, ',');
  if (strcmp(sig_type, "raw") == 0) {
    bytes32_t hash_val;
    bytes_t   hash_bytes = bytes(hash_val, 32);
    sha3_to(&data, hash_val);
    sb_add_bytes(&response[0]->result, "\"messageHash\":", &hash_bytes, 1, false);
  } else
    sb_add_bytes(&response[0]->result, "\"messageHash\":", &data, 1, false);
  sb_add_char(&response[0]->result, ',');
  sb_add_bytes(&response[0]->result, "\"signature\":", &sig_bytes, 1, false);
  sig_bytes = bytes(sig, 32);
  sb_add_char(&response[0]->result, ',');
  sb_add_bytes(&response[0]->result, "\"r\":", &sig_bytes, 1, false);
  sig_bytes = bytes(sig + 32, 32);
  sb_add_char(&response[0]->result, ',');
  sb_add_bytes(&response[0]->result, "\"s\":", &sig_bytes, 1, false);
  char v[15];
  sprintf(v, ",\"v\":%d}", (unsigned int) sig[64]);
  sb_add_chars(&response[0]->result, v);
  RESPONSE_END();
  return IN3_OK;
}

static in3_ret_t in3_cacheClear(in3_ctx_t* ctx, in3_response_t** response) {
  in3_storage_handler_t* cache = ctx->client->cache;
  if (!cache || !cache->clear)
    return ctx_set_error(ctx, "No storage set", IN3_ECONFIG);
  cache->clear(cache->cptr);
  RESPONSE_START();
  sb_add_chars(&response[0]->result, "true");
  RESPONSE_END();
  return IN3_OK;
}

static in3_ret_t in3_decryptKey(in3_ctx_t* ctx, d_token_t* params, in3_response_t** response) {
  d_token_t* keyfile        = d_get_at(params, 0);
  bytes_t    password_bytes = d_to_bytes(d_get_at(params, 1));
  bytes32_t  dst;
  bytes_t    dst_bytes = bytes(dst, 32);

  if (!password_bytes.data) return ctx_set_error(ctx, "you need to specify a passphrase", IN3_EINVAL);
  if (!keyfile || d_type(keyfile) != T_OBJECT) return ctx_set_error(ctx, "no valid key given", IN3_EINVAL);
  char* passphrase = alloca(password_bytes.len + 1);
  memcpy(passphrase, password_bytes.data, password_bytes.len);
  passphrase[password_bytes.len] = 0;
  in3_ret_t res                  = decrypt_key(keyfile, passphrase, dst);
  if (res) return ctx_set_error(ctx, "Invalid key", res);

  RESPONSE_START();
  sb_add_bytes(&response[0]->result, NULL, &dst_bytes, 1, false);
  RESPONSE_END();
  return IN3_OK;
}

static in3_ret_t eth_handle_intern(in3_ctx_t* ctx, in3_response_t** response) {
  if (ctx->len > 1) return IN3_ENOTSUP; // internal handling is only possible for single requests (at least for now)
  d_token_t* r      = ctx->requests[0];
  char*      method = d_get_stringk(r, K_METHOD);
  d_token_t* params = d_get(r, K_PARAMS);

  if (strcmp(method, "in3_abiEncode") == 0) return in3_abiEncode(ctx, params, response);
  if (strcmp(method, "in3_abiDecode") == 0) return in3_abiDecode(ctx, params, response);
  if (strcmp(method, "in3_checksumAddress") == 0) return in3_checkSumAddress(ctx, params, response);
  if (strcmp(method, "in3_ens") == 0) return in3_ens(ctx, params, response);
  if (strcmp(method, "web3_sha3") == 0) return in3_sha3(ctx, params, response);
  if (strcmp(method, "in3_config") == 0) return in3_config(ctx, params, response);
  if (strcmp(method, "in3_pk2address") == 0) return in3_pk2address(ctx, params, response);
  if (strcmp(method, "in3_pk2public") == 0) return in3_pk2address(ctx, params, response);
  if (strcmp(method, "in3_ecrecover") == 0) return in3_ecrecover(ctx, params, response);
  if (strcmp(method, "in3_signData") == 0) return in3_sign_data(ctx, params, response);
  if (strcmp(method, "in3_cacheClear") == 0) return in3_cacheClear(ctx, response);
  if (strcmp(method, "in3_decryptKey") == 0) return in3_decryptKey(ctx, params, response);

  return parent_handle ? parent_handle(ctx, response) : IN3_OK;
}

static int verify(in3_vctx_t* v) {
  char* method = d_get_stringk(v->request, K_METHOD);
  if (!method) return vc_err(v, "no method in the request!");

  if (strcmp(method, "in3_abiEncode") == 0 ||
      strcmp(method, "in3_abiDecode") == 0 ||
      strcmp(method, "in3_checksumAddress") == 0 ||
      strcmp(method, "web3_sha3") == 0 ||
      strcmp(method, "in3_ens") == 0 ||
      strcmp(method, "in3_config") == 0 ||
      strcmp(method, "in3_pk2address") == 0 ||
      strcmp(method, "in3_ecrecover") == 0 ||
      strcmp(method, "in3_signData") == 0 ||
      strcmp(method, "in3_pk2public") == 0 ||
      strcmp(method, "in3_decryptKey") == 0 ||
      strcmp(method, "in3_cacheClear") == 0)
    return IN3_OK;

  return parent_verify ? parent_verify(v) : IN3_ENOTSUP;
}

void in3_register_eth_api() {
  in3_verifier_t* v = in3_get_verifier(CHAIN_ETH);
  if (v && v->pre_handle == eth_handle_intern) return;
  if (v) {
    parent_verify = v->verify;
    parent_handle = v->pre_handle;
    v->verify     = (in3_verify) verify;
    v->pre_handle = eth_handle_intern;
  } else {
    in3_verifier_t* v = _calloc(1, sizeof(in3_verifier_t));
    v->type           = CHAIN_ETH;
    v->pre_handle     = eth_handle_intern;
    v->verify         = (in3_verify) verify;
    in3_register_verifier(v);
  }
}

// needed rpcs
/*
in3_send
in3_call
in3_pk2address
in3_pk2public
in3_ecrecover
in3_key
in3_hash

*/