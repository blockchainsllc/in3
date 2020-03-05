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

#include "../../../core/client/keys.h"
#include "../../../core/client/verifier.h"
#include "../../../core/util/mem.h"
#include <stdio.h>
#include <string.h>

static in3_ret_t find_code_in_accounts(in3_vctx_t* vc, address_t address, bytes_t** target, bytes_t** code_hash) {
  d_token_t* accounts = d_get(vc->proof, K_ACCOUNTS);
  if (!accounts) return IN3_EFIND;
  for (d_iterator_t iter = d_iter(accounts); iter.left; d_iter_next(&iter)) {
    if (memcmp(d_get_byteskl(iter.token, K_ADDRESS, 20)->data, address, 20) == 0) {
      // even if we don't have a code, we still set the code_hash, since we need it later to verify
      *code_hash    = d_get_bytesk(iter.token, K_CODE_HASH);
      bytes_t* code = d_get_bytesk(iter.token, K_CODE);
      if (code) {
        bytes32_t calculated_hash;
        sha3_to(code, calculated_hash);
        if (*code_hash && memcmp((*code_hash)->data, calculated_hash, 32) == 0) {
          *target = code;
          return IN3_OK;
        }
        vc_err(vc, "Wrong codehash");
        return IN3_EINVAL;
      }
    }
  }
  return IN3_EFIND;
}

static in3_ctx_t* find_pending_code_request(in3_vctx_t* vc, address_t address) {
  // ok, we need a request, do we have a useable?
  in3_ctx_t* ctx = vc->ctx->required;
  while (ctx) {
    if (strcmp(d_get_stringk(ctx->requests[0], K_METHOD), "eth_getCode") == 0) {
      // the first param of the eth_getCode is the address
      bytes_t adr = d_to_bytes(d_get_at(d_get(ctx->requests[0], K_PARAMS), 0));
      if (adr.len == 20 && memcmp(adr.data, address, 20) == 0) return ctx;
    }
    ctx = ctx->required;
  }
  return NULL;
}

static in3_ret_t in3_get_code_from_client(in3_vctx_t* vc, char* cache_key, address_t address, bool* must_free, bytes_t** target) {
  bytes_t* code_hash = NULL;

  in3_ret_t res = find_code_in_accounts(vc, address, target, &code_hash);
  // the only allowed error is not found, so keep on searching
  if (res != IN3_EFIND) return res;

  // ok, we need a request, do we have a useable?
  in3_ctx_t* ctx = find_pending_code_request(vc, address);

  // if we have found one, we verify the result and return the bytes.
  if (ctx)
    switch (in3_ctx_state(ctx)) {
      case CTX_SUCCESS: {
        d_token_t* rpc_result = d_get(ctx->responses[0], K_RESULT);
        if (!ctx->error && rpc_result) {
          bytes32_t calculated_code_hash;
          bytes_t   code = d_to_bytes(rpc_result);
          sha3_to(&code, calculated_code_hash);
          if (code_hash && memcmp(code_hash->data, calculated_code_hash, 32) != 0) {
            vc_err(vc, "Wrong codehash");
            ctx_remove_required(vc->ctx, ctx);
            // TODO maybe we should not give up here, but blacklist the node and try again!
            return IN3_EINVAL;
          }

          // since this is a sub request and and the code can be big, we don't want to duplicate the code.
          // so we keep the pointer  from the response and manipulate the resposen, so it won't be freed in the subrequest.
          *target          = _malloc(sizeof(bytes_t));
          (*target)->data  = code.data;
          (*target)->len   = code.len;
          rpc_result->data = NULL;
          *must_free       = 1;

          // we always try to cache the code
          if (vc->ctx->client->cache)
            vc->ctx->client->cache->set_item(vc->ctx->client->cache->cptr, cache_key, *target);
          return IN3_OK;
        } else
          return vc_err(vc, ctx->error);
      }
      case CTX_ERROR:
        return IN3_ERPC;
      default:
        return IN3_WAITING;
    }
  else {
    // for subrequests, we always need to allocate the request string, which will be freed when releasing the subrequest.
    char* req = _malloc(200);

    // we can use the cache_key, since it contains the hexencoded string with a "C"-prefix.
    snprintX(req, 200, "{\"method\":\"eth_getCode\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":[\"0x%s\",\"latest\"]}", cache_key + 1);
    in3_proof_t old_proof  = vc->ctx->client->proof;
    vc->ctx->client->proof = PROOF_NONE; // we don't need proof since we have the codehash!
    res                    = ctx_add_required(vc->ctx, ctx_new(vc->ctx->client, req));
    vc->ctx->client->proof = old_proof;
    return res;
  }
}

in3_ret_t in3_get_code(in3_vctx_t* vc, address_t address, cache_entry_t** target) {
  // search in thew cache of the current context
  for (cache_entry_t* en = vc->ctx->cache; en; en = en->next) {
    if (en->key.len == 20 && memcmp(address, en->key.data, 20) == 0) {
      *target = en;
      return IN3_OK;
    }
  }

  // the cache key is always "C"+the hexaddress (without prefix)
  char key_str[43];
  key_str[0] = 'C';
  bytes_to_hex(address, 20, key_str + 1);

  bytes_t*  code      = NULL;
  bool      must_free = false;
  in3_ret_t res;

  // not cached yet
  if (vc->ctx->client->cache)
    code = vc->ctx->client->cache->get_item(vc->ctx->client->cache->cptr, key_str);

  if (code)
    must_free = 1;
  else {
    res = in3_get_code_from_client(vc, key_str, address, &must_free, &code);
    if (res < 0) return res;
  }

  if (code) {
    bytes_t key = bytes(_malloc(20), 20);
    memcpy(key.data, address, 20);
    *target              = in3_cache_add_entry(&vc->ctx->cache, key, *code);
    (*target)->must_free = must_free;

    // we also store the length into the 4 bytes buffer, so we can reference it later on.
    int_to_bytes(code->len, (*target)->buffer);
    return IN3_OK;
  }
  return IN3_EFIND;
}