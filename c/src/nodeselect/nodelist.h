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

/**
 * handles nodelists.
 * 
 * */

#include "../core/client/client.h"
#include "../core/client/context.h"
#include "../core/util/mem.h"
#include <time.h>

#ifndef NODELIST_H
#define NODELIST_H

#define NODE_FILTER_INIT \
  (in3_node_filter_t) { .props = 0, .nodes = NULL }

typedef struct {
  in3_node_props_t props;
  d_token_t*       nodes;
} in3_node_filter_t;

typedef struct {
  bool               dirty;           /**< indicates whether the nodelist has been modified after last read from cache */
  uint16_t           avg_block_time;  /**< average block time (seconds) for this data (calculated internally) */
  unsigned int       nodelist_length; /**< number of nodes in the nodeList */
  uint64_t           last_block;      /**< last blocknumber the nodeList was updated, which is used to detect changed in the nodelist*/
  in3_node_t*        nodelist;        /**< array of nodes */
  in3_node_weight_t* weights;         /**< stats and weights recorded for each node */
  bytes_t**          init_addresses;  /**< array of addresses of nodes that should always part of the nodeList */
  in3_whitelist_t*   whitelist;       /**< if set the whitelist of the addresses. */
  struct {
    uint64_t  exp_last_block; /**< the last_block when the nodelist last changed reported by this node */
    uint64_t  timestamp;      /**< approx. time when nodelist must be updated (i.e. when reported last_block will be considered final) */
    address_t node;           /**< node that reported the last_block which necessitated a nodeList update */
  } * nodelist_upd8_params;
} in3_nodeselect_def_t;

/** removes all nodes and their weights from the nodelist */
NONULL void in3_nodelist_clear(in3_nodeselect_def_t* data);

/** removes all nodes and their weights from the nodelist */
NONULL void in3_whitelist_clear(in3_whitelist_t* data);

/** updates all whitelisted flags in the nodelist */
NONULL void in3_client_run_chain_whitelisting(in3_nodeselect_def_t* data);

/** check if the nodelist is up to date.
 * 
 * if not it will fetch a new version first (if the needs_update-flag is set).
 */
NONULL in3_ret_t in3_node_list_get(in3_ctx_t* ctx, chain_id_t chain_id, bool update, in3_node_t** nodelist, int* nodelist_length, in3_node_weight_t** weights);

/**
 * filters and fills the weights on a returned linked list.
 */
NONULL node_match_t* in3_node_list_fill_weight(in3_t* c, chain_id_t chain_id, in3_node_t* all_nodes, in3_node_weight_t* weights, int len, uint64_t now, uint32_t* total_weight, int* total_found, in3_node_filter_t filter);

/**
 * calculates the weight for a node.
 */
NONULL uint32_t in3_node_calculate_weight(in3_node_weight_t* n, uint32_t capa, uint64_t now);
/**
 * picks (based on the config) a random number of nodes and returns them as weightslist.
 */
NONULL in3_ret_t in3_node_list_pick_nodes(in3_ctx_t* ctx, node_match_t** nodes, int request_count, in3_node_filter_t filter);

/**
 * forces the client to update the nodelist
 */
in3_ret_t update_nodes(in3_t* c, in3_nodeselect_def_t* data);

NONULL static inline bool nodelist_first_upd8(const in3_nodeselect_def_t* data) {
  return (data->nodelist_upd8_params != NULL && data->nodelist_upd8_params->exp_last_block == 0);
}

NONULL static inline bool nodelist_not_first_upd8(const in3_nodeselect_def_t* data) {
  return (data->nodelist_upd8_params != NULL && data->nodelist_upd8_params->exp_last_block != 0);
}

NONULL static inline void blacklist_node_addr(in3_nodeselect_def_t* data, const address_t node_addr, uint64_t secs_from_now) {
  for (unsigned int i = 0; i < data->nodelist_length; ++i) {
    if (!memcmp(data->nodelist[i].address, node_addr, 20)) {
      uint64_t blacklisted_until_ = in3_time(NULL) + secs_from_now;
      if (data->weights[i].blacklisted_until != blacklisted_until_)
        data->dirty = true;
      data->weights[i].blacklisted_until = blacklisted_until_;
    }
  }
}

#endif
