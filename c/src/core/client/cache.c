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

#include "cache.h"
#include "../util/bitset.h"
#include "../util/log.h"
#include "../util/mem.h"
#include "../util/utils.h"
#include "context.h"
#include "nodelist.h"
#include "stdio.h"
#include <inttypes.h>
#include <string.h>

#define NODE_LIST_KEY "nodelist_%d"
#define WHITTE_LIST_KEY "_0x%s"
#define CACHE_VERSION 6
#define MAX_KEYLEN 200

static void write_cache_key(char* key, chain_id_t chain_id, const address_t contract) {
  if (contract && contract) {
    char contract_[41];
    if (contract) bytes_to_hex(contract, 20, contract_);
    sprintf(key, NODE_LIST_KEY WHITTE_LIST_KEY, chain_id, contract_);
  } else
    sprintf(key, NODE_LIST_KEY, chain_id);
}

in3_ret_t in3_cache_init(in3_t* c) {
  // the reason why we ignore the result here, is because we want to ignore errors if the cache is able to update.
  for (int i = 0; i < c->chains_length; i++) {
    if (in3_cache_update_nodelist(c, c->chains + i) != IN3_OK) { in3_log_debug("Failed to update cached nodelist\n"); }
    if (in3_cache_update_whitelist(c, c->chains + i) != IN3_OK) { in3_log_debug("Failed to update cached whitelist\n"); }
    in3_client_run_chain_whitelisting(c->chains + i);
  }

  return IN3_OK;
}

in3_ret_t in3_cache_update_nodelist(in3_t* c, in3_chain_t* chain) {
  // it is ok not to have a storage
  if (!c->cache) return IN3_OK;

  // define the key to use
  char key[MAX_KEYLEN];
  write_cache_key(key, chain->chain_id, chain->contract->data);

  // get from cache
  bytes_t* b = c->cache->get_item(c->cache->cptr, key);
  if (!b) return IN3_OK;

  // so we have a result... let's decode it.
  int    node_count;
  size_t pos = 0;

  // version check
  if (b_read_byte(b, &pos) != CACHE_VERSION) {
    b_free(b);
    return IN3_EVERS;
  }

  // clean up old
  in3_nodelist_clear(chain);

  // fill data
  chain->contract             = b_new_fixed_bytes(b, &pos, 20);
  chain->last_block           = b_read_long(b, &pos);
  chain->nodelist_length      = (node_count = b_read_int(b, &pos));
  chain->nodelist             = _calloc(node_count, sizeof(in3_node_t));
  chain->weights              = _calloc(node_count, sizeof(in3_node_weight_t));
  chain->nodelist_upd8_params = NULL;
  memcpy(chain->weights, b->data + pos, node_count * sizeof(in3_node_weight_t));
  pos += node_count * sizeof(in3_node_weight_t);

  for (int i = 0; i < node_count; i++) {
    in3_node_t* n = chain->nodelist + i;
    n->capacity   = b_read_int(b, &pos);
    n->index      = b_read_int(b, &pos);
    n->deposit    = b_read_long(b, &pos);
    n->props      = b_read_long(b, &pos);
    n->address    = b_new_fixed_bytes(b, &pos, 20);
    n->url        = b_new_chars(b, &pos);
    BIT_CLEAR(n->attrs, ATTR_WHITELISTED);
  }

  // read verified hashes
  const unsigned int hashes = b_read_int(b, &pos);
  if (!chain->verified_hashes && hashes) chain->verified_hashes = _calloc(c->max_verified_hashes, sizeof(in3_verified_hash_t));
  if (hashes)
    memcpy(chain->verified_hashes, b->data + pos, sizeof(in3_verified_hash_t) * (min(hashes, c->max_verified_hashes)));

  b_free(b);
  return IN3_OK;
}

in3_ret_t in3_cache_store_nodelist(in3_ctx_t* ctx, in3_chain_t* chain) {
  // it is ok not to have a storage
  if (!ctx->client->cache) return IN3_OK;

  // write to bytes_buffer
  bytes_builder_t* bb = bb_new();
  bb_write_byte(bb, CACHE_VERSION);          // Version flag
  bb_write_fixed_bytes(bb, chain->contract); // 20 bytes fixed
  bb_write_long(bb, chain->last_block);
  bb_write_int(bb, chain->nodelist_length);
  bb_write_raw_bytes(bb, chain->weights, chain->nodelist_length * sizeof(in3_node_weight_t));

  for (int i = 0; i < chain->nodelist_length; i++) {
    const in3_node_t* n = chain->nodelist + i;
    bb_write_int(bb, n->capacity);
    bb_write_int(bb, n->index);
    bb_write_long(bb, n->deposit);
    bb_write_long(bb, n->props);
    bb_write_fixed_bytes(bb, n->address);
    bb_write_chars(bb, n->url, strlen(n->url));
  }

  // verified hashes
  int count = 0;
  if (chain->verified_hashes) {
    count = ctx->client->max_verified_hashes;
    for (int i = 0; i < count; i++) {
      if (!chain->verified_hashes[i].block_number) {
        count = i;
        break;
      }
    }
    bb_write_int(bb, count);
    bb_write_raw_bytes(bb, chain->verified_hashes, count * sizeof(in3_verified_hash_t));
  } else
    bb_write_int(bb, 0);

  // create key
  char key[200];
  write_cache_key(key, chain->chain_id, chain->contract->data);

  // store it and ignore return value since failing when writing cache should not stop us.
  ctx->client->cache->set_item(ctx->client->cache->cptr, key, &bb->b);

  // clear buffer
  bb_free(bb);
  return IN3_OK;
}

in3_ret_t in3_cache_update_whitelist(in3_t* c, in3_chain_t* chain) {
  // it is ok not to have a storage
  if (!c->cache || !chain->whitelist) return IN3_OK;

  in3_whitelist_t* wl = chain->whitelist;

  // define the key to use
  char key[MAX_KEYLEN];
  write_cache_key(key, chain->chain_id, wl->contract);

  // get from cache
  bytes_t* data = c->cache->get_item(c->cache->cptr, key);
  if (data) {
    size_t pos = 0;

    // version check
    if (b_read_byte(data, &pos) != CACHE_VERSION) {
      b_free(data);
      return IN3_EVERS;
    }

    // clean up old
    if (wl->addresses.data) _free(wl->addresses.data);

    // fill data
    wl->last_block         = b_read_long(data, &pos);
    uint32_t adress_length = b_read_int(data, &pos) * 20;
    wl->addresses          = bytes(_malloc(adress_length), adress_length);
    if (!wl->addresses.data)
      return IN3_ENOMEM;
    memcpy(wl->addresses.data, data->data + pos, adress_length);
    b_free(data);
  }
  return IN3_OK;
}

in3_ret_t in3_cache_store_whitelist(in3_ctx_t* ctx, in3_chain_t* chain) {
  // write to bytes_buffer
  if (!ctx->client->cache || !chain->whitelist) return IN3_OK;

  const in3_whitelist_t* wl = chain->whitelist;
  bytes_builder_t*       bb = bb_new();
  bb_write_byte(bb, CACHE_VERSION); // Version flag
  bb_write_long(bb, wl->last_block);
  bb_write_int(bb, wl->addresses.len / 20);
  bb_write_fixed_bytes(bb, &wl->addresses);

  // create key
  char key[MAX_KEYLEN];
  write_cache_key(key, chain->chain_id, wl->contract);

  // store it and ignore return value since failing when writing cache should not stop us.
  ctx->client->cache->set_item(ctx->client->cache->cptr, key, &bb->b);

  // clear buffer
  bb_free(bb);
  return IN3_OK;
}
