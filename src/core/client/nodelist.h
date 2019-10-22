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

/** @file 
 * handles nodelists.
 * 
 * */

#include "../util/mem.h"
#include "client.h"
#include "context.h"
#include <time.h>

#ifndef NODELIST_H
#define NODELIST_H

#include "../util/bitset.h"

typedef enum {
  NODE_PROP_NONE = 0,
  NODE_PROP_PROOF_NODES,
  NODE_PROP_MULTICHAIN_NODES,
  NODE_PROP_ARCHIVE_NODES,
  NODE_PROP_HTTP_NODES,
  NODE_PROP_BINARY_NODES,
  NODE_PROP_TOR_NODES,
  NODE_PROP_DEPOSIT_TIMEOUT,
} in3_node_prop_t;

/** removes all nodes and their weights from the nodelist */
void in3_nodelist_clear(in3_chain_t* chain);

/** check if the nodelist is up to date.
 * 
 * if not it will fetch a new version first (if the needs_update-flag is set).
 */
in3_ret_t in3_node_list_get(in3_ctx_t* ctx, uint64_t chain_id, bool update, in3_node_t** nodeList, int* nodeListLength, in3_node_weight_t** weights);

/**
 * filters and fills the weights on a returned linked list.
 */
node_weight_t* in3_node_list_fill_weight(in3_t* c, in3_node_t* all_nodes, in3_node_weight_t* weights, int len, _time_t now, float* total_weight, int* total_found);

/**
 * picks (based on the config) a random number of nodes and returns them as weightslist.
 */
in3_ret_t in3_node_list_pick_nodes(in3_ctx_t* ctx, node_weight_t** nodes);

static inline in3_ret_t in3_node_props_set(uint64_t* node_props, in3_node_prop_t prop) {
  if (prop <= NODE_PROP_NONE || prop >= NODE_PROP_DEPOSIT_TIMEOUT) return IN3_EINVAL;
  BIT_SET(*node_props, prop);
  return IN3_OK;
}

static inline void in3_node_props_set_deposit_timeout(uint64_t* node_props, uint32_t deposit_timeout) {
  uint64_t dp_ = deposit_timeout;
  *node_props |= BITS_LSB(dp_, 32U);
}

static inline uint32_t in3_node_props_get(const uint64_t node_props) {
  return BITS_LSB(node_props, 32U);
}

static inline uint32_t in3_node_props_get_deposit_timeout(const uint64_t node_props) {
  return BITS_MSB(node_props, 32U);
}

static inline bool in3_node_props_match(const in3_node_t* node, const in3_t* c) {
  if (in3_node_props_get(node->props) == in3_node_props_get(c->node_props) && in3_node_props_get_deposit_timeout(node->props) >= in3_node_props_get_deposit_timeout(c->node_props)) {
    return true;
  }
  return false;
}

#endif
