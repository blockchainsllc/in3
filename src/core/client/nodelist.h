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

/** removes all nodes and their weights from the nodelist */
void in3_nodelist_clear(in3_chain_t* chain);

/** check if the nodelist is up to date.
 * 
 * if not it will fetch a new version first (if the needs_update-flag is set).
 */
in3_error_t in3_node_list_get(in3_ctx_t* ctx, uint64_t chain_id, bool update, in3_node_t** nodeList, int* nodeListLength, in3_node_weight_t** weights);

/**
 * filters and fills the weights on a returned linked list.
 */
node_weight_t* in3_node_list_fill_weight(in3_t* c, in3_node_t* all_nodes, in3_node_weight_t* weights, int len, _time_t now, float* total_weight, int* total_found);

/**
 * picks (based on the config) a random number of nodes and returns them as weightslist.
 */
in3_error_t in3_node_list_pick_nodes(in3_ctx_t* ctx, node_weight_t** nodes);

#endif
