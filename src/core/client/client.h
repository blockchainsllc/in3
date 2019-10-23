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
 * incubed main client file.
 * 
 * This includes the definition of the client and used enum values.
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

#define IN3_PROTO_VER_MAJOR 0x2
#define IN3_PROTO_VER_MINOR 0x0

#define ETH_CHAIN_ID_MAINNET 0x01L
#define ETH_CHAIN_ID_KOVAN 0x2aL
#define ETH_CHAIN_ID_TOBALABA 0x44dL
#define ETH_CHAIN_ID_GOERLI 0x5L
#define ETH_CHAIN_ID_EVAN 0x4b1L
#define ETH_CHAIN_ID_IPFS 0x7d0
#define ETH_CHAIN_ID_VOLTA 0x12046
#define ETH_CHAIN_ID_LOCAL 0xFFFFL

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
  CHAIN_IOTA      = 4, /**< IOTA chain */
  CHAIN_GENERIC   = 5, /**< other chains */
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
  VERIFICATION_NEVER                = 0, /**< No Verifacation */
  VERIFICATION_PROOF                = 1, /**< Includes the proof of the data */
  VERIFICATION_PROOF_WITH_SIGNATURE = 2  /**< Proof + Signatures */
} in3_verification_t;

/** the configuration as part of each incubed request. 
 * This will be generated for each request based on the client-configuration. the verifier may access this during verification in order to check against the request. 
 * 
 */
typedef struct in3_request_config {
  uint64_t           chainId;             /**< the chain to be used. this is holding the integer-value of the hexstring. */
  uint8_t            includeCode;         /**< if true the code needed will always be devlivered.  */
  uint8_t            useFullProof;        /**< this flaqg is set, if the proof is set to "PROOF_FULL" */
  uint8_t            useBinary;           /**< this flaqg is set, the client should use binary-format */
  bytes_t*           verifiedHashes;      /**< a list of blockhashes already verified. The Server will not send any proof for them again . */
  uint16_t           verifiedHashesCount; /**< number of verified blockhashes*/
  uint16_t           latestBlock;         /**< the last blocknumber the nodelistz changed */
  uint16_t           finality;            /**< number of signatures( in percent) needed in order to reach finality. */
  in3_verification_t verification;        /**< Verification-type */
  bytes_t*           clientSignature;     /**< the signature of the client with the client key */
  bytes_t*           signatures;          /**< the addresses of servers requested to sign the blockhash */
  uint8_t            signaturesCount;     /**< number or addresses */

} in3_request_config_t;

/** incubed node-configuration. 
 * 
 * These information are read from the Registry contract and stored in this struct representing a server or node.
 */
typedef struct in3_node {
  uint32_t index;    /**< index within the nodelist, also used in the contract as key */
  bytes_t* address;  /**< address of the server */
  uint64_t deposit;  /**< the deposit stored in the registry contract, which this would lose if it sends a wrong blockhash */
  uint32_t capacity; /**< the maximal capacity able to handle */
  uint64_t props;    /**< a bit set used to identify the cabalilities of the server. */
  char*    url;      /**< the url of the node */
} in3_node_t;

/**
 * Weight or reputation of a node.
 * 
 * Based on the past performance of the node a weight is calulcated given faster nodes a heigher weight 
 * and chance when selecting the next node from the nodelist.
 * These weights will also be stored in the cache (if available)
 */
typedef struct in3_node_weight {
  float    weight;              /**< current weight*/
  uint32_t response_count;      /**< counter for responses */
  uint32_t total_response_time; /**< total of all response times */
  uint64_t blacklistedUntil;    /**< if >0 this node is blacklisted until k. k is a unix timestamp */
} in3_node_weight_t;

/**
 * Chain definition inside incubed.
 * 
 * for incubed a chain can be any distributed network or database with incubed support.
 */
typedef struct in3_chain {
  uint64_t           chainId;        /**< chainId, which could be a free or based on the public ethereum networkId*/
  in3_chain_type_t   type;           /**< chaintype */
  uint64_t           lastBlock;      /**< last blocknumber the nodeList was updated, which is used to detect changed in the nodelist*/
  bool               needsUpdate;    /**< if true the nodelist should be updated and will trigger a `in3_nodeList`-request before the next request is send. */
  int                nodeListLength; /**< number of nodes in the nodeList */
  in3_node_t*        nodeList;       /**< array of nodes */
  in3_node_weight_t* weights;        /**< stats and weights recorded for each node */
  bytes_t**          initAddresses;  /**< array of addresses of nodes that should always part of the nodeList */
  bytes_t*           contract;       /**< the address of the registry contract */
  bytes32_t          registry_id;    /**< the identifier of the registry */
  uint8_t            version;        /**< version of the chain */
  json_ctx_t*        spec;           /**< optional chain specification, defining the transaitions and forks*/
} in3_chain_t;

/** 
 * storage handler function for reading from cache.
 * @returns the found result. if the key is found this function should return the values as bytes otherwise `NULL`.
 **/
typedef bytes_t* (*in3_storage_get_item)(
    void* cptr, /**< a custom pointer as set in the storage handler*/
    char* key /**< the key to search in the cache */);

/** 
 * storage handler function for writing to the cache.
 **/
typedef void (*in3_storage_set_item)(
    void*    cptr, /**< a custom pointer as set in the storage handler*/
    char*    key,  /**< the key to store the value.*/
    bytes_t* value /**< the value to store.*/);

/** 
 * storage handler to handle cache.
 **/
typedef struct in3_storage_handler {
  in3_storage_get_item get_item; /**< function pointer returning a stored value for the given key.*/
  in3_storage_set_item set_item; /**< function pointer setting a stored value for the given key.*/
  void*                cptr;     /**< custom pointer which will will be passed to functions */
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
typedef in3_ret_t (*in3_sign)(void* wallet, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst);

typedef struct in3_signer {
  /* function pointer returning a stored value for the given key.*/
  in3_sign sign;

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

/** the transport function to be implemented by the transport provider.
 */
typedef in3_ret_t (*in3_transport_send)(char** urls, int urls_len, char* payload, in3_response_t* results);

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
  uint32_t cacheTimeout;

  /** the limit of nodes to store in the client. */
  uint16_t nodeLimit;

  /** the client key to sign requests */
  bytes_t* key;

  /** number of max bytes used to cache the code in memory */
  uint32_t maxCodeCache;

  /** number of number of blocks cached  in memory */
  uint32_t maxBlockCache;

  /** the type of proof used */
  in3_proof_t proof;

  /** the number of request send when getting a first answer */
  uint8_t requestCount;

  /** the number of signatures used to proof the blockhash. */
  uint8_t signatureCount;

  /** min stake of the server. Only nodes owning at least this amount will be chosen. */
  uint64_t minDeposit;

  /** if specified, the blocknumber *latest* will be replaced by blockNumber- specified value */
  uint16_t replaceLatestBlock;

  /** the number of signatures in percent required for the request*/
  uint16_t finality;

  /** the max number of attempts before giving up*/
  uint16_t max_attempts;

  /** specifies the number of milliseconds before the request times out. increasing may be helpful if the device uses a slow connection. */
  uint32_t timeout;

  /** servers to filter for the given chain. The chain-id based on EIP-155.*/
  uint64_t chainId;

  /** if true the nodelist will be automaticly updated if the lastBlock is newer */
  uint8_t autoUpdateList;

  /** a cache handler offering 2 functions ( setItem(string,string), getItem(string) ) */
  in3_storage_handler_t* cacheStorage;

  /** signer-struct managing a wallet */
  in3_signer_t* signer;

  /** the transporthandler sending requests */
  in3_transport_send transport;

  /** includes the code when sending eth_call-requests */
  uint8_t includeCode;

  /** if true the client will use binary format*/
  uint8_t use_binary;

  /** if true the client will try to use http instead of https*/
  uint8_t use_http;

  /** chain spec and nodeList definitions*/
  in3_chain_t* chains;

  /** number of configured chains */
  uint16_t chainsCount;

  /** filter handler */
  in3_filter_handler_t* filters;

} in3_t;

/** creates a new Incubes configuration and returns the pointer.
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
 *
 * // configure transport
 * client->transport    = send_curl;
 *
 * // configure storage
 * client->cacheStorage = &storage_handler;
 * 
 * // init cache
 * in3_cache_init(client);
 * 
 * // ready to use ...
 * ```
 * 
 * @returns the incubed instance.
 */
in3_t* in3_new();

/** sends a request and stores the result in the provided buffer */
in3_ret_t in3_client_rpc(
    in3_t* c,      /**< [in] the pointer to the incubed client config. */
    char*  method, /**< [in] the name of the rpc-funcgtion to call. */
    char*  params, /**< [in] docs for input parameter v. */
    char** result, /**< [in] pointer to string which will be set if the request was successfull. This will hold the result as json-rpc-string. (make sure you free this after use!) */
    char** error /**< [in] pointer to a string containg the error-message. (make sure you free it after use!) */);

/** registers a new chain or replaces a existing (but keeps the nodelist)*/
in3_ret_t in3_client_register_chain(
    in3_t*           client,      /**< [in] the pointer to the incubed client config. */
    uint64_t         chain_id,    /**< [in] the chain id. */
    in3_chain_type_t type,        /**< [in] the verification type of the chain. */
    address_t        contract,    /**< [in] contract of the registry. */
    bytes32_t        registry_id, /**< [in] the identifier of the registry. */
    uint8_t          version,     /**< [in] the chain version. */
    json_ctx_t*      spec         /**< [in] chainspec or NULL. */
);

/** adds a node to a chain ore updates a existing node */
in3_ret_t in3_client_add_node(
    in3_t*    client,   /**< [in] the pointer to the incubed client config. */
    uint64_t  chain_id, /**< [in] the chain id. */
    char*     url,      /**< [in] url of the nodes. */
    uint64_t  props,    /**< [in]properties of the node. */
    address_t address); /**< [in] public address of the signer. */

/** removes a node from a nodelist */
in3_ret_t in3_client_remove_node(
    in3_t*    client,   /**< [in] the pointer to the incubed client config. */
    uint64_t  chain_id, /**< [in] the chain id. */
    address_t address); /**< [in] public address of the signer. */

/** removes all nodes from the nodelist */
in3_ret_t in3_client_clear_nodes(
    in3_t*   client,    /**< [in] the pointer to the incubed client config. */
    uint64_t chain_id); /**< [in] the chain id. */

/** frees the references of the client */
void in3_free(in3_t* a /**< [in] the pointer to the incubed client config to free. */);

/**
 * inits the cache.
 *
 */
in3_ret_t in3_cache_init(in3_t* c /**< the incubed client */);

/**
 * configures the clent based on a json-config.
 * 
 * For details about the structure of ther config see https://in3.readthedocs.io/en/develop/api-ts.html#type-in3config
 * 
 */
in3_ret_t in3_configure(in3_t* c, char* config);

/**
 * defines a default transport which is used when creating a new client.
 */
void in3_set_default_transport(in3_transport_send transport);

/**
 * defines a default storage handler which is used when creating a new client.
 */
void in3_set_default_storage(in3_storage_handler_t* cacheStorage);
/**
 * defines a default signer which is used when creating a new client.
 */
void in3_set_default_signer(in3_signer_t* signer);

#endif
