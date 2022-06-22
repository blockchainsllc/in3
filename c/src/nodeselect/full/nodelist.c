/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
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

#include "nodelist.h"
#include "../../core/client/keys.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/bitset.h"
#include "../../core/util/data.h"
#include "../../core/util/debug.h"
#include "cache.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define DAY              (24 * 3600)
#define DIFFTIME(t1, t0) (double) ((t1) > (t0) ? (t1) - (t0) : 0)
#define BLACKLISTTIME    DAY
#define BLACKLISTWEIGHT  (7 * DAY)

NONULL static void free_nodeList(in3_node_t* nodelist, unsigned int count) {
  // clean data..
  for (unsigned int i = 0; i < count; i++)
    if (nodelist[i].url) _free(nodelist[i].url);
  _free(nodelist);
}

NONULL static bool postpone_update(const in3_nodeselect_def_t* data) {
  if (data->nodelist_upd8_params && data->nodelist_upd8_params->timestamp)
    if (DIFFTIME(data->nodelist_upd8_params->timestamp, in3_time(NULL)) > 0)
      return true;
  return false;
}

NONULL static inline bool nodelist_exp_last_block_neq(in3_nodeselect_def_t* data, uint64_t exp_last_block) {
  return (data->nodelist_upd8_params != NULL && data->nodelist_upd8_params->exp_last_block != exp_last_block);
}

NONULL static in3_ret_t fill_chain(in3_nodeselect_def_t* data, in3_req_t* ctx, d_token_t* result) {
  in3_ret_t      res  = IN3_OK;
  uint64_t       _now = in3_time(NULL);
  const uint64_t now  = (uint64_t) _now;

  // read the nodes
  d_token_t *  nodes = d_get(result, K_NODES), *t;
  unsigned int len   = d_len(nodes);

  if (!nodes || d_type(nodes) != T_ARRAY)
    return req_set_error(ctx, "No Nodes in the result", IN3_EINVALDT);

  if (!(t = d_get(result, K_LAST_BLOCK_NUMBER)))
    return req_set_error(ctx, "LastBlockNumer is missing", IN3_EINVALDT);

  // update last blockNumber
  const uint64_t last_block = d_long(t);
  if (last_block > data->last_block)
    data->last_block = last_block;
  else
    return IN3_OK; // if the last block is older than the current one we don't update, but simply ignore it.

  // new nodelist
  in3_node_t*        newList = _calloc(len, sizeof(in3_node_t));
  in3_node_weight_t* weights = _calloc(len, sizeof(in3_node_weight_t));
  d_token_t*         node    = NULL;

  // set new values
  for (unsigned int i = 0; i < len; i++) {
    in3_node_t* n = newList + i;
    node          = node ? d_next(node) : d_get_at(nodes, i);
    if (!node) {
      res = req_set_error(ctx, "node missing", IN3_EINVALDT);
      break;
    }

    int old_index     = (int) i;
    n->capacity       = d_get_intd(node, K_CAPACITY, 1);
    n->index          = d_get_intd(node, K_INDEX, i);
    n->deposit        = d_get_long(node, K_DEPOSIT);
    n->props          = d_get_longd(node, K_PROPS, 65535);
    n->url            = d_get_string(node, K_URL);
    bytes_t adr_bytes = d_get_byteskl(node, K_ADDRESS, 20);
    if (adr_bytes.data && adr_bytes.len == 20)
      memcpy(n->address, adr_bytes.data, 20);
    else {
      res = req_set_error(ctx, "missing address in nodelist", IN3_EINVALDT);
      break;
    }
    BIT_CLEAR(n->attrs, ATTR_BOOT_NODE); // nodes are considered boot nodes only until first nodeList update succeeds

    if ((ctx->client->flags & FLAGS_BOOT_WEIGHTS) && (t = d_get(node, K_PERFORMANCE))) {
      weights[i].blacklisted_until   = d_get_long(t, K_LAST_FAILED) / 1000 + (24 * 3600);
      weights[i].response_count      = d_get_int(t, K_COUNT);
      weights[i].total_response_time = d_get_int(t, K_TOTAL);
    }

    // restore the nodeweights if the address was known in the old nodeList
    if (data->nodelist_length <= i || memcmp(data->nodelist[i].address, n->address, 20) != 0) {
      old_index = -1;
      for (unsigned int j = 0; j < data->nodelist_length; j++) {
        if (memcmp(data->nodelist[j].address, n->address, 20) == 0) {
          old_index = (int) j;
          break;
        }
      }
    }
    if (old_index >= 0) memcpy(weights + i, data->weights + old_index, sizeof(in3_node_weight_t));

    // if this is a newly registered node, we wait 24h before we use it, since this is the time where mallicous nodes may be unregistered.
    const uint64_t register_time = d_get_long(node, K_REGISTER_TIME);
    if (now && register_time + DAY > now && now > register_time)
      weights[i].blacklisted_until = register_time + DAY;

    // clone the url since the src will be freed
    if (n->url)
      n->url = _strdupn(n->url, -1);
    else {
      res = req_set_error(ctx, "missing url in nodelist", IN3_EINVALDT);
      break;
    }
  }

  if (res == IN3_OK) {
    // successfull, so we can update the data.
    free_nodeList(data->nodelist, data->nodelist_length);
    _free(data->weights);
    data->nodelist        = newList;
    data->nodelist_length = len;
    data->weights         = weights;
  }
  else {
    free_nodeList(newList, len);
    _free(weights);
  }

  data->dirty = true;
  return res;
}

#ifdef NODESELECT_DEF_WL
NONULL void in3_client_run_chain_whitelisting(in3_nodeselect_def_t* data) {
  if (!data->whitelist)
    return;

  for (unsigned int j = 0; j < data->nodelist_length; ++j)
    BIT_CLEAR(data->nodelist[j].attrs, ATTR_WHITELISTED);

  for (size_t i = 0; i < data->whitelist->addresses.len / 20; i += 20) {
    for (unsigned int j = 0; j < data->nodelist_length; ++j)
      if (!memcmp(data->whitelist->addresses.data + i, data->nodelist[j].address, 20))
        BIT_SET(data->nodelist[j].attrs, ATTR_WHITELISTED);
  }
}

NONULL static in3_ret_t in3_client_fill_chain_whitelist(in3_nodeselect_def_t* data, in3_req_t* ctx, d_token_t* result) {
  in3_whitelist_t* wl    = data->whitelist;
  int              i     = 0;
  d_token_t *      nodes = d_get(result, K_NODES), *t = NULL;

  if (!wl) return req_set_error(ctx, "No whitelist set", IN3_EINVALDT);
  if (!nodes || d_type(nodes) != T_ARRAY) return req_set_error(ctx, "No Nodes in the result", IN3_EINVALDT);

  const int len = d_len(nodes);
  if (!(t = d_get(result, K_LAST_BLOCK_NUMBER)))
    return req_set_error(ctx, "LastBlockNumer is missing", IN3_EINVALDT);

  // update last blockNumber
  const uint64_t last_block = d_long(t);
  if (last_block > wl->last_block)
    wl->last_block = last_block;
  else
    return IN3_OK; // if the last block is older than the current one we don't update, but simply ignore it.

  // now update the addresses
  if (wl->addresses.data) _free(wl->addresses.data);
  wl->addresses = bytes(_malloc(len * 20), len * 20);

  for (d_iterator_t iter = d_iter(nodes); iter.left; d_iter_next(&iter), i += 20)
    d_bytes_to(iter.token, wl->addresses.data + i, 20);

  in3_client_run_chain_whitelisting(data);
  return IN3_OK;
}
#endif

NONULL static in3_ret_t update_nodelist(in3_t* c, in3_nodeselect_def_t* data, in3_req_t* parent_ctx) {
  // is there a useable required ctx?
  in3_req_t* ctx = req_find_required(parent_ctx, "in3_nodeList", NULL);

  if (ctx) {
    if (in3_req_state(ctx) == REQ_ERROR || (in3_req_state(ctx) == REQ_SUCCESS && !d_get(ctx->responses[0], K_RESULT))) {
      // blacklist node that gave us an error response for nodelist (if not first update)
      // and clear nodelist params
      if (nodelist_not_first_upd8(data))
        blacklist_node_addr(data, data->nodelist_upd8_params->node, BLACKLISTTIME);
      _free(data->nodelist_upd8_params);
      data->nodelist_upd8_params = NULL;

      // if first update return error otherwise return IN3_OK, this is because first update is
      // always from a boot node which is presumed to be trusted
      return nodelist_first_upd8(data)
                 ? req_set_error(parent_ctx, "Error updating node_list", req_set_error(parent_ctx, ctx->error, IN3_ERPC))
                 : IN3_OK;
    }

    switch (in3_req_state(ctx)) {
      case REQ_ERROR: return IN3_OK; /* already handled before*/
      case REQ_WAITING_FOR_RESPONSE:
      case REQ_WAITING_TO_SEND:
        return IN3_WAITING;
      case REQ_SUCCESS: {
        d_token_t* r = d_get(ctx->responses[0], K_RESULT);
        // if the `lastBlockNumber` != `exp_last_block`, we can be certain that `data->nodelist_upd8_params->node` lied to us
        // about the nodelist update, so we blacklist it for an hour
        if (nodelist_exp_last_block_neq(data, d_get_long(r, K_LAST_BLOCK_NUMBER)))
          blacklist_node_addr(data, data->nodelist_upd8_params->node, BLACKLISTTIME);
        _free(data->nodelist_upd8_params);
        data->nodelist_upd8_params = NULL;

        const in3_ret_t res = fill_chain(data, ctx, r);
        if (res < 0)
          return req_set_error(parent_ctx, "Error updating node_list", req_set_error(parent_ctx, ctx->error, res));
        in3_cache_store_nodelist(ctx->client, data);
        req_remove_required(parent_ctx, ctx, true);

#ifdef NODESELECT_DEF_WL
        in3_client_run_chain_whitelisting(data);
#endif
        return IN3_OK;
      }
    }
  }

  in3_log_debug("update the nodelist...\n");

  // create random seed
  char seed[67] = {'0', 'x'};
  for (int i = 0, j = 2; i < 8; ++i, j += 8)
    sprintf(seed + j, "%08x", in3_rand(NULL) % 0xFFFFFFFF);

  sb_t* in3_sec = sb_new("{");
  if (nodelist_not_first_upd8(data)) {
    bytes_t addr_ = (bytes_t){.data = data->nodelist_upd8_params->node, .len = 20};
    sb_add_bytes(in3_sec, "\"dataNodes\":", &addr_, 1, true);
  }

  // create request
  in3_nodeselect_config_t* w   = in3_get_nodelist(c);
  char*                    req = _malloc(350);
  sprintf(req, "{\"method\":\"in3_nodeList\",\"jsonrpc\":\"2.0\",\"params\":[%i,\"%s\",[]%s],\"in3\":%s}",
          w->node_limit, seed,
          ((c->flags & FLAGS_BOOT_WEIGHTS) && nodelist_first_upd8(data)) ? ",true" : "",
          sb_add_char(in3_sec, '}')->data);
  sb_free(in3_sec);

  // new client
  return req_add_required(parent_ctx, req_new(c, req));
}

#ifdef NODESELECT_DEF_WL
NONULL static in3_ret_t update_whitelist(in3_t* c, in3_nodeselect_def_t* data, in3_req_t* parent_ctx) {
  // is there a useable required ctx?
  in3_req_t* ctx = req_find_required(parent_ctx, "in3_whiteList", NULL);

  if (ctx)
    switch (in3_req_state(ctx)) {
      case REQ_ERROR:
        return req_set_error(parent_ctx, "Error updating white_list", req_set_error(parent_ctx, ctx->error, IN3_ERPC));
      case REQ_WAITING_FOR_RESPONSE:
      case REQ_WAITING_TO_SEND:
        return IN3_WAITING;
      case REQ_SUCCESS: {
        d_token_t* result = d_get(ctx->responses[0], K_RESULT);
        if (result) {
          // we have a result....
          const in3_ret_t res = in3_client_fill_chain_whitelist(data, ctx, result);
          if (res < 0)
            return req_set_error(parent_ctx, "Error updating white_list", req_set_error(parent_ctx, ctx->error, res));
          in3_cache_store_whitelist(ctx->client, data);
          in3_client_run_chain_whitelisting(data);
          req_remove_required(parent_ctx, ctx, true);
          return IN3_OK;
        }
        else
          return req_set_error(parent_ctx, "Error updating white_list", req_check_response_error(ctx, 0));
      }
    }

  in3_log_debug("update the whitelist...\n");

  // create request
  char* req     = _malloc(300);
  char  tmp[41] = {0};
  bytes_to_hex(data->whitelist->contract, 20, tmp);
  sprintf(req, "{\"method\":\"in3_whiteList\",\"jsonrpc\":\"2.0\",\"params\":[\"0x%s\"]}", tmp);

  // new client
  return req_add_required(parent_ctx, req_new(c, req));
}
#endif

in3_ret_t update_nodes(in3_t* c, in3_nodeselect_def_t* data) {
  in3_req_t* ctx          = _calloc(1, sizeof(in3_req_t));
  ctx->verification_state = IN3_EIGNORE;
  ctx->error              = _calloc(1, 1);
  ctx->client             = c;
  if (data->nodelist_upd8_params) {
    _free(data->nodelist_upd8_params);
    data->nodelist_upd8_params = NULL;
  }

  in3_ret_t ret = update_nodelist(c, data, ctx);
  if (ret == IN3_WAITING && ctx->required) {
    ret = in3_send_req(ctx->required);
    if (!ret) ret = update_nodelist(c, data, ctx);
  }

  req_free(ctx);
  return ret;
}

IN3_EXPORT_TEST bool in3_node_props_match(const in3_node_props_t np_config, const in3_node_props_t np) {
  if (((np_config & np) & 0xFFFFFFFF) != (np_config & 0XFFFFFFFF)) return false;
  uint32_t min_blk_ht_conf = in3_node_props_get(np_config, NODE_PROP_MIN_BLOCK_HEIGHT);
  uint32_t min_blk_ht      = in3_node_props_get(np, NODE_PROP_MIN_BLOCK_HEIGHT);
  return min_blk_ht_conf ? (min_blk_ht <= min_blk_ht_conf) : true;
}

uint32_t in3_node_calculate_weight(in3_node_weight_t* n, uint32_t capa, uint64_t now) {
  // calculate the averge response time
  const uint32_t avg = (n->response_count > 4 && n->total_response_time)
                           ? (n->total_response_time / n->response_count)
                           : (10000 / (max(capa, 100) + 100));

  // and the weights based factore
  const uint32_t blacklist_factor = ((now - n->blacklisted_until) < BLACKLISTWEIGHT)
                                        ? ((now - n->blacklisted_until) * 100 / (BLACKLISTWEIGHT))
                                        : 100;
  return (0xFFFF / avg) * blacklist_factor / 100;
}

NONULL static char* to_http_url(char* src_url) {
  const size_t l = strlen(src_url);
  if (strncmp(src_url, "https://", 8) == 0) {
    char* url = _malloc(l);
    strcpy(url, src_url + 1);
    url[0] = 'h';
    url[2] = 't';
    url[3] = 'p';
    return url;
  }
  return _strdupn(src_url, l);
}

static bool in_address_list(bytes_t* filter, address_t adr) {
  if (filter->len == 0) return true;
  for (unsigned int i = 0; i < filter->len; i += 20) {
    if (memcmp(filter->data + i, adr, 20) == 0) return true;
  }
  return false;
}

node_match_t* in3_node_list_fill_weight(in3_t* c, in3_nodeselect_config_t* w, in3_nodeselect_def_t* data, in3_node_t* all_nodes, in3_node_weight_t* weights,
                                        unsigned int len, uint64_t now, uint32_t* total_weight, unsigned int* total_found,
                                        const in3_node_filter_t* filter, bytes_t* pre_filter) {

  int                found      = 0;
  uint32_t           weight_sum = 0;
  in3_node_t*        node_def   = NULL;
  in3_node_weight_t* weight_def = NULL;
  node_match_t*      prev       = NULL;
  node_match_t*      current    = NULL;
  node_match_t*      first      = NULL;
  *total_found                  = 0;

  for (unsigned int i = 0; i < len; i++) {
    node_def   = all_nodes + i;
    weight_def = weights + i;

    if (pre_filter && !in_address_list(pre_filter, node_def->address)) continue;

    if (filter && filter->nodes) {
      bool in_filter_nodes = false;
      for_children_of(it, filter->nodes) {
        if (memcmp(d_bytesl(it.token, 20).data, node_def->address, 20) == 0) {
          in_filter_nodes = true;
          break;
        }
      }
      if (!in_filter_nodes)
        continue;
    }

    if (filter && filter->exclusions) {
      bool is_excluded = false;
      for (const node_match_t* exc = filter->exclusions; exc; exc = exc->next) {
        if (memcmp(exc->address, node_def->address, 20) == 0) {
          is_excluded = true;
          break;
        }
      }
      if (is_excluded)
        continue;
    }

    if (weight_def->blacklisted_until > (uint64_t) now) continue;
    if (BIT_CHECK(node_def->attrs, ATTR_BOOT_NODE)) goto SKIP_FILTERING;

#ifdef NODESELECT_DEF_WL
    if (data->whitelist && !BIT_CHECK(node_def->attrs, ATTR_WHITELISTED)) continue;
#else
    UNUSED_VAR(data);
#endif

    if (node_def->deposit < w->min_deposit) continue;
    if (filter && !in3_node_props_match(filter->props, node_def->props)) continue;

  SKIP_FILTERING:
    current = _malloc(sizeof(node_match_t));
    if (!first) first = current;
    current->index    = i;
    node_def->blocked = false;
    current->next     = NULL;
    current->s        = weight_sum;
    current->w        = in3_node_calculate_weight(weight_def, node_def->capacity, now);
    current->url      = (c->flags & FLAGS_HTTP) ? to_http_url(node_def->url) : _strdupn(node_def->url, -1);
    memcpy(current->address, node_def->address, 20);
    weight_sum += current->w;
    found++;
    if (prev) prev->next = current;
    prev = current;
  }
  *total_weight = weight_sum;
  *total_found  = found;
  return first;
}

static bool update_in_progress(const in3_req_t* ctx) {
  return req_is_method(ctx, "in3_nodeList");
}

in3_ret_t in3_node_list_get(in3_req_t* ctx, in3_nodeselect_def_t* data, bool update, in3_node_t** nodelist, unsigned int* nodelist_length, in3_node_weight_t** weights) {
  in3_ret_t res;

  // do we need to update the nodelist?
  if (data->nodelist_upd8_params || update || req_find_required(ctx, "in3_nodeList", NULL)) {
    // skip update if update has been postponed or there's already one in progress
    if (postpone_update(data) || update_in_progress(ctx))
      goto SKIP_UPDATE;

    // now update the nodeList
    res = update_nodelist(ctx->client, data, ctx);
    if (res < 0) return res;
  }

SKIP_UPDATE:

#ifdef NODESELECT_DEF_WL
  // do we need to update the whitelist?
  if (data->whitelist                                                                               // only if we have a whitelist
      && (data->whitelist->needs_update || update || req_find_required(ctx, "in3_whiteList", NULL)) // which has the needs_update-flag (or forced) or we have already sent the request and are now picking up the result
      && !memiszero(data->whitelist->contract, 20)) {                                               // and we need to have a contract set, zero-contract = manual whitelist, which will not be updated.
    data->whitelist->needs_update = false;
    // now update the whiteList
    res = update_whitelist(ctx->client, data, ctx);
    if (res < 0) return res;
  }
#endif

  // now update the results
  *nodelist_length = data->nodelist_length;
  *nodelist        = data->nodelist;
  *weights         = data->weights;
  return IN3_OK;
}

in3_ret_t in3_node_list_pick_nodes(in3_req_t* ctx, in3_nodeselect_config_t* w, in3_nodeselect_def_t* data, node_match_t** nodes, unsigned int request_count, const in3_node_filter_t* filter) {
  // get all nodes from the nodelist
  uint64_t           now       = in3_time(NULL);
  in3_node_t*        all_nodes = NULL;
  in3_node_weight_t* weights   = NULL;
  uint32_t           total_weight;
  unsigned int       all_nodes_len, total_found;

  in3_ret_t res = in3_node_list_get(ctx, data, false, &all_nodes, &all_nodes_len, &weights);
  if (res < 0)
    return req_set_error(ctx, "could not find the data", res);

  // filter out nodes
  node_match_t* found = in3_node_list_fill_weight(
      ctx->client, w, data, all_nodes, weights, all_nodes_len,
      now, &total_weight, &total_found, filter, data->pre_address_filter);

  if (total_found == 0) {
    // no node available, so we should check if we can retry some blacklisted
    unsigned int blacklisted = 0;
    for (unsigned int i = 0; i < all_nodes_len; i++) {
      if (weights[i].blacklisted_until > (uint64_t) now) blacklisted++;
    }

    // if morethan 50% of the nodes are blacklisted, we remove the mark and try again
    if (blacklisted > all_nodes_len / 2 || data->pre_address_filter) {
      for (unsigned int i = 0; i < all_nodes_len; i++)
        weights[i].blacklisted_until = 0;
      found = in3_node_list_fill_weight(ctx->client, w, data, all_nodes, weights, all_nodes_len, now, &total_weight, &total_found, filter, NULL);
    }

    if (total_found == 0)
      return req_set_error(ctx, "No nodes found that match the criteria", IN3_EFIND);
  }

  unsigned int filled_len = total_found < request_count ? total_found : request_count;
  if (total_found == filled_len) {
    *nodes = found;
    return IN3_OK;
  }

  uint32_t      r;
  unsigned int  added   = 0;
  node_match_t* last    = NULL;
  node_match_t* first   = NULL;
  node_match_t* current = NULL;

  // we want to make sure this loop is run only max 10xthe number of requested nodes
  for (unsigned int i = 0; added < filled_len && i < filled_len * 10; i++) {
    // pick a random number
    r = total_weight ? (in3_rand(NULL) % total_weight) : 0;

    // find the first node matching it.
    current = found;
    while (current) {
      if (current->s <= r && current->s + current->w >= r) break;
      current = current->next;
    }

    if (current) {
      added++;

      if (!first) first = current;
      if (last) {
        last->next = current;
        last       = last->next;
      }
      else
        last = first;

      // remove current from `found` list
      for (node_match_t** tmp = &found; *tmp; tmp = &(*tmp)->next) {
        if ((*tmp)->index == current->index) {
          total_weight -= current->w;
          *tmp = (*tmp)->next;

          while (*tmp) {
            (*tmp)->s -= current->w;
            tmp = &(*tmp)->next;
          }
          current->next = NULL;
          break;
        }
      }
    }
  }

  *nodes = first;
  if (found) in3_req_free_nodes(found);

  // select them based on random
  return res;
}

/** removes all nodes and their weights from the nodelist */
void in3_nodelist_clear(in3_nodeselect_def_t* data) {
  for (unsigned int i = 0; i < data->nodelist_length; i++) {
    if (data->nodelist[i].url) _free(data->nodelist[i].url);
  }
  _free(data->nodelist);
  _free(data->weights);
  data->dirty = true;
}

#ifdef NODESELECT_DEF_WL
void in3_whitelist_clear(in3_whitelist_t* wl) {
  if (!wl) return;
  if (wl->addresses.data) _free(wl->addresses.data);
  _free(wl);
}
#endif

void in3_node_props_set(in3_node_props_t* node_props, in3_node_props_type_t type, uint8_t value) {
  if (type == NODE_PROP_MIN_BLOCK_HEIGHT) {
    const uint64_t dp_ = value;
    *node_props        = (*node_props & 0xFFFFFFFF) | (dp_ << 32U);
  }
  else {
    (value != 0) ? ((*node_props) |= type) : ((*node_props) &= ~type);
  }
}
