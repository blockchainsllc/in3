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

#include "cache.h"
#include "../util/log.h"
#include "../util/mem.h"
#include "../util/utils.h"
#include "context.h"
#include "nodelist.h"
#include "stdio.h"
#include <inttypes.h>
#include <string.h>

#define NODE_LIST_KEY ("nodelist_%" PRIx64 "_0x%s")
#define CACHE_VERSION 3

static void write_cache_key(char* key, uint64_t chain_id, bytes_t* contract) {
  char contract_[41];
  if (contract) bytes_to_hex(contract->data, contract->len, contract_);
  sprintf(key, NODE_LIST_KEY, chain_id, contract_);
}

in3_ret_t in3_cache_init(in3_t* c) {
  int i;
  // the reason why we ignore the result here, is because we want to ignore errors if the cache is able to update.
  for (i = 0; i < c->chainsCount; i++) {
    if (in3_cache_update_nodelist(c, c->chains + i) != IN3_OK) {
      in3_log_debug("Failed to update cached nodelist\n");
    }
    if (in3_cache_update_whitelist(c, c->chains + i) != IN3_OK) {
      in3_log_debug("Failed to update cached whitelist\n");
    }
  }
  return IN3_OK;
}

in3_ret_t in3_cache_update_nodelist(in3_t* c, in3_chain_t* chain) {
  // it is ok not to have a storage
  if (!c->cacheStorage) return IN3_OK;

  // define the key to use
  char key[200];
  write_cache_key(key, chain->chainId, chain->contract);

  // get from cache
  bytes_t* b = c->cacheStorage->get_item(c->cacheStorage->cptr, key);
  if (b) {
    int    i, count;
    size_t p = 0;

    // version check
    if (b_read_byte(b, &p) != CACHE_VERSION) {
      b_free(b);
      return IN3_EVERS;
    }

    // clean up old
    in3_nodelist_clear(chain);

    // fill data
    chain->contract       = b_new_fixed_bytes(b, &p, 20);
    chain->lastBlock      = b_read_long(b, &p);
    chain->nodeListLength = count = b_read_int(b, &p);
    chain->nodeList               = _calloc(count, sizeof(in3_node_t));
    chain->weights                = _calloc(count, sizeof(in3_node_weight_t));
    chain->needsUpdate            = UPDATE_NONE;
    memcpy(chain->weights, b->data + p, count * sizeof(in3_node_weight_t));
    p += count * sizeof(in3_node_weight_t);

    for (i = 0; i < count; i++) {
      in3_node_t* n  = chain->nodeList + i;
      n->capacity    = b_read_int(b, &p);
      n->index       = b_read_int(b, &p);
      n->deposit     = b_read_long(b, &p);
      n->props       = b_read_long(b, &p);
      n->address     = b_new_fixed_bytes(b, &p, 20);
      n->url         = b_new_chars(b, &p);
      n->whiteListed = b_read_byte(b, &p) != 0;
    }
    b_free(b);
  }
  return IN3_OK;
}

in3_ret_t in3_cache_store_nodelist(in3_ctx_t* ctx, in3_chain_t* chain) {
  int i;

  // write to bytes_buffer
  bytes_builder_t* bb = bb_new();
  bb_write_byte(bb, CACHE_VERSION);          // Version flag
  bb_write_fixed_bytes(bb, chain->contract); // 20 bytes fixed

  bb_write_long(bb, chain->lastBlock);
  bb_write_int(bb, chain->nodeListLength);
  bb_write_raw_bytes(bb, chain->weights, chain->nodeListLength * sizeof(in3_node_weight_t));

  for (i = 0; i < chain->nodeListLength; i++) {
    in3_node_t* n = chain->nodeList + i;
    bb_write_int(bb, n->capacity);
    bb_write_int(bb, n->index);
    bb_write_long(bb, n->deposit);
    bb_write_long(bb, n->props);
    bb_write_fixed_bytes(bb, n->address);
    bb_write_chars(bb, n->url, strlen(n->url));
    bb_write_byte(bb, n->whiteListed);
  }

  // create key
  char key[200];
  write_cache_key(key, chain->chainId, chain->contract);

  // store it and ignore return value since failing when writing cache should not stop us.
  ctx->client->cacheStorage->set_item(ctx->client->cacheStorage->cptr, key, &bb->b);

  // clear buffer
  bb_free(bb);
  return IN3_OK;
}

in3_ret_t in3_cache_update_whitelist(in3_t* c, in3_chain_t* chain) {
  // it is ok not to have a storage
  if (!c->cacheStorage) return IN3_OK;

  // define the key to use
  char key[200];
  write_cache_key(key, chain->chainId, chain->whiteListContract);

  // get from cache
  bytes_t* b = c->cacheStorage->get_item(c->cacheStorage->cptr, key);
  if (b) {
    size_t p = 0;

    // version check
    if (b_read_byte(b, &p) != CACHE_VERSION) {
      b_free(b);
      return IN3_EVERS;
    }

    // clean up old
    b_free(chain->whiteList);

    // fill data
    chain->lastBlockWl       = b_read_long(b, &p);
    chain->whiteListContract = b_new_fixed_bytes(b, &p, 20);
    if (!memiszero(chain->whiteListContract->data, 20)) {
      b_free(chain->whiteListContract);
      chain->whiteListContract = NULL;
    }
    p += 20;

    uint32_t l = b_read_int(b, &p) * 20;
    b_readl(b, p, chain->whiteList->data, l);
    chain->whiteList->len = l;
    b_free(b);
  }
  return IN3_OK;
}

in3_ret_t in3_cache_store_whitelist(in3_ctx_t* ctx, in3_chain_t* chain) {
  // write to bytes_buffer
  bytes_builder_t* bb = bb_new();
  bb_write_byte(bb, CACHE_VERSION); // Version flag

  if (chain->whiteListContract) {
    bb_write_fixed_bytes(bb, chain->whiteListContract);
  } else {
    uint8_t tmp[20] = {0};
    bb_write_raw_bytes(bb, tmp, 20); // 20 bytes fixed
  }

  bb_write_int(bb, chain->whiteList->len / 20);
  bb_write_fixed_bytes(bb, chain->whiteList);

  // create key
  char key[200];
  write_cache_key(key, chain->chainId, chain->whiteListContract);

  // store it and ignore return value since failing when writing cache should not stop us.
  ctx->client->cacheStorage->set_item(ctx->client->cacheStorage->cptr, key, &bb->b);

  // clear buffer
  bb_free(bb);
  return IN3_OK;
}
