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
#include "../util/stringbuilder.h"
#include <stdbool.h>
#include <stdint.h>

/** the protocol version used when sending requests from the this client */
#define IN3_PROTO_VER "2.1.0"

#define ETH_CHAIN_ID_MULTICHAIN 0x0 /**< chain_id working with all known chains */
#define ETH_CHAIN_ID_MAINNET 0x01   /**< chain_id for mainnet */
#define ETH_CHAIN_ID_KOVAN 0x2a     /**< chain_id for kovan */
#define ETH_CHAIN_ID_TOBALABA 0x44d /**< chain_id for tobalaba */
#define ETH_CHAIN_ID_GOERLI 0x5     /**< chain_id for goerlii */
#define ETH_CHAIN_ID_EVAN 0x4b1     /**< chain_id for evan */
#define ETH_CHAIN_ID_IPFS 0x7d0     /**< chain_id for ipfs */
#define ETH_CHAIN_ID_VOLTA 0x12046  /**< chain_id for volta */
#define ETH_CHAIN_ID_LOCAL 0xFFFF   /**< chain_id for local chain */

/**
 * type for a chain_id.
 */
typedef uint32_t chain_id_t;

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

/** verification as delivered by the server. 
 * 
 * This will be part of the in3-request and will be generated based on the prooftype.*/
typedef enum {
  VERIFICATION_NEVER = 0, /**< No Verifacation */
  VERIFICATION_PROOF = 1, /**< Includes the proof of the data */
} in3_verification_t;

/** the configuration as part of each incubed request. 
 * This will be generated for each request based on the client-configuration. the verifier may access this during verification in order to check against the request. 
 * 
 */
typedef struct in3_request_config {
  chain_id_t         chain_id;               /**< the chain to be used. this is holding the integer-value of the hexstring. */
  uint8_t            include_code;           /**< if true the code needed will always be devlivered.  */
  uint8_t            use_full_proof;         /**< this flaqg is set, if the proof is set to "PROOF_FULL" */
  uint8_t            use_binary;             /**< this flaqg is set, the client should use binary-format */
  bytes_t*           verified_hashes;        /**< a list of blockhashes already verified. The Server will not send any proof for them again . */
  uint16_t           verified_hashes_length; /**< number of verified blockhashes*/
  uint16_t           latest_block;           /**< the last blocknumber the nodelistz changed */
  uint16_t           finality;               /**< number of signatures( in percent) needed in order to reach finality. */
  in3_verification_t verification;           /**< Verification-type */
  bytes_t*           client_signature;       /**< the signature of the client with the client key */
  bytes_t*           signers;                /**< the addresses of servers requested to sign the blockhash */
  uint8_t            signers_length;         /**< number or addresses */

} in3_request_config_t;

/**
 * Node capabilities
 * @note Always access using getters/setters in nodelist.h
 */
typedef uint64_t in3_node_props_t;

typedef enum {
  NODE_PROP_PROOF            = 0x1,   /* filter out nodes which are providing no proof */
  NODE_PROP_MULTICHAIN       = 0x2,   /* filter out nodes other then which have capability of the same RPC endpoint may also accept requests for different chains */
  NODE_PROP_ARCHIVE          = 0x4,   /* filter out non-archive supporting nodes */
  NODE_PROP_HTTP             = 0x8,   /* filter out non-http nodes  */
  NODE_PROP_BINARY           = 0x10,  /* filter out nodes that don't support binary encoding */
  NODE_PROP_ONION            = 0x20,  /* filter out non-onion nodes */
  NODE_PROP_SIGNER           = 0x40,  /* filter out non-signer nodes */
  NODE_PROP_DATA             = 0x80,  /* filter out non-data provider nodes */
  NODE_PROP_STATS            = 0x100, /* filter out nodes that do not provide stats */
  NODE_PROP_MIN_BLOCK_HEIGHT = 0x400, /* filter out nodes that will sign blocks with lower min block height than specified */
} in3_node_props_type_t;

/** incubed node-configuration. 
 * 
 * These information are read from the Registry contract and stored in this struct representing a server or node.
 */
typedef struct in3_node {
  bytes_t*         address;     /**< address of the server */
  uint64_t         deposit;     /**< the deposit stored in the registry contract, which this would lose if it sends a wrong blockhash */
  uint32_t         index;       /**< index within the nodelist, also used in the contract as key */
  uint32_t         capacity;    /**< the maximal capacity able to handle */
  in3_node_props_t props;       /**< used to identify the capabilities of the node. See in3_node_props_type_t in nodelist.h */
  char*            url;         /**< the url of the node */
  bool             whitelisted; /**< boolean indicating if node exists in whiteList */
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
  float    weight;              /**< current weight*/
} in3_node_weight_t;

/**
 * Initializer for in3_node_props_t
 */
#define in3_node_props_init(np) *(np) = 0

/**
 * setter method for interacting with in3_node_props_t.
 * @param[out] node_props
 * @param type
 * @param
 */
void in3_node_props_set(in3_node_props_t*     node_props,
                        in3_node_props_type_t type,
                        uint8_t               value);

/**
 * returns the value of the specified propertytype.
 * @param np the properties as defined in the nodeList 
 * @param t the value to extract
 * @return value as a number
 */
static inline uint32_t in3_node_props_get(in3_node_props_t np, in3_node_props_type_t t) {
  return ((t == NODE_PROP_MIN_BLOCK_HEIGHT) ? ((np >> 32U) & 0xFFU) : !!(np & t));
}

/**
 * checkes if the given type is set in the properties
 * @param np the properties as defined in the nodeList 
 * @param t the value to extract
 * @return true if set
 */
static inline bool in3_node_props_matches(in3_node_props_t np, in3_node_props_type_t t) {
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

/**
 * Chain definition inside incubed.
 * 
 * for incubed a chain can be any distributed network or database with incubed support.
 */
typedef struct in3_chain {
  chain_id_t         chain_id;        /**< chain_id, which could be a free or based on the public ethereum networkId*/
  in3_chain_type_t   type;            /**< chaintype */
  uint64_t           last_block;      /**< last blocknumber the nodeList was updated, which is used to detect changed in the nodelist*/
  bool               needs_update;    /**< if true the nodelist should be updated and will trigger a `in3_nodeList`-request before the next request is send. */
  int                nodelist_length; /**< number of nodes in the nodeList */
  in3_node_t*        nodelist;        /**< array of nodes */
  in3_node_weight_t* weights;         /**< stats and weights recorded for each node */
  bytes_t**          init_addresses;  /**< array of addresses of nodes that should always part of the nodeList */
  bytes_t*           contract;        /**< the address of the registry contract */
  bytes32_t          registry_id;     /**< the identifier of the registry */
  uint8_t            version;         /**< version of the chain */
  in3_whitelist_t*   whitelist;       /**< if set the whitelist of the addresses. */
} in3_chain_t;

/** 
 * storage handler function for reading from cache.
 * @returns the found result. if the key is found this function should return the values as bytes otherwise `NULL`.
 **/
typedef bytes_t* (*in3_storage_get_item)(
    void* cptr, /**< a custom pointer as set in the storage handler*/
    char* key   /**< the key to search in the cache */
);

/** 
 * storage handler function for writing to the cache.
 **/
typedef void (*in3_storage_set_item)(
    void*    cptr, /**< a custom pointer as set in the storage handler*/
    char*    key,  /**< the key to store the value.*/
    bytes_t* value /**< the value to store.*/
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

#define IN3_SIGN_ERR_REJECTED -1          /**< return value used by the signer if the the signature-request was rejected. */
#define IN3_SIGN_ERR_ACCOUNT_NOT_FOUND -2 /**< return value used by the signer if the requested account was not found. */
#define IN3_SIGN_ERR_INVALID_MESSAGE -3   /**< return value used by the signer if the message was invalid. */
#define IN3_SIGN_ERR_GENERAL_ERROR -4     /**< return value used by the signer for unspecified errors. */

/** type of the requested signature */
typedef enum {
  SIGN_EC_RAW  = 0, /**< sign the data directly */
  SIGN_EC_HASH = 1, /**< hash and sign the data */
} d_signature_type_t;

/** 
 * signing function.
 * 
 * signs the given data and write the signature to dst.
 * the return value must be the number of bytes written to dst.
 * In case of an error a negativ value must be returned. It should be one of the IN3_SIGN_ERR... values.
 * 
*/
typedef in3_ret_t (*in3_sign)(void* ctx, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst);

/** 
 * transform transaction function.
 * 
 * for multisigs, we need to change the transaction to gro through the ms.
 * if the new_tx is not set within the function, it will use the old_tx.
 * 
*/
typedef in3_ret_t (*in3_prepare_tx)(void* ctx, d_token_t* old_tx, json_ctx_t** new_tx);

typedef struct in3_signer {
  /* function pointer returning a stored value for the given key.*/
  in3_sign sign;

  /* function pointer returning capable of manipulating the transaction before signing it. This is needed in order to support multisigs.*/
  in3_prepare_tx prepare_tx;

  /* custom object whill will be passed to functions */
  void* wallet;

} in3_signer_t;

/** response-object. 
 * 
 * if the error has a length>0 the response will be rejected
 */
typedef struct n3_response {
  sb_t error;  /**< a stringbuilder to add any errors! */
  sb_t result; /**< a stringbuilder to add the result */
} in3_response_t;

/** request-object. 
 * 
 * represents a RPC-request
 */
typedef struct n3_request {
  char*           payload;  /**< the payload to send */
  char**          urls;     /**< array of urls */
  int             urls_len; /**< number of urls */
  in3_response_t* results;  /** the responses*/
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
  /** filter type: (event, block or pending) */
  in3_filter_type_t type;

  /** associated filter options */
  char* options;

  /** block no. when filter was created OR eth_getFilterChanges was called */
  uint64_t last_block;

  /** method to release owned resources */
  void (*release)(struct in3_filter_t_* f);
} in3_filter_t;

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
typedef struct in3_t_ {
  /** number of seconds requests can be cached. */
  uint32_t cache_timeout;

  /** the limit of nodes to store in the client. */
  uint16_t node_limit;

  /** the client key to sign requests */
  bytes_t* key;

  /** number of max bytes used to cache the code in memory */
  uint32_t max_code_cache;

  /** number of number of blocks cached  in memory */
  uint32_t max_block_cache;

  /** the type of proof used */
  in3_proof_t proof;

  /** the number of request send when getting a first answer */
  uint8_t request_count;

  /** the number of signatures used to proof the blockhash. */
  uint8_t signature_count;

  /** min stake of the server. Only nodes owning at least this amount will be chosen. */
  uint64_t min_deposit;

  /** if specified, the blocknumber *latest* will be replaced by blockNumber- specified value */
  uint16_t replace_latest_block;

  /** the number of signatures in percent required for the request*/
  uint16_t finality;

  /** the max number of attempts before giving up*/
  uint16_t max_attempts;

  /** specifies the number of milliseconds before the request times out. increasing may be helpful if the device uses a slow connection. */
  uint32_t timeout;

  /** servers to filter for the given chain. The chain-id based on EIP-155.*/
  chain_id_t chain_id;

  /** if true the nodelist will be automaticly updated if the last_block is newer */
  uint8_t auto_update_list;

  /** a cache handler offering 2 functions ( setItem(string,string), getItem(string) ) */
  in3_storage_handler_t* cache;

  /** signer-struct managing a wallet */
  in3_signer_t* signer;

  /** the transporthandler sending requests */
  in3_transport_send transport;

  /** includes the code when sending eth_call-requests */
  uint8_t include_code;

  /** if true the client will use binary format*/
  uint8_t use_binary;

  /** if true the client will try to use http instead of https*/
  uint8_t use_http;

  /** if true the in3-section with the proof will also returned*/
  uint8_t keep_in3;

  /** chain spec and nodeList definitions*/
  in3_chain_t* chains;

  /** number of configured chains */
  uint16_t chains_length;

  /** filter handler */
  in3_filter_handler_t* filters;

  /** used to identify the capabilities of the node. */
  in3_node_props_t node_props;

} in3_t;

/** creates a new Incubes configuration and returns the pointer.
 * 
 * This Method is depricated. you should use `in3_for_chain(ETH_CHAIN_ID_MULTICHAIN)` instead.
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
 * // configure storage...
 * in3_storage_handler_t storage_handler;
 * storage_handler.get_item = storage_get_item;
 * storage_handler.set_item = storage_set_item;
 * storage_handler.clear = storage_clear;
 *
 * // configure transport
 * client->transport    = send_curl;
 *
 * // configure storage
 * client->cache = &storage_handler;
 * 
 * // init cache
 * in3_cache_init(client);
 * 
 * // ready to use ...
 * ```
 * 
 * @returns the incubed instance.
 */
in3_t* in3_new() __attribute__((deprecated("use in3_for_chain(ETH_CHAIN_ID_MULTICHAIN)")));

/** creates a new Incubes configuration for a specified chain and returns the pointer.
 * when creating the client only the one chain will be configured. (saves memory). 
 * but if you pass `ETH_CHAIN_ID_MULTICHAIN` as argument all known chains will be configured allowing you to switch between chains within the same client or configuring your own chain. 
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
 * in3_t* client = in3_for_chain(ETH_CHAIN_ID_MAINNET);
 * 
 * // configure storage...
 * in3_storage_handler_t storage_handler;
 * storage_handler.get_item = storage_get_item;
 * storage_handler.set_item = storage_set_item;
 * storage_handler.clear = storage_clear;
 *
 * // configure transport
 * client->transport    = send_curl;
 *
 * // configure storage
 * client->cache = &storage_handler;
 * 
 * // init cache
 * in3_cache_init(client);
 * 
 * // ready to use ...
 * ```
 * ** This Method is depricated. you should use `in3_for_chain` instead.**
 * 
 * @returns the incubed instance.
 */
in3_t* in3_for_chain(
    chain_id_t chain_id /**< the chain_id (see ETH_CHAIN_ID_... constants). */
);

/** sends a request and stores the result in the provided buffer */
in3_ret_t in3_client_rpc(
    in3_t* c,      /**< [in] the pointer to the incubed client config. */
    char*  method, /**< [in] the name of the rpc-funcgtion to call. */
    char*  params, /**< [in] docs for input parameter v. */
    char** result, /**< [in] pointer to string which will be set if the request was successfull. This will hold the result as json-rpc-string. (make sure you free this after use!) */
    char** error /**< [in] pointer to a string containg the error-message. (make sure you free it after use!) */);

/** executes a request and returns result as string. in case of an error, the error-property of the result will be set. 
 * The resulting string must be free by the the caller of this function! 
 */
char* in3_client_exec_req(
    in3_t* c,  /**< [in] the pointer to the incubed client config. */
    char*  req /**< [in] the request as rpc. */
);

/**
 * adds a response for a request-object.
 * This function should be used in the transport-function to set the response.
 */
void in3_req_add_response(
    in3_response_t* res,      /**< [in] the response-pointer */
    int             index,    /**< [in] the index of the url, since this request could go out to many urls */
    bool            is_error, /**< [in] if true this will be reported as error. the message should then be the error-message */
    void*           data,     /**<  the data or the the string*/
    int             data_len  /**<  the length of the data or the the string (use -1 if data is a null terminated string)*/
);

/** registers a new chain or replaces a existing (but keeps the nodelist)*/
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
in3_ret_t in3_client_add_node(
    in3_t*           client,   /**< [in] the pointer to the incubed client config. */
    chain_id_t       chain_id, /**< [in] the chain id. */
    char*            url,      /**< [in] url of the nodes. */
    in3_node_props_t props,    /**< [in]properties of the node. */
    address_t        address);        /**< [in] public address of the signer. */

/** removes a node from a nodelist */
in3_ret_t in3_client_remove_node(
    in3_t*     client,   /**< [in] the pointer to the incubed client config. */
    chain_id_t chain_id, /**< [in] the chain id. */
    address_t  address);  /**< [in] public address of the signer. */

/** removes all nodes from the nodelist */
in3_ret_t in3_client_clear_nodes(
    in3_t*     client,    /**< [in] the pointer to the incubed client config. */
    chain_id_t chain_id); /**< [in] the chain id. */

/** frees the references of the client */
void in3_free(in3_t* a /**< [in] the pointer to the incubed client config to free. */);

/**
 * inits the cache.
 *
 * this will try to read the nodelist from cache.
 */
in3_ret_t in3_cache_init(
    in3_t* c /**< the incubed client */
);

/**
 * finds the chain-config for the given chain_id.
 * 
 * My return NULL if not found.
 */
in3_chain_t* in3_find_chain(
    in3_t*     c /**< the incubed client */,
    chain_id_t chain_id /**< chain_id */
);

/**
 * configures the clent based on a json-config.
 * 
 * For details about the structure of ther config see https://in3.readthedocs.io/en/develop/api-ts.html#type-in3config
 * 
 */
in3_ret_t in3_configure(
    in3_t* c,     /**< the incubed client */
    char*  config /**< JSON-string with the configuration to set. */
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
in3_signer_t* in3_create_signer(
    in3_sign       sign,       /**< function pointer returning a stored value for the given key.*/
    in3_prepare_tx prepare_tx, /**< function pointer returning capable of manipulating the transaction before signing it. This is needed in order to support multisigs.*/
    void*          wallet      /**<custom object whill will be passed to functions */
);

/**
 * create a new storage handler-object to be set on the client.
 * the caller will need to free this pointer after usage.
 */
in3_storage_handler_t* in3_create_storage_handler(
    in3_storage_get_item get_item, /**< function pointer returning a stored value for the given key.*/
    in3_storage_set_item set_item, /**< function pointer setting a stored value for the given key.*/
    in3_storage_clear    clear,    /**< function pointer clearing all contents of cache.*/
    void*                cptr      /**< custom pointer which will will be passed to functions */
);
#endif
