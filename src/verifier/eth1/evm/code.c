/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 *
 * Copyright (C) 2019 Blockchains, LLC
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
 * 
 * If you cannot meet the requirements of AGPL, 
 * you should contact us to inquire about a commercial license.
 *******************************************************************************/

#include "../../../core/client/keys.h"
#include "../../../core/client/verifier.h"
#include "../../../core/util/mem.h"
#include <stdio.h>
#include <string.h>

bytes_t* in3_get_code_from_client(in3_vctx_t* vc, char* hex_address, uint8_t* address, uint8_t* must_free) {
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
          if (code_hash && memcmp(code_hash->data, tmp, 32) == 0)
            return res;
          else {
            vc_err(vc, "Wrong codehash");
            return NULL;
          }
        }
        break;
      }
    }
  }

  char params[100];
  sprintf(params, "[\"0x%s\",\"latest\"]", hex_address + 1);
  in3_proof_t old_proof  = vc->ctx->client->proof;
  vc->ctx->client->proof = PROOF_NONE; // we don't need proof since we have the codehash!
  in3_ctx_t* ctx         = in3_client_rpc_ctx(vc->ctx->client, "eth_getCode", params);
  vc->ctx->client->proof = old_proof;
  if (!ctx->error && ctx->responses[0] && (t = d_get(ctx->responses[0], K_RESULT))) {
    sha3_to(d_bytes(t), tmp);
    if (code_hash && memcmp(code_hash->data, tmp, 32) != 0) {
      vc_err(vc, "Wrong codehash");
      return NULL;
    }
    if (vc->ctx->client->cacheStorage)
      vc->ctx->client->cacheStorage->set_item(vc->ctx->client->cacheStorage->cptr, hex_address, d_bytes(t));
    else
      res = b_dup(d_bytes(t));
  } else
    vc_err(vc, ctx->error);
  free_ctx(ctx);
  *must_free = 1;
  return res;
}

cache_entry_t* in3_get_code(in3_vctx_t* vc, uint8_t* address) {
  for (cache_entry_t* en = vc->ctx->cache; en; en = en->next) {
    if (en->key.len == 20 && memcmp(address, en->key.data, 20) == 0)
      return en;
  }
  char key_str[43];
  key_str[0] = 'C';
  bytes_to_hex(address, 20, key_str + 1);
  bytes_t*       b         = NULL;
  cache_entry_t* entry     = NULL;
  uint8_t        must_free = 0;

  // not cached yet
  if (vc->ctx->client->cacheStorage) {
    b = vc->ctx->client->cacheStorage->get_item(vc->ctx->client->cacheStorage->cptr, key_str);
    if (!b) {
      in3_get_code_from_client(vc, key_str, address, &must_free);
      b = vc->ctx->client->cacheStorage->get_item(vc->ctx->client->cacheStorage->cptr, key_str);
    }
  } else
    b = in3_get_code_from_client(vc, key_str, address, &must_free);

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
    return entry;
  }
  return NULL;
}