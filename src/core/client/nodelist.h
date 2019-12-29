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

/**
 * handles nodelists.
 * 
 * */

#include "../util/mem.h"
#include "client.h"
#include "context.h"
#include <time.h>

#ifndef NODELIST_H
#define NODELIST_H

/** removes all nodes and their weights from the nodelist */
void in3_nodelist_clear(in3_chain_t* chain);

/** updates all whitelisted flags in the nodelist */
void in3_client_run_chain_whitelisting(in3_chain_t* chain);

/** check if the nodelist is up to date.
 * 
 * if not it will fetch a new version first (if the needs_update-flag is set).
 */
in3_ret_t in3_node_list_get(in3_ctx_t* ctx, chain_id_t chain_id, bool update, in3_node_t** nodelist, int* nodelist_length, in3_node_weight_t** weights);

/**
 * filters and fills the weights on a returned linked list.
 */
node_weight_t* in3_node_list_fill_weight(in3_t* c, chain_id_t chain_id, in3_node_t* all_nodes, in3_node_weight_t* weights, int len, _time_t now, float* total_weight, int* total_found, in3_node_props_t props);

/**
 * picks (based on the config) a random number of nodes and returns them as weightslist.
 */
in3_ret_t in3_node_list_pick_nodes(in3_ctx_t* ctx, node_weight_t** nodes, int request_count, in3_node_props_t props);

/**
 * forces the client to update the nodelist
 */
in3_ret_t update_nodes(in3_t* c, in3_chain_t* chain);
// weights
void in3_ctx_free_nodes(node_weight_t* c);
int  ctx_nodes_len(node_weight_t* root);

#endif
