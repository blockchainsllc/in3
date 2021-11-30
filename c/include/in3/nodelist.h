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

/**
 * handles nodelists.
 *
 * */

#include "client.h"
#include "request.h"
#include "log.h"
#include "mem.h"
#include <time.h>

#ifndef NODELIST_H
#define NODELIST_H

#ifdef THREADSAFE
#if defined(_MSC_VER) || defined(__MINGW32__)
#include <windows.h>
typedef HANDLE in3_mutex_t;
#define MUTEX_INIT(mutex)   mutex = CreateMutex(NULL, FALSE, NULL);
#define MUTEX_LOCK(mutex)   WaitForSingleObject(mutex, INFINITE);
#define MUTEX_UNLOCK(mutex) ReleaseMutex(mutex);
#define MUTEX_FREE(mutex)   CloseHandle(mutex);
#else
#include <pthread.h>
typedef pthread_mutex_t in3_mutex_t;
#define MUTEX_INIT(mutex)                                      \
  {                                                            \
    pthread_mutexattr_t attr;                                  \
    pthread_mutexattr_init(&attr);                             \
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); \
    pthread_mutex_init(&(mutex), &attr);                       \
  }
#define MUTEX_LOCK(mutex)   pthread_mutex_lock(&(mutex));
#define MUTEX_UNLOCK(mutex) pthread_mutex_unlock(&(mutex));
#define MUTEX_FREE(mutex)   pthread_mutex_destroy(&(mutex));
#endif
#endif

/**
 * a list of node attributes (mostly used internally)
 */
typedef enum {
  ATTR_WHITELISTED = 1, /**< indicates if node exists in whiteList */
  ATTR_BOOT_NODE   = 2, /**< used to avoid filtering manually added nodes before first nodeList update */
} in3_node_attr_type_t;

/** incubed node-configuration.
 *
 * These information are read from the Registry contract and stored in this struct representing a server or node.
 */
typedef struct in3_node {
  address_t        address;  /**< address of the server */
  bool             blocked;  /**< if true this node has  been blocked for sending wrong responses */
  uint_fast16_t    index;    /**< index within the nodelist, also used in the contract as key */
  uint_fast16_t    capacity; /**< the maximal capacity able to handle */
  uint64_t         deposit;  /**< the deposit stored in the registry contract, which this would lose if it sends a wrong blockhash */
  in3_node_props_t props;    /**< used to identify the capabilities of the node. See in3_node_props_type_t in nodelist.h */
  char*            url;      /**< the url of the node */
  uint_fast8_t     attrs;    /**< bitmask of internal attributes */
} in3_node_t;

/**
 * Weight or reputation of a node.
 *
 * Based on the past performance of the node a weight is calculated given faster nodes a higher weight
 * and chance when selecting the next node from the nodelist.
 * These weights will also be stored in the cache (if available)
 */
typedef struct in3_node_weight {
  uint32_t response_count;      /**< counter for responses */
  uint32_t total_response_time; /**< total of all response times */
  uint64_t blacklisted_until;   /**< if >0 this node is blacklisted until k. k is a unix timestamp */
} in3_node_weight_t;

/**
 * defines a whitelist structure used for the nodelist.
 */
typedef struct in3_whitelist {
  bool      needs_update; /**< if true the nodelist should be updated and will trigger a `in3_nodeList`-request before the next request is send. */
  uint64_t  last_block;   /**< last blocknumber the whiteList was updated, which is used to detect changed in the whitelist */
  address_t contract;     /**< address of whiteList contract. If specified, whiteList is always auto-updated and manual whiteList is overridden */
  bytes_t   addresses;    /**< serialized list of node addresses that constitute the whiteList */
} in3_whitelist_t;

typedef enum {
  NODE_PROP_PROOF            = 0x1,   /**< filter out nodes which are providing no proof */
  NODE_PROP_MULTICHAIN       = 0x2,   /**< filter out nodes other then which have capability of the same RPC endpoint may also accept requests for different chains */
  NODE_PROP_ARCHIVE          = 0x4,   /**< filter out non-archive supporting nodes */
  NODE_PROP_HTTP             = 0x8,   /**< filter out non-http nodes  */
  NODE_PROP_BINARY           = 0x10,  /**< filter out nodes that don't support binary encoding */
  NODE_PROP_ONION            = 0x20,  /**< filter out non-onion nodes */
  NODE_PROP_SIGNER           = 0x40,  /**< filter out non-signer nodes */
  NODE_PROP_DATA             = 0x80,  /**< filter out non-data provider nodes */
  NODE_PROP_STATS            = 0x100, /**< filter out nodes that do not provide stats */
  NODE_PROP_MIN_BLOCK_HEIGHT = 0x400, /**< filter out nodes that will sign blocks with lower min block height than specified */
} in3_node_props_type_t;

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

  chain_id_t                 chain_id;           /**< the chain_id of the data */
  struct in3_nodeselect_def* next;               /**< the next in the linked list */
  uint32_t                   ref_counter;        /**< number of client using this nodelist */
  bytes_t*                   pre_address_filter; /**< addresses of allowed list (usually because those nodes where paid for) */

#ifdef THREADSAFE
  in3_mutex_t mutex; /**< mutex to lock this nodelist */
#endif
} in3_nodeselect_def_t;

/** config for each client pointing to the global data*/
typedef struct in3_nodeselect_config {
  //  in3_nodeselect_def_t* data;          /**< points to the global nodelist data*/
  in3_nodeselect_def_t** chains;        /**< array of pointers to the global nodelist data*/
  uint32_t               chains_len;    /**< pnumber of chains*/
  in3_node_props_t       node_props;    /**< used to identify the capabilities of the node. */
  uint64_t               min_deposit;   /**< min stake of the server. Only nodes owning at least this amount will be chosen. */
  uint16_t               node_limit;    /**< the limit of nodes to store in the client. */
  uint8_t                request_count; /**< the number of request send when getting a first answer */
} in3_nodeselect_config_t;

/** returns the nodelistwrapper.*/
NONULL in3_nodeselect_def_t* in3_get_nodelist_data(in3_nodeselect_config_t* conf, chain_id_t chain_id);

/** returns the nodelistwrapper.*/
NONULL in3_nodeselect_config_t* in3_get_nodelist(in3_t* c);

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
NONULL in3_ret_t in3_node_list_get(in3_req_t* req, in3_nodeselect_def_t* data, bool update, in3_node_t** nodelist, unsigned int* nodelist_length, in3_node_weight_t** weights);

/**
 * filters and fills the weights on a returned linked list.
 */
NONULL_FOR((1, 2, 3, 4, 7, 8))
node_match_t* in3_node_list_fill_weight(in3_t* c, in3_nodeselect_config_t* w, in3_nodeselect_def_t* data, in3_node_t* all_nodes, in3_node_weight_t* weights, unsigned int len, uint64_t now, uint32_t* total_weight, unsigned int* total_found, const in3_node_filter_t* filter, bytes_t* pre_filter);

/**
 * calculates the weight for a node.
 */
NONULL uint32_t in3_node_calculate_weight(in3_node_weight_t* n, uint32_t capa, uint64_t now);
/**
 * picks (based on the config) a random number of nodes and returns them as weightslist.
 */
NONULL_FOR((1, 2, 3))
in3_ret_t in3_node_list_pick_nodes(in3_req_t* req, in3_nodeselect_config_t* w, in3_nodeselect_def_t* data, node_match_t** nodes, unsigned int request_count, const in3_node_filter_t* filter);

/**
 * forces the client to update the nodelist
 */
in3_ret_t update_nodes(in3_t* c, in3_nodeselect_def_t* data);

#define NODE_FILTER_INIT \
  (in3_node_filter_t) { .props = 0, .nodes = NULL }

/**
 * Initializer for in3_node_props_t
 */
#define in3_node_props_init(np) *(np) = 0

/**
 * setter method for interacting with in3_node_props_t.
 */
NONULL void in3_node_props_set(in3_node_props_t*     node_props, /**< pointer to the properties to change */
                               in3_node_props_type_t type,       /**< key or type of the property */
                               uint8_t               value       /**< value to set */
);

/**
 * returns the value of the specified property-type.
 * @return value as a number
 */
static inline uint32_t in3_node_props_get(in3_node_props_t      np,  /**< property to read from */
                                          in3_node_props_type_t t) { /**< the value to extract  */
  return ((t == NODE_PROP_MIN_BLOCK_HEIGHT) ? ((np >> 32U) & 0xFFU) : !!(np & t));
}

/**
 * checks if the given type is set in the properties
 * @return true if set
 */
static inline bool in3_node_props_matches(in3_node_props_t      np,  /**< property to read from */
                                          in3_node_props_type_t t) { /**< the value to extract */
  return !!(np & t);
}

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
