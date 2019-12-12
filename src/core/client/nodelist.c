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

#include "nodelist.h"
#include "../util/data.h"
#include "../util/log.h"
#include "../util/mem.h"
#include "../util/utils.h"
#include "cache.h"
#include "client.h"
#include "context.h"
#include "keys.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef __TEST__

#endif

#define DAY 24 * 2600
static void free_nodeList(in3_node_t* nodeList, int count) {
  int i;
  // clean chain..
  for (i = 0; i < count; i++) {
    if (nodeList[i].url) _free(nodeList[i].url);
    if (nodeList[i].address) b_free(nodeList[i].address);
  }
  _free(nodeList);
}

static in3_ret_t in3_client_fill_chain(in3_chain_t* chain, in3_ctx_t* ctx, d_token_t* result) {
  int       i, len;
  in3_ret_t res  = IN3_OK;
  _time_t   _now = _time(); // TODO here we might get a -1 or a unsuable number if the device does not know the current timestamp.
  uint64_t  now  = (uint64_t) max(0, _now);

  // read the nodes
  d_token_t *t, *nodes = d_get(result, K_NODES), *node = NULL;
  if (!nodes || d_type(nodes) != T_ARRAY)
    return ctx_set_error(ctx, "No Nodes in the result", IN3_EINVALDT);

  if (!(t = d_get(result, K_LAST_BLOCK_NUMBER)))
    return ctx_set_error(ctx, "LastBlockNumer is missing", IN3_EINVALDT);

  // update last blockNumber
  chain->lastBlock = d_long(t);

  // new nodelist
  in3_node_t*        newList = _calloc((len = d_len(nodes)), sizeof(in3_node_t));
  in3_node_weight_t* weights = _calloc(len, sizeof(in3_node_weight_t));

  // set new values
  for (i = 0; i < len; i++) {
    in3_node_t* n = newList + i;
    node          = node ? d_next(node) : d_get_at(nodes, i);
    if (!node) {
      res = ctx_set_error(ctx, "node missing", IN3_EINVALDT);
      break;
    }

    int old_index          = i;
    weights[i].weight      = 1;
    n->capacity            = d_get_intkd(node, K_CAPACITY, 1);
    n->index               = d_get_intkd(node, K_INDEX, i);
    n->deposit             = d_get_longk(node, K_DEPOSIT);
    n->props               = d_get_longkd(node, K_PROPS, 65535);
    n->url                 = d_get_stringk(node, K_URL);
    n->address             = d_get_byteskl(node, K_ADDRESS, 20);
    uint64_t register_time = d_get_longk(node, K_REGISTER_TIME);

    if (n->address)
      n->address = b_dup(n->address); // create a copy since the src will be freed.
    else {
      res = ctx_set_error(ctx, "missing address in nodelist", IN3_EINVALDT);
      break;
    }

    // restore the nodeweights if the address was known in the old nodeList
    if (chain->nodeListLength <= i || !b_cmp(chain->nodeList[i].address, n->address)) {
      old_index = -1;
      for (int j = 0; j < chain->nodeListLength; j++) {
        if (b_cmp(chain->nodeList[j].address, n->address)) {
          old_index = j;
          break;
        }
      }
    }
    if (old_index >= 0) memcpy(weights + i, chain->weights + old_index, sizeof(in3_node_weight_t));

    // if this is a newly registered node, we wait 24h before we use it, since this is the time where mallicous nodes may be unregistered.
    if (now && register_time + DAY > now && now > register_time)
      weights[i].blacklistedUntil = register_time + DAY;

    // clone the url since the src will be freed
    if (n->url)
      n->url = _strdupn(n->url, -1);
    else {
      res = ctx_set_error(ctx, "missing url in nodelist", IN3_EINVALDT);
      break;
    }
  }

  if (res == IN3_OK) {
    // successfull, so we can update the chain.
    free_nodeList(chain->nodeList, chain->nodeListLength);
    _free(chain->weights);
    chain->nodeList       = newList;
    chain->nodeListLength = len;
    chain->weights        = weights;
  } else {
    free_nodeList(newList, len);
    _free(weights);
  }

  return res;
}

static in3_ret_t update_nodelist(in3_t* c, in3_chain_t* chain, in3_ctx_t* parent_ctx) {
  in3_ret_t res = IN3_OK;

  // is there a useable required ctx?
  in3_ctx_t* ctx = ctx_find_required(parent_ctx, "in3_nodeList");

  if (ctx)
    switch (in3_ctx_state(ctx)) {
      case CTX_ERROR:
        return ctx_set_error(parent_ctx, "Error updating node_list", ctx_set_error(parent_ctx, ctx->error, IN3_ERPC));
      case CTX_WAITING_FOR_REQUIRED_CTX:
      case CTX_WAITING_FOR_RESPONSE:
        return IN3_WAITING;
      case CTX_SUCCESS: {

        d_token_t* r = d_get(ctx->responses[0], K_RESULT);
        if (r) {
          // we have a result....
          res = in3_client_fill_chain(chain, ctx, r);
          if (res < 0)
            return ctx_set_error(parent_ctx, "Error updating node_list", ctx_set_error(parent_ctx, ctx->error, res));
          else if (c->cacheStorage)
            in3_cache_store_nodelist(ctx, chain);
          ctx_remove_required(parent_ctx, ctx);
          return IN3_OK;
        } else
          return ctx_set_error(parent_ctx, "Error updating node_list", ctx_check_response_error(ctx, 0));
      }
    }

  in3_log_debug("update the nodelist...\n");

  // create random seed
  char seed[67];
  sprintf(seed, "0x%08x%08x%08x%08x%08x%08x%08x%08x", _rand(), _rand(), _rand(), _rand(), _rand(), _rand(), _rand(), _rand());

  // create request
  char* req = _malloc(300);
  sprintf(req, "{\"method\":\"in3_nodeList\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":[%i,\"%s\",[]]}", c->nodeLimit, seed);

  // new client
  return ctx_add_required(parent_ctx, ctx = new_ctx(c, req));
}

static in3_ret_t in3_client_fill_chain_whitelist(in3_chain_t* chain, in3_ctx_t* ctx, d_token_t* result) {
  in3_ret_t res = IN3_OK;

  // read the nodes
  d_token_t *nodes = d_get(result, K_NODES), *t = NULL;
  if (!nodes || d_type(nodes) != T_ARRAY)
    return ctx_set_error(ctx, "No Nodes in the result", IN3_EINVALDT);
  int len = d_len(nodes);

  if (!(t = d_get(result, K_LAST_BLOCK_NUMBER)))
    return ctx_set_error(ctx, "LastBlockNumer is missing", IN3_EINVALDT);

  chain->lastBlockWl = d_long(t);

  for (int j = 0; j < chain->nodeListLength; ++j)
    chain->nodeList[j].whiteListed = false;

  in3_whitelist_clear(chain);
  bytes_builder_t* bb = bb_newl(len * 20);
  bytes_t*         addr_;
  for (int i = 0; i < len; ++i) {
    addr_ = d_bytesl(d_get_at(nodes, i), 20);
    bb_write_raw_bytes(bb, addr_, 20);

    for (int j = 0; j < chain->nodeListLength; ++j)
      if (b_cmp(addr_, chain->nodeList[j].address))
        chain->nodeList[j].whiteListed = true;
  }
  chain->whiteList = bb;
  return res;
}

static in3_ret_t update_whitelist(in3_t* c, in3_chain_t* chain, in3_ctx_t* parent_ctx) {
  in3_ret_t res = IN3_OK;

  // is there a useable required ctx?
  in3_ctx_t* ctx = ctx_find_required(parent_ctx, "in3_whiteList");

  if (ctx)
    switch (in3_ctx_state(ctx)) {
      case CTX_ERROR:
        return ctx_set_error(parent_ctx, "Error updating white_list", ctx_set_error(parent_ctx, ctx->error, IN3_ERPC));
      case CTX_WAITING_FOR_REQUIRED_CTX:
      case CTX_WAITING_FOR_RESPONSE:
        return IN3_WAITING;
      case CTX_SUCCESS: {

        d_token_t* r = d_get(ctx->responses[0], K_RESULT);
        if (r) {
          // we have a result....
          res = in3_client_fill_chain_whitelist(chain, ctx, r);
          if (res < 0)
            return ctx_set_error(parent_ctx, "Error updating white_list", ctx_set_error(parent_ctx, ctx->error, res));
          else if (c->cacheStorage)
            in3_cache_store_whitelist(ctx, chain);
          ctx_remove_required(parent_ctx, ctx);
          return IN3_OK;
        } else
          return ctx_set_error(parent_ctx, "Error updating white_list", ctx_check_response_error(ctx, 0));
      }
    }

  in3_log_debug("update the whitelist...\n");

  // create request
  char* req = _malloc(300);
  sprintf(req, "{\"method\":\"in3_whiteList\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":[\"0x");

  char tmp[40] = {0};
  bytes_to_hex(chain->whiteListContract->data, 20, tmp);
  strcat(req, tmp);
  strcat(req, "\"]}");

  // new client
  return ctx_add_required(parent_ctx, ctx = new_ctx(c, req));
}

void free_ctx_nodes(node_weight_t* c) {
  node_weight_t* p = NULL;
  while (c) {
    p = c;
    c = c->next;
    _free(p);
  }
}

in3_ret_t update_nodes(in3_t* c, in3_chain_t* chain) {
  in3_ctx_t ctx;
  memset(&ctx, 0, sizeof(ctx));
  chain->needsUpdate = UPDATE_NONE;

  in3_ret_t ret = update_nodelist(c, chain, &ctx);
  if (ret == IN3_WAITING && ctx.required) {
    ret = in3_send_ctx(ctx.required);
    if (ret) return ret;
    return update_nodelist(c, chain, &ctx);
  }
  return ret;
}

node_weight_t* in3_node_list_fill_weight(in3_t* c, in3_node_t* all_nodes, in3_node_weight_t* weights,
                                         int len, _time_t now, float* total_weight, int* total_found) {
  int                i, p;
  float              s         = 0;
  in3_node_t*        nodeDef   = NULL;
  in3_node_weight_t* weightDef = NULL;
  node_weight_t*     prev      = NULL;
  node_weight_t*     w         = NULL;
  node_weight_t*     first     = NULL;

  for (i = 0, p = 0; i < len; i++) {
    nodeDef = all_nodes + i;
    if (nodeDef->deposit < c->minDeposit) continue;
    weightDef = weights + i;
    if (weightDef->blacklistedUntil > (uint64_t) now) continue;
    w = _malloc(sizeof(node_weight_t));
    if (!first) first = w;
    w->node   = nodeDef;
    w->weight = weightDef;
    w->next   = NULL;
    w->s      = s;
    w->w      = weightDef->weight * nodeDef->capacity * (500 / (weightDef->response_count ? (weightDef->total_response_time / weightDef->response_count) : 500));
    s += w->w;
    p++;
    if (prev) prev->next = w;
    prev = w;
  }
  *total_weight = s;
  *total_found  = p;
  return first;
}

in3_ret_t in3_node_list_get(in3_ctx_t* ctx, uint64_t chain_id, bool update, in3_node_t** nodeList, int* nodeListLength, in3_node_weight_t** weights) {
  int          i;
  in3_ret_t    res   = IN3_EFIND;
  in3_chain_t* chain = NULL;
  in3_t*       c     = ctx->client;
  for (i = 0; i < c->chainsCount; i++) {
    chain = c->chains + i;
    if (chain->chainId == chain_id) {
      if ((chain->needsUpdate & UPDATE_NODELIST) || update || ctx_find_required(ctx, "in3_nodeList")) {
        chain->needsUpdate &= ~UPDATE_NODELIST;
        // now update the nodeList
        res = update_nodelist(c, chain, ctx);
        if (res < 0) break;
      }
      if (chain->whiteListContract && ((chain->needsUpdate & UPDATE_WHITELIST) || ctx_find_required(ctx, "in3_whiteList"))) {
        chain->needsUpdate &= ~UPDATE_NODELIST;
        res = update_whitelist(c, chain, ctx);
        if (res < 0) break;
      }

      *nodeListLength = chain->nodeListLength;
      *nodeList       = chain->nodeList;
      *weights        = chain->weights;
      return 0;
    }
  }

  return res;
}

in3_ret_t in3_node_list_pick_nodes(in3_ctx_t* ctx, node_weight_t** nodes, int request_count) {

  // get all nodes from the nodelist
  _time_t            now       = _time();
  in3_node_t*        all_nodes = NULL;
  in3_node_weight_t* weights   = NULL;
  float              total_weight;
  int                all_nodes_len, total_found, i, l;

  in3_ret_t res = in3_node_list_get(ctx, ctx->client->chainId, false, &all_nodes, &all_nodes_len, &weights);
  if (res < 0)
    return ctx_set_error(ctx, "could not find the chain", res);

  // filter out nodes
  node_weight_t* found = in3_node_list_fill_weight(ctx->client, all_nodes, weights, all_nodes_len, now, &total_weight, &total_found);

  if (total_found == 0) {
    // no node available, so we should check if we can retry some blacklisted
    int blacklisted = 0;
    for (i = 0; i < all_nodes_len; i++) {
      if (weights[i].blacklistedUntil > (uint64_t) now) blacklisted++;
    }

    // if morethan 50% of the nodes are blacklisted, we remove the mark and try again
    if (blacklisted > all_nodes_len / 2) {
      for (i = 0; i < all_nodes_len; i++)
        weights[i].blacklistedUntil = 0;
      found = in3_node_list_fill_weight(ctx->client, all_nodes, weights, all_nodes_len, now, &total_weight, &total_found);
    }

    if (total_found == 0)
      return ctx_set_error(ctx, "No nodes found that match the criteria", IN3_EFIND);
  }

  l = total_found < request_count ? total_found : request_count;
  if (total_found == l) {
    *nodes = found;
    return IN3_OK;
  }

  node_weight_t* last  = NULL;
  node_weight_t* first = NULL;
  node_weight_t* wn    = NULL;
  float          r;

  int            added = 0;
  node_weight_t* w     = NULL;

  // we want ot make sure this loop is run only max 10xthe number of requested nodes
  for (i = 0; added < l && i < l * 10; i++) {
    // pick a random number
    r = total_weight * ((float) (_rand() % 10000)) / 10000.0;

    // find the first node matching it.
    w = found;
    while (w) {
      if (w->s <= r && w->s + w->w >= r) break;
      w = w->next;
    }

    if (w) {
      // check if we already added it,
      wn = first;
      while (wn) {
        if (wn->node == w->node) break;
        wn = wn->next;
      }

      if (!wn) {
        added++;
        wn         = _calloc(1, sizeof(node_weight_t));
        wn->s      = w->s;
        wn->w      = w->w;
        wn->weight = w->weight;
        wn->node   = w->node;

        if (last) last->next = wn;
        if (!first) first = wn;
      }
    }
  }

  *nodes = first;
  free_ctx_nodes(found);

  // select them based on random
  return res;
}

/** removes all nodes and their weights from the nodelist */
void in3_nodelist_clear(in3_chain_t* chain) {
  int i;
  for (i = 0; i < chain->nodeListLength; i++) {
    if (chain->nodeList[i].url)
      _free(chain->nodeList[i].url);
    if (chain->nodeList[i].address)
      b_free(chain->nodeList[i].address);
  }
  _free(chain->nodeList);
  _free(chain->weights);
}

void in3_whitelist_clear(in3_chain_t* chain) {
  bb_free(chain->whiteList);
}
