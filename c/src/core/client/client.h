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

#define CHAIN_ID_MULTICHAIN 0x0 /**< chain_id working with all known chains */
#define CHAIN_ID_MAINNET 0x01   /**< chain_id for mainnet */
#define CHAIN_ID_KOVAN 0x2a     /**< chain_id for kovan */
#define CHAIN_ID_TOBALABA 0x44d /**< chain_id for tobalaba */
#define CHAIN_ID_GOERLI 0x5     /**< chain_id for goerlii */
#define CHAIN_ID_EVAN 0x4b1     /**< chain_id for evan */
#define CHAIN_ID_EWC 0xf6       /**< chain_id for ewc */
#define CHAIN_ID_IPFS 0x7d0     /**< chain_id for ipfs */
#define CHAIN_ID_BTC 0x99       /**< chain_id for btc */
#define CHAIN_ID_LOCAL 0xFFFF   /**< chain_id for local chain */
#define DEF_REPL_LATEST_BLK 6   /**< default replace_latest_block */

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
  bytes_t*         address;  /**< address of the server */
  uint64_t         deposit;  /**< the deposit stored in the registry contract, which this would lose if it sends a wrong blockhash */
  uint32_t         index;    /**< index within the nodelist, also used in the contract as key */
  uint32_t         capacity; /**< the maximal capacity able to handle */
  in3_node_props_t props;    /**< used to identify the capabilities of the node. See in3_node_props_type_t in nodelist.h */
  char*            url;      /**< the url of the node */
  uint8_t          attrs;    /**< bitmask of internal attributes */
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
  address_t contract;     /**< address of whiteList contract. If specified, whiteList is always auto-updated and manual whiteList is overridden */
  bytes_t   addresses;    /**< serialized list of node addresses that constitute the whiteList */
  uint64_t  last_block;   /**< last blocknumber the whiteList was updated, which is used to detect changed in the whitelist */
  bool      needs_update; /**< if true the nodelist should be updated and will trigger a `in3_nodeList`-request before the next request is send. */
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
  chain_id_t           chain_id;        /**< chain_id, which could be a free or based on the public ethereum networkId*/
  in3_chain_type_t     type;            /**< chaintype */
  uint64_t             last_block;      /**< last blocknumber the nodeList was updated, which is used to detect changed in the nodelist*/
  unsigned int         nodelist_length; /**< number of nodes in the nodeList */
  in3_node_t*          nodelist;        /**< array of nodes */
  in3_node_weight_t*   weights;         /**< stats and weights recorded for each node */
  bytes_t**            init_addresses;  /**< array of addresses of nodes that should always part of the nodeList */
  bytes_t*             contract;        /**< the address of the registry contract */
  bytes32_t            registry_id;     /**< the identifier of the registry */
  uint8_t              version;         /**< version of the chain */
  in3_verified_hash_t* verified_hashes; /**< contains the list of already verified blockhashes */
  in3_whitelist_t*     whitelist;       /**< if set the whitelist of the addresses. */
  uint16_t             avg_block_time;  /**< average block time (seconds) for this chain (calculated internally) */
  bool                 dirty;           /**< indicates whether the nodelist has been modified after last read from cache */
  void*                conf;            /**< this configuration will be set by the verifiers and allow to add special structs here.*/
  struct {
    address_t node;           /**< node that reported the last_block which necessitated a nodeList update */
    uint64_t  exp_last_block; /**< the last_block when the nodelist last changed reported by this node */
    uint64_t  timestamp;      /**< approx. time when nodelist must be updated (i.e. when reported last_block will be considered final) */
  } * nodelist_upd8_params;
} in3_chain_t;

/** 
 * storage handler function for reading from cache.
 * @returns the found result. if the key is found this function should return the values as bytes otherwise `NULL`.
 **/
typedef bytes_t* (*in3_storage_get_item)(
    void*       cptr, /**< a custom pointer as set in the storage handler*/
    const char* key   /**< the key to search in the cache */
);

/** 
 * storage handler function for writing to the cache.
 **/
typedef void (*in3_storage_set_item)(
    void*       cptr, /**< a custom pointer as set in the storage handler*/
    const char* key,  /**< the key to store the value.*/
    bytes_t*    value /**< the value to store.*/
);

/**
 * storage handler function for clearing the cache.
 **/
typedef void (*in3_storage_clear)(
    void* cptr /**< a custom pointer as set in the storage handler*/
);

/** 
 * storage handler to handle cache.
 **/
typedef struct in3_storage_handler {
  in3_storage_get_item get_item; /**< function pointer returning a stored value for the given key.*/
  in3_storage_set_item set_item; /**< function pointer setting a stored value for the given key.*/
  in3_storage_clear    clear;    /**< function pointer clearing all contents of cache.*/
  void*                cptr;     /**< custom pointer which will be passed to functions */
} in3_storage_handler_t;

#define IN3_SIGN_ERR_REJECTED -1 /**< return value used by the signer if the the signature-request was rejected. */
#define IN3_SIGN_ERR_ACCOUNT_NOT_FOUND -2 /**< return value used by the signer if the requested account was not found. */
#define IN3_SIGN_ERR_INVALID_MESSAGE -3 /**< return value used by the signer if the message was invalid. */
#define IN3_SIGN_ERR_GENERAL_ERROR -4 /**< return value used by the signer for unspecified errors. */

/** type of the requested signature */
typedef enum {
  SIGN_EC_RAW  = 0, /**< sign the data directly */
  SIGN_EC_HASH = 1, /**< hash and sign the data */
} d_signature_type_t;

/**
 * signing context. This Context is passed to the signer-function. 
 */
typedef struct sign_ctx {
  d_signature_type_t type;          /**< the type of signature*/
  bytes_t            message;       /**< the message to sign*/
  bytes_t            account;       /**< the account to use for the signature */
  uint8_t            signature[65]; /**< the resulting signature needs to be writte into these bytes */
  void*              wallet;        /**< the custom wallet-pointer  */
  struct in3_ctx*    ctx;           /**< the context of the request in order report errors */
} in3_sign_ctx_t;

/** 
 * signing function.
 * 
 * signs the given data and write the signature to dst.
 * the return value must be the number of bytes written to dst.
 * In case of an error a negativ value must be returned. It should be one of the IN3_SIGN_ERR... values.
 * 
*/
typedef in3_ret_t (*in3_sign)(in3_sign_ctx_t* ctx);

/** 
 * transform transaction function.
 * 
 * for multisigs, we need to change the transaction to gro through the ms.
 * if the new_tx is not set within the function, it will use the old_tx.
 * 
*/
typedef in3_ret_t (*in3_prepare_tx)(struct in3_ctx* ctx, bytes_t raw_tx, bytes_t* new_raw_tx);

/**
 * definition of a signer holding funciton-pointers and data.
 */
typedef struct in3_signer {
  in3_sign       sign;            /**< function pointer returning a stored value for the given key.*/
  in3_prepare_tx prepare_tx;      /**< function pointer returning capable of manipulating the transaction before signing it. This is needed in order to support multisigs.*/
  void*          wallet;          /**< custom object whill will be passed to functions */
  address_t      default_address; /**< the address in case no address is assigned*/
} in3_signer_t;

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
  in3_ret_t state; /**< the state of the response */
  sb_t      data;  /**< a stringbuilder to add the result */
  uint32_t  time;  /**< measured time (in ms) which will be used for ajusting the weights */
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
  REQ_ACTION_SEND    = 0, /**< The request should be send */
  REQ_ACTION_RECEIVE = 1, /**< a response is expected now. the request will not contains the urls anymore! */
  REQ_ACTION_CLEANUP = 2, /**< the cstptr can perform clean up */
} in3_req_action_t;

/** request-object. 
 * 
 * represents a RPC-request
 */
typedef struct in3_request {
  char*            payload;  /**< the payload to send */
  char**           urls;     /**< array of urls */
  uint_fast16_t    urls_len; /**< number of urls */
  in3_req_action_t action;   /**< the action the transport should execute */
  struct in3_ctx*  ctx;      /**< the current context */
  void*            cptr;     /**< a custom ptr to hold information during */
} in3_request_t;

/** the transport function to be implemented by the transport provider.
 */
typedef in3_ret_t (*in3_transport_send)(in3_request_t* request);

/**
 * Filter type used internally when managing filters.
 */
typedef enum {
  FILTER_EVENT   = 0, /**< Event filter */
  FILTER_BLOCK   = 1, /**< Block filter */
  FILTER_PENDING = 2, /**< Pending filter (Unsupported) */
} in3_filter_type_t;

typedef struct in3_filter_t_ {

  in3_filter_type_t type;                   /**< filter type: (event, block or pending) */
  char*             options;                /**< associated filter options */
  uint64_t          last_block;             /**< block no. when filter was created OR eth_getFilterChanges was called */
  bool              is_first_usage;         /**< if true the filter was not used previously */
  void (*release)(struct in3_filter_t_* f); /**< method to release owned resources */
} in3_filter_t;

/** plugin action list */
typedef enum {
  PLGN_ACT_INIT = 0x1, /**< initialize plugin - use for allocating/setting-up internal resources */
  PLGN_ACT_TERM = 0x2, /**< terminate plugin - use for releasing internal resources and cleanup. */
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
typedef in3_ret_t (*in3_plugin_act_fn)(in3_plugin_t* plugin, in3_plugin_act_t action, void* plugin_ctx);

typedef uint64_t in3_plugin_supp_acts_t;

struct in3_plugin {
  in3_plugin_supp_acts_t acts;      /**< bitmask of supported actions this plugin can handle */
  void*                  data;      /**< opaque pointer to plugin data */
  in3_plugin_act_fn      action_fn; /**< plugin action handler */
  in3_plugin_t*          next;      /**< pointer to next plugin in list */
};

/** plugin execution strategies, see in3_plugin_execute_ctx() for usage */
typedef enum {
  PLGN_EXC_ALL,
  PLGN_EXC_FIRST,
  PLGN_EXC_FITRST_OR_NONE,
} in3_plugin_exec_t;

/** registers a plugin with the client */
in3_ret_t in3_plugin_register(in3_t* c, in3_plugin_supp_acts_t acts, in3_plugin_act_fn action_fn, void* data);

/** executes all plugins irrespective of their return values, returns first error (if any) */
in3_ret_t in3_plugin_execute_all(in3_t* c, in3_plugin_act_t action, void* plugin_ctx);

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

  uint32_t               cache_timeout;        /**< number of seconds requests can be cached. */
  uint16_t               node_limit;           /**< the limit of nodes to store in the client. */
  void*                  key;                  /**< the client key to sign requests (pointer to 32bytes private key seed) */
  uint32_t               max_code_cache;       /**< number of max bytes used to cache the code in memory */
  uint32_t               max_block_cache;      /**< number of number of blocks cached  in memory */
  in3_proof_t            proof;                /**< the type of proof used */
  uint8_t                request_count;        /**< the number of request send when getting a first answer */
  uint8_t                signature_count;      /**< the number of signatures used to proof the blockhash. */
  uint64_t               min_deposit;          /**< min stake of the server. Only nodes owning at least this amount will be chosen. */
  uint8_t                replace_latest_block; /**< if specified, the blocknumber *latest* will be replaced by blockNumber- specified value */
  uint16_t               finality;             /**< the number of signatures in percent required for the request*/
  uint_fast16_t          max_attempts;         /**< the max number of attempts before giving up*/
  uint_fast16_t          max_verified_hashes;  /**< max number of verified hashes to cache */
  uint32_t               timeout;              /**< specifies the number of milliseconds before the request times out. increasing may be helpful if the device uses a slow connection. */
  chain_id_t             chain_id;             /**< servers to filter for the given chain. The chain-id based on EIP-155.*/
  in3_storage_handler_t* cache;                /**< a cache handler offering 2 functions ( setItem(string,string), getItem(string) ) */
  in3_signer_t*          signer;               /**< signer-struct managing a wallet */
  in3_transport_send     transport;            /**< the transport handler sending requests */
  uint_fast8_t           flags;                /**< a bit mask with flags defining the behavior of the incubed client. See the FLAG...-defines*/
  in3_chain_t*           chains;               /**< chain spec and nodeList definitions*/
  uint16_t               chains_length;        /**< number of configured chains */
  in3_filter_handler_t*  filters;              /**< filter handler */
  in3_node_props_t       node_props;           /**< used to identify the capabilities of the node. */
  uint_fast16_t          pending;              /**< number of pending requests created with this instance */
  in3_plugin_t*          plugins;              /**< list of related plugins */

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

/** sends a request and stores the result in the provided buffer */
NONULL in3_ret_t in3_client_rpc_raw(
    in3_t*      c,       /**< [in] the pointer to the incubed client config. */
    const char* request, /**< [in] the rpc request including method and params. */
    char**      result,  /**< [in] pointer to string which will be set if the request was successfull. This will hold the result as json-rpc-string. (make sure you free this after use!) */
    char**      error /**< [in] pointer to a string containg the error-message. (make sure you free it after use!) */);

/** executes a request and returns result as string. in case of an error, the error-property of the result will be set. 
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

/**
 * defines a default transport which is used when creating a new client.
 */
void in3_set_default_transport(
    in3_transport_send transport /**< the default transport-function. */
);

/**
 * defines a default storage handler which is used when creating a new client.
 */
void in3_set_default_storage(
    in3_storage_handler_t* cacheStorage /**< pointer to the handler-struct */
);
/**
 * defines a default signer which is used when creating a new client.
 */
void in3_set_default_signer(
    in3_signer_t* signer /**< default signer-function. */
);

/**
 * create a new signer-object to be set on the client.
 * the caller will need to free this pointer after usage.
 */
NONULL_FOR((1))
in3_signer_t* in3_create_signer(
    in3_sign       sign,       /**< function pointer returning a stored value for the given key.*/
    in3_prepare_tx prepare_tx, /**< function pointer returning capable of manipulating the transaction before signing it. This is needed in order to support multisigs.*/
    void*          wallet      /**<custom object whill will be passed to functions */
);

/**
 * helper function to retrieve and message from a in3_sign_ctx_t
 */
bytes_t in3_sign_ctx_get_message(
    in3_sign_ctx_t* ctx /**< the signer context */
);

/**
 * helper function to retrieve and account from a in3_sign_ctx_t
 */
bytes_t in3_sign_ctx_get_account(
    in3_sign_ctx_t* ctx /**< the signer context */
);

/**
 * helper function to retrieve the signature from a in3_sign_ctx_t
 */
uint8_t* in3_sign_ctx_get_signature(
    in3_sign_ctx_t* ctx /**< the signer context */
);

/**
 * set the transport handler on the client.
 */
void in3_set_transport(
    in3_t*             c,   /**< the incubed client */
    in3_transport_send cptr /**< custom pointer which will will be passed to functions */
);

/**
 * getter to retrieve the payload from a in3_request_t struct
 */
char* in3_get_request_payload(
    in3_request_t* request /**< request struct */
);

/**
 * getter to retrieve the urls list from a in3_request_t struct
 */
char** in3_get_request_urls(
    in3_request_t* request /**< request struct */
);

/**
 * getter to retrieve the urls list length from a in3_request_t struct
 */
int in3_get_request_urls_len(
    in3_request_t* request /**< request struct */
);

/**
 * getter to retrieve the urls list length from a in3_request_t struct
 */
uint32_t in3_get_request_timeout(
    in3_request_t* request /**< request struct */
);
/**
 * set the signer on the client.
 * the caller will need to free this pointer after usage.
 */
in3_signer_t* in3_set_signer(
    in3_t*         c,          /**< the incubed client */
    in3_sign       sign,       /**< function pointer returning a stored value for the given key.*/
    in3_prepare_tx prepare_tx, /**< function pointer returning capable of manipulating the transaction before signing it. This is needed in order to support multisigs.*/
    void*          wallet      /**<custom object whill will be passed to functions */
);
/**
 * create a new storage handler-object to be set on the client.
 * the caller will need to free this pointer after usage.
 */
NONULL_FOR((1, 2, 3, 4))
in3_storage_handler_t* in3_set_storage_handler(
    in3_t*               c,        /**< the incubed client */
    in3_storage_get_item get_item, /**< function pointer returning a stored value for the given key.*/
    in3_storage_set_item set_item, /**< function pointer setting a stored value for the given key.*/
    in3_storage_clear    clear,    /**< function pointer clearing all contents of cache.*/
    void*                cptr      /**< custom pointer which will will be passed to functions */
);

/**
 * adds a response for a request-object.
 * This function should be used in the transport-function to set the response.
 */
NONULL void in3_req_add_response(
    in3_request_t* req,      /**< [in]the the request */
    int            index,    /**< [in] the index of the url, since this request could go out to many urls */
    bool           is_error, /**< [in] if true this will be reported as error. the message should then be the error-message */
    const char*    data,     /**<  the data or the the string*/
    int            data_len  /**<  the length of the data or the the string (use -1 if data is a null terminated string)*/
);

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
