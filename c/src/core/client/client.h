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

// @PUBLIC_HEADER
/** @file
 * this file defines the incubed configuration struct and it registration.
 * 
 * 
 * */

#ifndef CLIENT_H
#define CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../util/bytes.h"
#include "../util/data.h"
#include "../util/error.h"
#include "../util/mem.h"
#include "../util/stringbuilder.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define IN3_PROTO_VER "2.1.0" /**< the protocol version used when sending requests from the this client */

#define CHAIN_ID_MAINNET    0x01 /**< chain_id for mainnet */
#define CHAIN_ID_GOERLI     0x5 /**< chain_id for goerlii */
#define CHAIN_ID_EWC        0xf6 /**< chain_id for ewc */
#define CHAIN_ID_IPFS       0x7d0 /**< chain_id for ipfs */
#define CHAIN_ID_BTC        0x99 /**< chain_id for btc */
#define CHAIN_ID_LOCAL      0x11 /**< chain_id for local chain */
#define DEF_REPL_LATEST_BLK 6 /**< default replace_latest_block */

/**
 * type for a chain_id.
 */
typedef uint32_t chain_id_t;

/** the type of the chain. 
 * 
 * for incubed a chain can be any distributed network or database with incubed support.
 * Depending on this chain-type the previously registered verifier will be chosen and used.
 */
typedef enum {
  CHAIN_ETH       = 0, /**< Ethereum chain */
  CHAIN_SUBSTRATE = 1, /**< substrate chain */
  CHAIN_IPFS      = 2, /**< ipfs verification */
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

/**
 * a list of flags defining the behavior of the incubed client. They should be used as bitmask for the flags-property.
 */
typedef enum {
  FLAGS_KEEP_IN3           = 0x1,  /**< the in3-section with the proof will also returned */
  FLAGS_AUTO_UPDATE_LIST   = 0x2,  /**< the nodelist will be automatically updated if the last_block is newer  */
  FLAGS_INCLUDE_CODE       = 0x4,  /**< the code is included when sending eth_call-requests  */
  FLAGS_BINARY             = 0x8,  /**< the client will use binary format  */
  FLAGS_HTTP               = 0x10, /**< the client will try to use http instead of https  */
  FLAGS_STATS              = 0x20, /**< nodes will keep track of the stats (default=true)  */
  FLAGS_NODE_LIST_NO_SIG   = 0x40, /**< nodelist update request will not automatically ask for signatures and proof */
  FLAGS_BOOT_WEIGHTS       = 0x80, /**< if true the client will initialize the first weights from the nodelist given by the nodelist.*/
  FLAGS_ALLOW_EXPERIMENTAL = 0x100 /**< if true the client will support experimental features.*/
} in3_flags_type_t;

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
  uint8_t              version;         /**< version of the chain */
  chain_id_t           chain_id;        /**< chain_id, which could be a free or based on the public ethereum networkId*/
  in3_chain_type_t     type;            /**< chain-type */
  in3_verified_hash_t* verified_hashes; /**< contains the list of already verified blockhashes */
} in3_chain_t;

#define PLGN_ACT_LIFECYCLE (PLGN_ACT_INIT | PLGN_ACT_TERM)
#define PLGN_ACT_TRANSPORT (PLGN_ACT_TRANSPORT_SEND | PLGN_ACT_TRANSPORT_RECEIVE | PLGN_ACT_TRANSPORT_CLEAN)
#define PLGN_ACT_NODELIST  (PLGN_ACT_NL_PICK | PLGN_ACT_NL_PICK_FOLLOWUP | PLGN_ACT_NL_BLACKLIST | PLGN_ACT_NL_FAILABLE | PLGN_ACT_NL_OFFLINE)
#define PLGN_ACT_CACHE     (PLGN_ACT_CACHE_SET | PLGN_ACT_CACHE_GET | PLGN_ACT_CACHE_CLEAR)
#define PLGN_ACT_CONFIG    (PLGN_ACT_CONFIG_SET | PLGN_ACT_CONFIG_GET)

/** plugin action list */
typedef enum {
  PLGN_ACT_INIT              = 0x1,       /**< initialize plugin - use for allocating/setting-up internal resources . Plugins will be initialized before first used. The plgn_ctx will be the first request ctx in3_req_t */
  PLGN_ACT_TERM              = 0x2,       /**< terminate plugin - use for releasing internal resources and cleanup. */
  PLGN_ACT_TRANSPORT_SEND    = 0x4,       /**< sends out a request - the transport plugin will receive a request_t as plgn_ctx, it may set a cptr which will be passed back when fetching more responses. */
  PLGN_ACT_TRANSPORT_RECEIVE = 0x8,       /**< fetch next response - the transport plugin will receive a request_t as plgn_ctx, which contains a cptr  if set previously*/
  PLGN_ACT_TRANSPORT_CLEAN   = 0x10,      /**< free-up transport resources - the transport plugin will receive a request_t as plgn_ctx if the cptr was set.*/
  PLGN_ACT_SIGN_ACCOUNT      = 0x20,      /**< returns the default account of the signer */
  PLGN_ACT_SIGN_PREPARE      = 0x40,      /**< allows a wallet to manipulate the payload before signing - the plgn_ctx will be in3_sign_ctx_t. This way a tx can be send through a multisig */
  PLGN_ACT_SIGN              = 0x80,      /**< signs the payload - the plgn_ctx will be in3_sign_ctx_t.  */
  PLGN_ACT_RPC_HANDLE        = 0x100,     /**< a plugin may respond to a rpc-request directly (without sending it to the node). */
  PLGN_ACT_RPC_VERIFY        = 0x200,     /**< verifies the response. the plgn_ctx will be a in3_vctx_t holding all data */
  PLGN_ACT_CACHE_SET         = 0x400,     /**< stores data to be reused later - the plgn_ctx will be a in3_cache_ctx_t containing the data */
  PLGN_ACT_CACHE_GET         = 0x800,     /**< reads data to be previously stored - the plgn_ctx will be a in3_cache_ctx_t containing the key. if the data was found the data-property needs to be set. */
  PLGN_ACT_CACHE_CLEAR       = 0x1000,    /**< clears all stored data - plgn_ctx will be NULL  */
  PLGN_ACT_CONFIG_SET        = 0x2000,    /**< gets a config-token and reads data from it */
  PLGN_ACT_CONFIG_GET        = 0x4000,    /**< gets a string-builder and adds all config to it. */
  PLGN_ACT_PAY_PREPARE       = 0x8000,    /**< prepares a payment */
  PLGN_ACT_PAY_FOLLOWUP      = 0x10000,   /**< called after a request to update stats. */
  PLGN_ACT_PAY_HANDLE        = 0x20000,   /**< handles the payment */
  PLGN_ACT_PAY_SIGN_REQ      = 0x40000,   /**< signs a request */
  PLGN_ACT_LOG_ERROR         = 0x80000,   /**< report an error */
  PLGN_ACT_NL_PICK           = 0x100000,  /**< picks the data nodes, plgn_ctx will be a pointer to in3_req_t */
  PLGN_ACT_NL_PICK_FOLLOWUP  = 0x200000,  /**< called after receiving a response in order to decide whether a update is needed, plgn_ctx will be a pointer to in3_req_t */
  PLGN_ACT_NL_BLACKLIST      = 0x400000,  /**< blacklist a particular node in the nodelist, plgn_ctx will be a pointer to the node's address. */
  PLGN_ACT_NL_FAILABLE       = 0x800000,  /**< handle fail-able request, plgn_ctx will be a pointer to in3_req_t */
  PLGN_ACT_NL_OFFLINE        = 0x1000000, /**< mark a particular node in the nodelist as offline, plgn_ctx will be a pointer to in3_nl_offline_ctx_t. */
  PLGN_ACT_CHAIN_CHANGE      = 0x2000000, /**< chain id change event, called after setting new chain id */
  PLGN_ACT_GET_DATA          = 0x4000000, /**< get access to plugin data as a void ptr */
  PLGN_ACT_ADD_PAYLOAD       = 0x8000000, /**< add plugin specific metadata to payload, plgn_ctx will be a sb_t pointer, make sure to begin with a comma */
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

/** Incubed Configuration.
 * 
 * This struct holds the configuration and also point to internal resources such as filters or chain configs.
 * 
 */
typedef struct in3_t_ {
  uint8_t                signature_count;       /**< the number of signatures used to proof the blockhash. */
  uint8_t                replace_latest_block;  /**< if specified, the blocknumber *latest* will be replaced by blockNumber- specified value */
  uint_fast16_t          flags;                 /**< a bit mask with flags defining the behavior of the incubed client. See the FLAG...-defines*/
  uint16_t               finality;              /**< the number of signatures in percent required for the request*/
  uint_fast16_t          max_attempts;          /**< the max number of attempts before giving up*/
  uint_fast16_t          max_verified_hashes;   /**< max number of verified hashes to cache (actual number may temporarily exceed this value due to pending requests) */
  uint_fast16_t          alloc_verified_hashes; /**< number of currently allocated verified hashes */
  uint_fast16_t          pending;               /**< number of pending requests created with this instance */
  uint32_t               cache_timeout;         /**< number of seconds requests can be cached. */
  uint32_t               timeout;               /**< specifies the number of milliseconds before the request times out. increasing may be helpful if the device uses a slow connection. */
  uint32_t               id_count;              /**< counter for use as JSON RPC id - incremented for every request */
  in3_plugin_supp_acts_t plugin_acts;           /**< bitmask of supported actions of all plugins registered with this client */
  in3_proof_t            proof;                 /**< the type of proof used */
  in3_chain_t            chain;                 /**< chain spec and nodeList definitions*/
  in3_plugin_t*          plugins;               /**< list of registered plugins */
} in3_t;

/** creates a new Incubed configuration for a specified chain and returns the pointer.
 * when creating the client only the one chain will be configured. (saves memory). 
 * but if you pass `CHAIN_ID_MULTICHAIN` as argument all known chains will be configured allowing you to switch between chains within the same client or configuring your own chain. 
 * 
 * you need to free this instance with `in3_free` after use!
 * 
 * Before using the client you still need to set the transport and optional the storage handlers:
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
    const char* method, /**< [in] the name of the rpc-function to call. */
    const char* params, /**< [in] docs for input parameter v. */
    char**      result, /**< [in] pointer to string which will be set if the request was successful. This will hold the result as json-rpc-string. (make sure you free this after use!) */
    char**      error /**< [in] pointer to a string containing the error-message. (make sure you free it after use!) */);

/** sends a request and stores the result in the provided buffer, this method will always return the first, so bulk-requests are not supported. */
NONULL in3_ret_t in3_client_rpc_raw(
    in3_t*      c,       /**< [in] the pointer to the incubed client config. */
    const char* request, /**< [in] the rpc request including method and params. */
    char**      result,  /**< [in] pointer to string which will be set if the request was successful. This will hold the result as json-rpc-string. (make sure you free this after use!) */
    char**      error /**< [in] pointer to a string containing the error-message. (make sure you free it after use!) */);

/** executes a request and returns result as string. in case of an error, the error-property of the result will be set. 
 * This function also supports sending bulk-requests, but you can not mix internal and external calls, since bulk means all requests will be send to picked nodes.
 * The resulting string must be free by the the caller of this function! 
 */
NONULL char* in3_client_exec_req(
    in3_t* c,  /**< [in] the pointer to the incubed client config. */
    char*  req /**< [in] the request as rpc. */
);

/** registers a new chain or replaces a existing (but keeps the nodelist)*/
NONULL
in3_ret_t in3_client_register_chain(
    in3_t*           client,   /**< [in] the pointer to the incubed client config. */
    chain_id_t       chain_id, /**< [in] the chain id. */
    in3_chain_type_t type,     /**< [in] the verification type of the chain. */
    uint8_t          version   /**< [in] the chain version. */
);

/** frees the references of the client */
NONULL void in3_free(in3_t* a /**< [in] the pointer to the incubed client config to free. */);

/**
 * configures the client based on a json-config.
 * 
 * For details about the structure of the config see https://in3.readthedocs.io/en/develop/api-ts.html#type-in3config
 * Returns NULL on success, and error string on failure (to be freed by caller) - in which case the client state is undefined
 */
NONULL char* in3_configure(
    in3_t*      c,     /**< the incubed client */
    const char* config /**< JSON-string with the configuration to set. */
);

/**
 * gets the current config as json.
 * 
 * For details about the structure of the config see https://in3.readthedocs.io/en/develop/api-ts.html#type-in3config
 */
NONULL char* in3_get_config(
    in3_t* c /**< the incubed client */
);

/** a register-function for a plugin.
 */
typedef in3_ret_t (*plgn_register)(in3_t* c);

#define assert_in3(c)                                  \
  assert(c);                                           \
  assert((c)->chain.chain_id);                         \
  assert((c)->plugins);                                \
  assert((c)->max_attempts > 0);                       \
  assert((c)->proof >= 0 && (c)->proof <= PROOF_FULL); \
  assert((c)->proof >= 0 && (c)->proof <= PROOF_FULL);

#ifdef __cplusplus
}
#endif

#endif
