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
#include "../core/util/log.h"
#include "../core/util/mem.h"
#include <time.h>
#ifdef THREADSAFE
#if defined(_MSC_VER) || defined(__MINGW32__)
#include <windows.h>
#define MUTEX_INIT(mutex) mutex = CreateMutex(NULL, FALSE, NULL);
#define MUTEX_LOCK(mutex, re)                                                                       \
  {                                                                                                 \
    if ((++re) == 1 && WaitForSingleObject(mutex, INFINITE) == WAIT_ABANDONED) return IN3_EUNKNOWN; \
  }
#define MUTEX_UNLOCK(mutex, re) \
  if ((--re) == 0 && !ReleaseMutex(mutex)) return IN3_EUNKNOWN;
#define MUTEX_FREE(mutex) CloseHandle(mutex);
#else
#include <pthread.h>
#define MUTEX_INIT(mutex)                   \
  if (pthread_mutex_init(&(mutex), NULL)) { \
    _free(data);                            \
    data = NULL;                            \
  }
#define MUTEX_LOCK(mutex, re) \
  if ((++re) == 1) pthread_mutex_lock(&(mutex));
#define MUTEX_UNLOCK(mutex, re) \
  if ((--re) == 0) pthread_mutex_unlock(&(mutex));
#define MUTEX_FREE(mutex) pthread_mutex_destroy(&(mutex));
#endif
#endif

#ifndef NODELIST_H
#define NODELIST_H

#define NODE_FILTER_INIT \
  (in3_node_filter_t) { .props = 0, .nodes = NULL }

typedef struct {
  in3_node_props_t props;
  d_token_t*       nodes;
  node_match_t*    exclusions;
} in3_node_filter_t;

typedef struct node_offline_ {
  in3_node_t*           offline;
  address_t             reporter;
  struct node_offline_* next;
} node_offline_t;

typedef struct in3_nodeselect_def {
  bool               dirty;           /**< indicates whether the nodelist has been modified after last read from cache */
  uint16_t           avg_block_time;  /**< average block time (seconds) for this data (calculated internally) */
  unsigned int       nodelist_length; /**< number of nodes in the nodeList */
  uint64_t           last_block;      /**< last blocknumber the nodeList was updated, which is used to detect changed in the nodelist*/
  address_t          contract;        /**< the address of the registry contract */
  bytes32_t          registry_id;     /**< the identifier of the registry */
  in3_node_t*        nodelist;        /**< array of nodes */
  in3_node_weight_t* weights;         /**< stats and weights recorded for each node */
  bytes_t**          init_addresses;  /**< array of addresses of nodes that should always part of the nodeList */
  node_offline_t*    offlines;        /**< linked-list of offline nodes */

#ifdef NODESELECT_DEF_WL
  in3_whitelist_t* whitelist; /**< if set the whitelist of the addresses. */
#endif

  struct {
    uint64_t  exp_last_block; /**< the last_block when the nodelist last changed reported by this node */
    uint64_t  timestamp;      /**< approx. time when nodelist must be updated (i.e. when reported last_block will be considered final) */
    address_t node;           /**< node that reported the last_block which necessitated a nodeList update */
  } * nodelist_upd8_params;

  chain_id_t                 chain_id;    /**< the chain_id of the data */
  struct in3_nodeselect_def* next;        /**< the next in the linked list */
  uint32_t                   ref_counter; /**< number of client using this nodelist */
#ifdef THREADSAFE
  uint32_t reentrance; /**< reentrance count */
#if defined(_MSC_VER) || defined(__MINGW32__)
  HANDLE mutex;
#else
  pthread_mutex_t mutex;
#endif
#endif
} in3_nodeselect_def_t;

/** wrapper for each client pointing to the global data*/
typedef struct in3_nodelist_wrapper {
  in3_nodeselect_def_t* data; /**< points to the global nodelist data*/
} in3_nodeselect_wrapper_t;

/** removes all nodes and their weights from the nodelist */
NONULL void in3_nodelist_clear(in3_nodeselect_def_t* data);

#ifdef NODESELECT_DEF_WL
/** removes all nodes and their weights from the nodelist */
NONULL void in3_whitelist_clear(in3_whitelist_t* data);

/** updates all whitelisted flags in the nodelist */
NONULL void in3_client_run_chain_whitelisting(in3_nodeselect_def_t* data);
#endif

/** check if the nodelist is up to date.
 * 
 * if not it will fetch a new version first (if the needs_update-flag is set).
 */
NONULL in3_ret_t in3_node_list_get(in3_ctx_t* ctx, in3_nodeselect_def_t* data, bool update, in3_node_t** nodelist, unsigned int* nodelist_length, in3_node_weight_t** weights);

/**
 * filters and fills the weights on a returned linked list.
 */
NONULL_FOR((1, 2, 3, 4, 7, 8))
node_match_t* in3_node_list_fill_weight(in3_t* c, in3_nodeselect_def_t* data, in3_node_t* all_nodes, in3_node_weight_t* weights, unsigned int len, uint64_t now, uint32_t* total_weight, unsigned int* total_found, const in3_node_filter_t* filter);

/**
 * calculates the weight for a node.
 */
NONULL uint32_t in3_node_calculate_weight(in3_node_weight_t* n, uint32_t capa, uint64_t now);
/**
 * picks (based on the config) a random number of nodes and returns them as weightslist.
 */
NONULL_FOR((1, 2, 3))
in3_ret_t in3_node_list_pick_nodes(in3_ctx_t* ctx, in3_nodeselect_def_t* data, node_match_t** nodes, unsigned int request_count, const in3_node_filter_t* filter);

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

NONULL static inline in3_node_t* get_node_idx(const in3_nodeselect_def_t* data, unsigned int index) {
  return index < data->nodelist_length ? data->nodelist + index : NULL;
}

NONULL static inline in3_node_weight_t* get_node_weight_idx(const in3_nodeselect_def_t* data, unsigned int index) {
  return index < data->nodelist_length ? data->weights + index : NULL;
}

static inline in3_node_t* get_node(const in3_nodeselect_def_t* data, const node_match_t* node) {
  return node ? get_node_idx(data, node->index) : NULL;
}

NONULL static inline in3_node_weight_t* get_node_weight(const in3_nodeselect_def_t* data, const node_match_t* node) {
  return node ? get_node_weight_idx(data, node->index) : NULL;
}

static inline bool is_blacklisted(const in3_node_t* node) { return node && node->blocked; }

NONULL static in3_ret_t blacklist_node(in3_nodeselect_def_t* data, unsigned int index, uint64_t secs_from_now) {
  in3_node_t* node = get_node_idx(data, index);
  if (is_blacklisted(node)) return IN3_ERPC; // already handled

  if (node && !node->blocked) {
    in3_node_weight_t* w = get_node_weight_idx(data, index);
    if (!w) {
      in3_log_debug("failed to blacklist node: %s\n", node->url);
      return IN3_EFIND;
    }

    // blacklist the node
    uint64_t blacklisted_until_ = in3_time(NULL) + secs_from_now;
    if (w->blacklisted_until != blacklisted_until_)
      data->dirty = true;
    w->blacklisted_until = blacklisted_until_;
    node->blocked        = true;
    in3_log_debug("Blacklisting node for unverifiable response: %s\n", node ? node->url : "");
  }
  return IN3_OK;
}

NONULL static inline in3_ret_t blacklist_node_addr(in3_nodeselect_def_t* data, const address_t node_addr, uint64_t secs_from_now) {
  for (unsigned int i = 0; i < data->nodelist_length; ++i)
    if (!memcmp(data->nodelist[i].address, node_addr, 20))
      return blacklist_node(data, data->nodelist[i].index, secs_from_now);
  return IN3_OK;
}

NONULL static inline in3_ret_t blacklist_node_url(in3_nodeselect_def_t* data, const char* node_url, uint64_t secs_from_now) {
  for (unsigned int i = 0; i < data->nodelist_length; ++i)
    if (!strcmp(data->nodelist[i].url, node_url))
      return blacklist_node(data, data->nodelist[i].index, secs_from_now);
  return IN3_OK;
}

#endif
