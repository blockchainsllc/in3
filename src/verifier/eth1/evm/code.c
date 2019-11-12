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

#include "../../../core/client/keys.h"
#include "../../../core/client/verifier.h"
#include "../../../core/util/mem.h"
#include <stdio.h>
#include <string.h>

in3_ret_t in3_get_code_from_client(in3_vctx_t* vc, char* hex_address, uint8_t* address, uint8_t* must_free, bytes_t** target) {
  bytes_t *  res = NULL, *code_hash = NULL, tmp[32];
  d_token_t* t = NULL;

  // first see, if this is part of the proof
  d_token_t* accounts = d_get(vc->proof, K_ACCOUNTS);
  int        i;
  if (accounts) {
    for (i = 0, t = accounts + 1; i < d_len(accounts); i++, t = d_next(t)) {
      if (memcmp(d_get_byteskl(t, K_ADDRESS, 20)->data, address, 20) == 0) {
        code_hash = d_get_byteskl(t, K_CODE_HASH, 32);
        res       = d_get_bytesk(t, K_CODE);
        if (res) {
          sha3_to(res, tmp);
          if (code_hash && memcmp(code_hash->data, tmp, 32) == 0) {
            *target = res;
            return IN3_OK;
          } else {
            vc_err(vc, "Wrong codehash");
            return IN3_EINVAL;
          }
        }
        break;
      }
    }
  }

  // ok, we need a request, do we have a useable?
  in3_ctx_t* ctx = vc->ctx->required;
  while (ctx) {
    if (strcmp(d_get_stringk(ctx->requests[0], K_METHOD), "eth_getCode") == 0) {
      bytes_t ad = d_to_bytes(d_get_at(d_get(ctx->requests[0], K_PARAMS), 0));
      if (ad.len == 20 && memcmp(ad.data, address, 20) == 0) break;
    }
    ctx = ctx->required;
  }

  // if we have found one, we verify the result and return the bytes.
  if (ctx)
    switch (in3_ctx_state(ctx)) {
      case CTX_SUCCESS: {
        if (!ctx->error && ctx->responses[0] && (t = d_get(ctx->responses[0], K_RESULT))) {
          bytes_t b = d_to_bytes(t);
          sha3_to(&b, tmp);
          if (code_hash && memcmp(code_hash->data, tmp, 32) != 0) {
            vc_err(vc, "Wrong codehash");
            in3_remove_required(vc->ctx, ctx);
            // TODO maybe we should not give up here, but blacklist the node and try again!
            return IN3_EINVAL;
          }
          *must_free = 1;
          if (vc->ctx->client->cacheStorage)
            vc->ctx->client->cacheStorage->set_item(vc->ctx->client->cacheStorage->cptr, hex_address, &b);
          else
            *target = b_dup(d_bytes(t));
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
    char* req = _malloc(200);
    snprintX(req, 200, "{\"method\":\"eth_getCode\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":[\"0x%s\",\"latest\"]}", hex_address + 1);
    in3_proof_t old_proof  = vc->ctx->client->proof;
    vc->ctx->client->proof = PROOF_NONE; // we don't need proof since we have the codehash!
    i                      = in3_add_required(vc->ctx, new_ctx(vc->ctx->client, req));
    vc->ctx->client->proof = old_proof;
    return i;
  }
}
in3_ret_t in3_get_code(in3_vctx_t* vc, uint8_t* address, cache_entry_t** target) {
  for (cache_entry_t* en = vc->ctx->cache; en; en = en->next) {
    if (en->key.len == 20 && memcmp(address, en->key.data, 20) == 0) {
      *target = en;
      return IN3_OK;
    }
  }
  char key_str[43];
  key_str[0] = 'C';
  bytes_to_hex(address, 20, key_str + 1);
  bytes_t*       b         = NULL;
  cache_entry_t* entry     = NULL;
  uint8_t        must_free = 0;
  in3_ret_t      ret;

  // not cached yet
  if (vc->ctx->client->cacheStorage) {
    b = vc->ctx->client->cacheStorage->get_item(vc->ctx->client->cacheStorage->cptr, key_str);
    if (!b) {
      ret = in3_get_code_from_client(vc, key_str, address, &must_free, &b);
      if (ret < 0) return ret;
      b = vc->ctx->client->cacheStorage->get_item(vc->ctx->client->cacheStorage->cptr, key_str);
    } else
      must_free = 1;
  } else {
    ret = in3_get_code_from_client(vc, key_str, address, &must_free, &b);
    if (ret < 0) return ret;
  }

  if (b) {
    bytes_t key = {.len = 20, .data = _malloc(20)};
    memcpy(key.data, address, 20);
    entry            = _malloc(sizeof(cache_entry_t));
    entry->next      = vc->ctx->cache;
    entry->key       = key;
    entry->must_free = must_free;
    entry->value     = *b;
    vc->ctx->cache   = entry;
    int_to_bytes(b->len, entry->buffer);
    *target = entry;
    return IN3_OK;
  }
  return IN3_EFIND;
}