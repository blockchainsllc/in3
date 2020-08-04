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

// @PUBLIC_HEADER
/** @file
 * this file defines the incubed configuration struct and it registration.
 * 
 * 
 * */

#ifndef CLIENT_H
#define CLIENT_H

#include "../util/bytes.h"
#include "../util/data.h"
#include "../util/error.h"
#include "../util/mem.h"
#include "../util/stringbuilder.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define IN3_PROTO_VER "2.1.0" /**< the protocol version used when sending requests from the this client */

#define CHAIN_ID_MULTICHAIN 0x0    /**< chain_id working with all known chains */
#define CHAIN_ID_MAINNET    0x01   /**< chain_id for mainnet */
#define CHAIN_ID_KOVAN      0x2a   /**< chain_id for kovan */
#define CHAIN_ID_TOBALABA   0x44d  /**< chain_id for tobalaba */
#define CHAIN_ID_GOERLI     0x5    /**< chain_id for goerlii */
#define CHAIN_ID_EVAN       0x4b1  /**< chain_id for evan */
#define CHAIN_ID_EWC        0xf6   /**< chain_id for ewc */
#define CHAIN_ID_IPFS       0x7d0  /**< chain_id for ipfs */
#define CHAIN_ID_BTC        0x99   /**< chain_id for btc */
#define CHAIN_ID_LOCAL      0x11 /**< chain_id for local chain */
#define DEF_REPL_LATEST_BLK 6      /**< default replace_latest_block */

/**
 * type for a chain_id.
 */
typedef uint32_t chain_id_t;

struct in3_ctx;

/** the type of the chain. 
 * 
 * for incubed a chain can be any distributed network or database with incubed support.
 * Depending on this chain-type the previously registered verifyer will be choosen and used.
 */
typedef enum {
  CHAIN_ETH       = 0, /**< Ethereum chain */
  CHAIN_SUBSTRATE = 1, /**< substrate chain */
  CHAIN_IPFS      = 2, /**< ipfs verifiaction */
  CHAIN_BTC       = 3, /**< Bitcoin chain */
  CHAIN_EOS       = 4, /**< EOS chain */
  CHAIN_IOTA      = 5, /**< IOTA chain */
  CHAIN_GENERIC   = 6, /**< other chains */
} in3_chain_type_t;

/** the type of proof.
 * 
 * Depending on the proof-type different levels of proof will be requested from the node.
*/
typedef enum {
  PROOF_NONE     = 0, /**< No Verification */
  PROOF_STANDARD = 1, /**< Standard Verification of the important properties */
  PROOF_FULL     = 2  /**< All field will be validated including uncles */
} in3_proof_t;

/**
 * Node capabilities
 * @note Always access using getters/setters in nodelist.h
 */
typedef uint64_t in3_node_props_t;

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

/**
 * a list of flags definiing the behavior of the incubed client. They should be used as bitmask for the flags-property.
 */
typedef enum {
  FLAGS_KEEP_IN3         = 0x1,  /**< the in3-section with the proof will also returned */
  FLAGS_AUTO_UPDATE_LIST = 0x2,  /**< the nodelist will be automaticly updated if the last_block is newer  */
  FLAGS_INCLUDE_CODE     = 0x4,  /**< the code is included when sending eth_call-requests  */
  FLAGS_BINARY           = 0x8,  /**< the client will use binary format  */
  FLAGS_HTTP             = 0x10, /**< the client will try to use http instead of https  */
  FLAGS_STATS            = 0x20, /**< nodes will keep track of the stats (default=true)  */
  FLAGS_NODE_LIST_NO_SIG = 0x40, /**< nodelist update request will not automatically ask for signatures and proof */
  FLAGS_BOOT_WEIGHTS     = 0x80  /**< if true the client will initialize the first weights from the nodelist given by the nodelist.*/
} in3_flags_type_t;

/**
 * a list of node attributes (mostly used internally)
 */
typedef enum {
  ATTR_WHITELISTED = 1, /**< indicates if node exists in whiteList */
  ATTR_BOOT_NODE   = 2, /**< used to avoid filtering manually added nodes before first nodeList update */
} in3_node_attr_type_t;

typedef uint8_t in3_node_attr_t;

/** incubed node-configuration. 
 * 
 * These information are read from the Registry contract and stored in this struct representing a server or node.
 */
typedef struct in3_node {
  address_t        address;  /**< address of the server */
  uint64_t         deposit;  /**< the deposit stored in the registry contract, which this would lose if it sends a wrong blockhash */
  uint_fast16_t    index;    /**< index within the nodelist, also used in the contract as key */
  uint_fast16_t    capacity; /**< the maximal capacity able to handle */
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
#ifdef PAY
  uint32_t price; /**< the price per request unit */
  uint64_t payed; /**< already payed */
#endif
} in3_node_weight_t;

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
 * returns the value of the specified propertytype.
 * @return value as a number
 */
static inline uint32_t in3_node_props_get(in3_node_props_t      np,  /**< property to read from */
                                          in3_node_props_type_t t) { /**< the value to extract  */
  return ((t == NODE_PROP_MIN_BLOCK_HEIGHT) ? ((np >> 32U) & 0xFFU) : !!(np & t));
}

/**
 * checkes if the given type is set in the properties
 * @return true if set
 */
static inline bool in3_node_props_matches(in3_node_props_t      np,  /**< property to read from */
                                          in3_node_props_type_t t) { /**< the value to extract */
  return !!(np & t);
}

/**
 * defines a whitelist structure used for the nodelist.
 */
typedef struct in3_whitelist {
  bool      needs_update; /**< if true the nodelist should be updated and will trigger a `in3_nodeList`-request before the next request is send. */
  uint64_t  last_block;   /**< last blocknumber the whiteList was updated, which is used to detect changed in the whitelist */
  address_t contract;     /**< address of whiteList contract. If specified, whiteList is always auto-updated and manual whiteList is overridden */
  bytes_t   addresses;    /**< serialized list of node addresses that constitute the whiteList */
} in3_whitelist_t;

/** represents a blockhash which was previously verified */
typedef struct in3_verified_hash {
  uint64_t  block_number; /**< the number of the block */
  bytes32_t hash;         /**< the blockhash */
} in3_verified_hash_t;

/**
 * Chain definition inside incubed.
 * 
 * for incubed a chain can be any distributed network or database with incubed support.
 */
typedef struct in3_chain {
  bool                 dirty;           /**< indicates whether the nodelist has been modified after last read from cache */
  uint8_t              version;         /**< version of the chain */
  unsigned int         nodelist_length; /**< number of nodes in the nodeList */
  uint16_t             avg_block_time;  /**< average block time (seconds) for this chain (calculated internally) */
  chain_id_t           chain_id;        /**< chain_id, which could be a free or based on the public ethereum networkId*/
  in3_chain_type_t     type;            /**< chaintype */
  uint64_t             last_block;      /**< last blocknumber the nodeList was updated, which is used to detect changed in the nodelist*/
  in3_node_t*          nodelist;        /**< array of nodes */
  in3_node_weight_t*   weights;         /**< stats and weights recorded for each node */
  bytes_t**            init_addresses;  /**< array of addresses of nodes that should always part of the nodeList */
  bytes_t*             contract;        /**< the address of the registry contract */
  bytes32_t            registry_id;     /**< the identifier of the registry */
  in3_verified_hash_t* verified_hashes; /**< contains the list of already verified blockhashes */
  in3_whitelist_t*     whitelist;       /**< if set the whitelist of the addresses. */
  struct {
    uint64_t  exp_last_block; /**< the last_block when the nodelist last changed reported by this node */
    uint64_t  timestamp;      /**< approx. time when nodelist must be updated (i.e. when reported last_block will be considered final) */
    address_t node;           /**< node that reported the last_block which necessitated a nodeList update */
  } * nodelist_upd8_params;
} in3_chain_t;

/** 
 * payment prepearation function.
 * 
 * allows the payment to handle things before the request will be send.
 * 
*/
typedef in3_ret_t (*in3_pay_prepare)(struct in3_ctx* ctx, void* cptr);

/** 
 * called after receiving a parseable response with a in3-section.
*/
typedef in3_ret_t (*in3_pay_follow_up)(struct in3_ctx* ctx, void* node, d_token_t* in3, d_token_t* error, void* cptr);

/** 
 * free function for the custom pointer.
*/
typedef void (*in3_pay_free)(void* cptr);

/** 
 * handles the request.
 * 
 * this function is called when the in3-section of payload of the request is built and allows the handler to add properties. 
*/
typedef in3_ret_t (*in3_pay_handle_request)(struct in3_ctx* ctx, sb_t* sb, void* cptr);

/** 
 * the payment handler.
 * 
 * if a payment handler is set it will be used when generating the request.
*/
typedef struct in3_pay {
  in3_pay_prepare        prepare;        /**< payment prepearation function.*/
  in3_pay_follow_up      follow_up;      /**< payment function to be called after the request.*/
  in3_pay_handle_request handle_request; /**< this function is called when the in3-section of payload of the request is built and allows the handler to add properties. .*/
  in3_pay_free           free;           /**< frees the custom pointer (cptr).*/
  void*                  cptr;           /**< custom object whill will be passed to functions */
} in3_pay_t;

/** response-object. 
 * 
 * if the error has a length>0 the response will be rejected
 */
typedef struct in3_response {
  uint32_t  time;  /**< measured time (in ms) which will be used for ajusting the weights */
  in3_ret_t state; /**< the state of the response */
  sb_t      data;  /**< a stringbuilder to add the result */
} in3_response_t;

/** Incubed Configuration. 
 * 
 * This struct holds the configuration and also point to internal resources such as filters or chain configs.
 * 
 */
typedef struct in3_t_ in3_t;

/**
 * Filter type used internally when managing filters.
 */
typedef enum {
  FILTER_EVENT   = 0, /**< Event filter */
  FILTER_BLOCK   = 1, /**< Block filter */
  FILTER_PENDING = 2, /**< Pending filter (Unsupported) */
} in3_filter_type_t;

typedef struct in3_filter_t_ {
  bool              is_first_usage;         /**< if true the filter was not used previously */
  in3_filter_type_t type;                   /**< filter type: (event, block or pending) */
  uint64_t          last_block;             /**< block no. when filter was created OR eth_getFilterChanges was called */
  char*             options;                /**< associated filter options */
  void (*release)(struct in3_filter_t_* f); /**< method to release owned resources */
} in3_filter_t;

#define PLGN_ACT_TRANSPORT (PLGN_ACT_TRANSPORT_SEND | PLGN_ACT_TRANSPORT_RECEIVE | PLGN_ACT_TRANSPORT_CLEAN)
#define PLGN_ACT_CACHE     (PLGN_ACT_CACHE_SET | PLGN_ACT_CACHE_GET | PLGN_ACT_CACHE_CLEAR)

/** plugin action list */
typedef enum {
  PLGN_ACT_INIT              = 0x1,      /**< initialize plugin - use for allocating/setting-up internal resources */
  PLGN_ACT_TERM              = 0x2,      /**< terminate plugin - use for releasing internal resources and cleanup. */
  PLGN_ACT_TRANSPORT_SEND    = 0x4,      /**< sends out a request - the transport plugin will receive a request_t as plgn_ctx, it may set a cptr which will be passed back when fetching more resonses. */
  PLGN_ACT_TRANSPORT_RECEIVE = 0x8,      /**< fetch next response - the transport plugin will receive a request_t as plgn_ctx, which contains a cptr  if set previously*/
  PLGN_ACT_TRANSPORT_CLEAN   = 0x10,     /**< freeup transport resources - the transport plugin will receive a request_t as plgn_ctx if the cptr was set.*/
  PLGN_ACT_SIGN_ACCOUNT      = 0x20,     /**<  returns the default account of the signer */
  PLGN_ACT_SIGN_PREPARE      = 0x40,     /**< allowes a wallet to manipulate the payload before signing - the plgn_ctx will be in3_sign_ctx_t. This way a tx can be send through a multisig */
  PLGN_ACT_SIGN              = 0x80,     /**<  signs the payload - the plgn_ctx will be in3_sign_ctx_t.  */
  PLGN_ACT_RPC_HANDLE        = 0x100,    /**< a plugin may respond to a rpc-request directly (without sending it to the node). */
  PLGN_ACT_RPC_VERIFY        = 0x200,    /**< verifies the response. the plgn_ctx will be a in3_vctx_t holding all data */
  PLGN_ACT_CACHE_SET         = 0x400,    /**< stores data to be reused later - the plgn_ctx will be a in3_cache_ctx_t containing the data */
  PLGN_ACT_CACHE_GET         = 0x800,    /**< reads data to be previously stored - the plgn_ctx will be a in3_cache_ctx_t containing the key. if the data was found the data-property needs to be set. */
  PLGN_ACT_CACHE_CLEAR       = 0x1000,   /**< clears alls stored data - plgn_ctx will be NULL  */
  PLGN_ACT_CONFIG_SET        = 0x2000,   /**< gets a config-token and reads data from it */
  PLGN_ACT_CONFIG_GET        = 0x4000,   /**< gets a stringbuilder and adds all config to it. */
  PLGN_ACT_PAY_PREPARE       = 0x8000,   /**< prerpares a payment */
  PLGN_ACT_PAY_FOLLOWUP      = 0x10000,  /**< called after a requeest to update stats. */
  PLGN_ACT_PAY_HANDLE        = 0x20000,  /**< handles the payment */
  PLGN_ACT_PAY_SIGN_REQ      = 0x40000,  /**< signs a request */
  PLGN_ACT_NL_PICK_DATA      = 0x80000,  /**< picks the data nodes */
  PLGN_ACT_NL_PICK_SIGNER    = 0x100000, /**< picks the signer nodes */
  PLGN_ACT_NL_PICK_FOLLOWUP  = 0x200000, /**< called after receiving a response in order to decide whether a update is needed. */

} in3_plugin_act_t;

/**
 * plugin interface definition
 */
typedef struct in3_plugin in3_plugin_t;

/**
 * plugin action handler
 *
 * Implementations of this function must strictly follow the below pattern for return values -
 * * IN3_OK - successfully handled specified action
 * * IN3_WAITING - handling specified action, but waiting for more information
 * * IN3_EIGNORE - could handle specified action, but chose to ignore it so maybe another handler could handle it
 * * Other errors - handled but failed
 */
typedef in3_ret_t (*in3_plugin_act_fn)(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx);

typedef uint32_t in3_plugin_supp_acts_t;

struct in3_plugin {
  in3_plugin_supp_acts_t acts;      /**< bitmask of supported actions this plugin can handle */
  void*                  data;      /**< opaque pointer to plugin data */
  in3_plugin_act_fn      action_fn; /**< plugin action handler */
  in3_plugin_t*          next;      /**< pointer to next plugin in list */
};

/**
 * Handler which is added to client config in order to handle filter.
 */
typedef struct in3_filter_handler_t_ {
  in3_filter_t** array; /** array of filters */
  size_t         count; /** counter for filters */
} in3_filter_handler_t;

/** Incubed Configuration. 
 * 
 * This struct holds the configuration and also point to internal resources such as filters or chain configs.
 * 
 */
struct in3_t_ {
  uint8_t                request_count;        /**< the number of request send when getting a first answer */
  uint8_t                signature_count;      /**< the number of signatures used to proof the blockhash. */
  uint8_t                replace_latest_block; /**< if specified, the blocknumber *latest* will be replaced by blockNumber- specified value */
  uint_fast8_t           flags;                /**< a bit mask with flags defining the behavior of the incubed client. See the FLAG...-defines*/
  uint16_t               node_limit;           /**< the limit of nodes to store in the client. */
  uint16_t               finality;             /**< the number of signatures in percent required for the request*/
  uint16_t               chains_length;        /**< number of configured chains */
  uint_fast16_t          max_attempts;         /**< the max number of attempts before giving up*/
  uint_fast16_t          max_verified_hashes;  /**< max number of verified hashes to cache */
  uint_fast16_t          pending;              /**< number of pending requests created with this instance */
  uint32_t               cache_timeout;        /**< number of seconds requests can be cached. */
  uint32_t               timeout;              /**< specifies the number of milliseconds before the request times out. increasing may be helpful if the device uses a slow connection. */
  chain_id_t             chain_id;             /**< servers to filter for the given chain. The chain-id based on EIP-155.*/
  in3_plugin_supp_acts_t plugin_acts;          /**< bitmask of supported actions of all plugins registered with this client */
  in3_proof_t            proof;                /**< the type of proof used */
  uint64_t               min_deposit;          /**< min stake of the server. Only nodes owning at least this amount will be chosen. */
  in3_node_props_t       node_props;           /**< used to identify the capabilities of the node. */
  in3_chain_t*           chains;               /**< chain spec and nodeList definitions*/
  in3_filter_handler_t*  filters;              /**< filter handler */
  in3_plugin_t*          plugins;              /**< list of registered plugins */

#ifdef PAY
  in3_pay_t* pay; /**< payment handler. if set it will add payment to each request */
#endif

#ifndef DEV_NO_INTRN_PTR
  void* internal; /**< pointer to internal data */
#endif
};

/** creates a new Incubes configuration and returns the pointer.
 * 
 * This Method is depricated. you should use `in3_for_chain(CHAIN_ID_MULTICHAIN)` instead.
 * 
 * you need to free this instance with `in3_free` after use!
 * 
 * Before using the client you still need to set the tramsport and optional the storage handlers:
 * 
 *  * example of initialization:
 * ```c
 * // register verifiers
 * in3_register_eth_full();
 * 
 * // create new client
 * in3_t* client = in3_new();
 * 
 * // configure transport
 * client->transport    = send_curl;
 *
 * // configure storage
 * in3_set_storage_handler(c, storage_get_item, storage_set_item, storage_clear, NULL);
 * 
 * // ready to use ...
 * ```
 * 
 * @returns the incubed instance.
 */
in3_t* in3_new() __attribute__((deprecated("use in3_for_chain(CHAIN_ID_MULTICHAIN)")));

/** creates a new Incubes configuration for a specified chain and returns the pointer.
 * when creating the client only the one chain will be configured. (saves memory). 
 * but if you pass `CHAIN_ID_MULTICHAIN` as argument all known chains will be configured allowing you to switch between chains within the same client or configuring your own chain. 
 * 
 * you need to free this instance with `in3_free` after use!
 * 
 * Before using the client you still need to set the tramsport and optional the storage handlers:
 * 
 *  * example of initialization:
 * ```c
 * // register verifiers
 * in3_register_eth_full();
 * 
 * // create new client
 * in3_t* client = in3_for_chain(CHAIN_ID_MAINNET);
 * 
 * // configure transport
 * client->transport    = send_curl;
 *
 * // configure storage
 * in3_set_storage_handler(c, storage_get_item, storage_set_item, storage_clear, NULL);
 * 
 * // ready to use ...
 * ```
 * ** This Method is depricated. you should use `in3_for_chain` instead.**
 * 
 * @returns the incubed instance.
 */
#define in3_for_chain(chain_id) in3_for_chain_default(chain_id)

in3_t* in3_for_chain_default(
    chain_id_t chain_id /**< the chain_id (see CHAIN_ID_... constants). */
);

/** sends a request and stores the result in the provided buffer */
NONULL in3_ret_t in3_client_rpc(
    in3_t*      c,      /**< [in] the pointer to the incubed client config. */
    const char* method, /**< [in] the name of the rpc-funcgtion to call. */
    const char* params, /**< [in] docs for input parameter v. */
    char**      result, /**< [in] pointer to string which will be set if the request was successfull. This will hold the result as json-rpc-string. (make sure you free this after use!) */
    char**      error /**< [in] pointer to a string containg the error-message. (make sure you free it after use!) */);

/** sends a request and stores the result in the provided buffer, this method will always return the first, so bulk-requests are not saupported. */
NONULL in3_ret_t in3_client_rpc_raw(
    in3_t*      c,       /**< [in] the pointer to the incubed client config. */
    const char* request, /**< [in] the rpc request including method and params. */
    char**      result,  /**< [in] pointer to string which will be set if the request was successfull. This will hold the result as json-rpc-string. (make sure you free this after use!) */
    char**      error /**< [in] pointer to a string containg the error-message. (make sure you free it after use!) */);

/** executes a request and returns result as string. in case of an error, the error-property of the result will be set. 
 * This fuinction also supports sending bulk-requests, but you can not mix internal and external calls, since bulk means all requests will be send to picked nodes.
 * The resulting string must be free by the the caller of this function! 
 */
NONULL char* in3_client_exec_req(
    in3_t* c,  /**< [in] the pointer to the incubed client config. */
    char*  req /**< [in] the request as rpc. */
);

/** registers a new chain or replaces a existing (but keeps the nodelist)*/
NONULL_FOR((1, 4))
in3_ret_t in3_client_register_chain(
    in3_t*           client,      /**< [in] the pointer to the incubed client config. */
    chain_id_t       chain_id,    /**< [in] the chain id. */
    in3_chain_type_t type,        /**< [in] the verification type of the chain. */
    address_t        contract,    /**< [in] contract of the registry. */
    bytes32_t        registry_id, /**< [in] the identifier of the registry. */
    uint8_t          version,     /**< [in] the chain version. */
    address_t        wl_contract  /**< [in] contract of whiteList. */
);

/** adds a node to a chain ore updates a existing node */
NONULL in3_ret_t in3_client_add_node(
    in3_t*           client,   /**< [in] the pointer to the incubed client config. */
    chain_id_t       chain_id, /**< [in] the chain id. */
    char*            url,      /**< [in] url of the nodes. */
    in3_node_props_t props,    /**< [in]properties of the node. */
    address_t        address);        /**< [in] public address of the signer. */

/** removes a node from a nodelist */
NONULL in3_ret_t in3_client_remove_node(
    in3_t*     client,   /**< [in] the pointer to the incubed client config. */
    chain_id_t chain_id, /**< [in] the chain id. */
    address_t  address);  /**< [in] public address of the signer. */

/** removes all nodes from the nodelist */
NONULL in3_ret_t in3_client_clear_nodes(
    in3_t*     client,    /**< [in] the pointer to the incubed client config. */
    chain_id_t chain_id); /**< [in] the chain id. */

/** frees the references of the client */
NONULL void in3_free(in3_t* a /**< [in] the pointer to the incubed client config to free. */);

/**
 * inits the cache.
 *
 * this will try to read the nodelist from cache.
 */
NONULL in3_ret_t in3_cache_init(
    in3_t* c /**< the incubed client */
);

/**
 * returns the chain-config for the current chain_id.
 */
NONULL in3_chain_t* in3_get_chain(
    const in3_t* c /**< the incubed client */
);

/**
 * finds the chain-config for the given chain_id.
 * 
 * My return NULL if not found.
 */
NONULL in3_chain_t* in3_find_chain(
    const in3_t* c /**< the incubed client */,
    chain_id_t   chain_id /**< chain_id */
);

/**
 * configures the clent based on a json-config.
 * 
 * For details about the structure of ther config see https://in3.readthedocs.io/en/develop/api-ts.html#type-in3config
 * Returns NULL on success, and error string on failure (to be freed by caller) - in which case the client state is undefined
 */
NONULL char* in3_configure(
    in3_t*      c,     /**< the incubed client */
    const char* config /**< JSON-string with the configuration to set. */
);

/**
 * gets the current config as json.
 * 
 * For details about the structure of ther config see https://in3.readthedocs.io/en/develop/api-ts.html#type-in3config
 */
NONULL char* in3_get_config(
    in3_t* c /**< the incubed client */
);

/** a register-function for a plugion.
 */
typedef in3_ret_t (*plgn_register)(in3_t* c);

#ifdef PAY
/**
  *  configure function for a payment.
  */
typedef char* (*pay_configure)(in3_t* c, d_token_t* config);

/**
 * registers a payment provider
 */
void in3_register_payment(
    char*         name,   /**< name of the payment-type */
    pay_configure handler /**< pointer to the handler- */
);
#endif

#endif
