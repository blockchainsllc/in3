/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2019 slock.it GmbH, Blockchains LLC
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

#include "../../core/client/context.h"
#include "../../core/client/keys.h"
#include "../../core/client/verifier.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "../../verifier/eth1/nano/rlp.h"
#include "abi.h"
#include "eth_api.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
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
  sb_add_char(&response[0]->result, '"');
  sb_add_chars(&response[0]->result, result);
  sb_add_char(&response[0]->result, '"');
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
  in3_ret_t ret   = in3_configure(ctx->client, r.data);
  r.data[r.len]   = old;
  if (ret) return ctx_set_error(ctx, "Invalid config", ret);

  RESPONSE_START();
  sb_add_chars(&response[0]->result, "true");
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

static in3_ret_t eth_handle_intern(in3_ctx_t* ctx, in3_response_t** response) {
  if (ctx->len > 1) return IN3_ENOTSUP; // internal handling is only possible for single requests (at least for now)
  d_token_t* r      = ctx->requests[0];
  char*      method = d_get_stringk(r, K_METHOD);
  d_token_t* params = d_get(r, K_PARAMS);

  if (strcmp(method, "in3_abiEncode") == 0) return in3_abiEncode(ctx, params, response);
  if (strcmp(method, "in3_abiDecode") == 0) return in3_abiDecode(ctx, params, response);
  if (strcmp(method, "in3_checksumAddress") == 0) return in3_checkSumAddress(ctx, params, response);
  if (strcmp(method, "web3_sha3") == 0) return in3_sha3(ctx, params, response);
  if (strcmp(method, "in3_config") == 0) return in3_config(ctx, params, response);
  if (strcmp(method, "in3_cacheClear") == 0) return in3_cacheClear(ctx, response);

  return parent_handle ? parent_handle(ctx, response) : IN3_OK;
}

static int verify(in3_vctx_t* v) {
  char* method = d_get_stringk(v->request, K_METHOD);
  if (!method) return vc_err(v, "no method in the request!");

  if (strcmp(method, "in3_abiEncode") == 0 ||
      strcmp(method, "in3_abiDecode") == 0 ||
      strcmp(method, "in3_checksumAddress") == 0 ||
      strcmp(method, "web3_sha3") == 0 ||
      strcmp(method, "in3_config") == 0 ||
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
    v->verify     = verify;
    v->pre_handle = eth_handle_intern;
  } else {
    in3_verifier_t* v = _calloc(1, sizeof(in3_verifier_t));
    v->type           = CHAIN_ETH;
    v->pre_handle     = eth_handle_intern;
    v->verify         = verify;
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