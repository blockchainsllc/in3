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
 * this file defines the plugin-contexts
 * 
 * 
 * */

#ifndef PLUGIN_H
#define PLUGIN_H

#include "client.h"
#include "context.h"

// ---------- plugin management -----------------

/** checks if a plugin for specified action is registered with the client */
#define in3_plugin_is_registered(client, action) ((client)->plugin_acts & (action))

/** registers a plugin with the client */
in3_ret_t in3_plugin_register(
    const char*            name,      /**< the name of the plugin (optional), which is ignored if LOGGIN is not defined */
    in3_t*                 c,         /**< the client */
    in3_plugin_supp_acts_t acts,      /**< the actions to register for combined with OR */
    in3_plugin_act_fn      action_fn, /**< the plugin action function */
    void*                  data,      /**< an optional data or config struct which will be passed to the action function when executed */
    bool                   replace_ex /**< if this is true and an plugin with the same action is already registered, it will replace it */
);

/** registers a plugin and uses the function name as plugin name */
#define plugin_register(c, acts, action_fn, data, replace_ex) in3_plugin_register(#action_fn, c, acts, action_fn, data, replace_ex)
/**
 * adds a plugin rregister function to the default. All defaults functions will automaticly called and registered for every new in3_t instance.
 */
void in3_register_default(plgn_register reg_fn);

/** executes all plugins irrespective of their return values, returns first error (if any) */
in3_ret_t in3_plugin_execute_all(in3_t* c, in3_plugin_act_t action, void* plugin_ctx);

/**
 * executes all plugin actions one-by-one, stops when a plugin returns anything other than IN3_EIGNORE.
 * returns IN3_EPLGN_NONE if no plugin was able to handle specified action, otherwise returns IN3_OK
 * plugin errors are reported via the in3_ctx_t
 */
in3_ret_t in3_plugin_execute_first(in3_ctx_t* ctx, in3_plugin_act_t action, void* plugin_ctx);

/**
 * same as in3_plugin_execute_first(), but returns IN3_OK even if no plugin could handle specified action
 */
in3_ret_t in3_plugin_execute_first_or_none(in3_ctx_t* ctx, in3_plugin_act_t action, void* plugin_ctx);

/**
 * same as in3_plugin_execute_first() using the client instead of context, but returns IN3_OK even if no plugin could handle specified action
 */
in3_ret_t in3_plugin_execute(in3_t* client, in3_plugin_act_t action, void* plugin_ctx);

// ----------- RPC HANDLE -----------

/**
 * verification context holding the pointers to all relevant toknes.
 */
typedef struct {
  in3_ctx_t*       ctx;      /**< Request context. */
  d_token_t*       request;  /**< request */
  in3_response_t** response; /**< the responses which a prehandle-method should set*/
} in3_rpc_handle_ctx_t;

/**
* creates a response and returns a stringbuilder to add the result-data.
*/
NONULL sb_t* in3_rpc_handle_start(in3_rpc_handle_ctx_t* hctx);

/**
 * finish the response.
 */
NONULL in3_ret_t in3_rpc_handle_finish(in3_rpc_handle_ctx_t* hctx);

/**
 * creates a response with bytes.
 */
NONULL in3_ret_t in3_rpc_handle_with_bytes(in3_rpc_handle_ctx_t* hctx, bytes_t data);

/**
 * creates a response with string.
 */
NONULL in3_ret_t in3_rpc_handle_with_string(in3_rpc_handle_ctx_t* hctx, char* data);

/**
 * creates a response with a value which is added as hex-string.
 */
NONULL in3_ret_t in3_rpc_handle_with_int(in3_rpc_handle_ctx_t* hctx, uint64_t value);

// -------------- TRANSPORT -------------

/** request-object. 
 * 
 * represents a RPC-request
 */
typedef struct in3_request {
  char*           payload;  /**< the payload to send */
  char**          urls;     /**< array of urls */
  uint_fast16_t   urls_len; /**< number of urls */
  struct in3_ctx* ctx;      /**< the current context */
  void*           cptr;     /**< a custom ptr to hold information during */
  uint32_t        wait;     /**< time in ms to wait before sending out the request */
} in3_request_t;

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
 * adds a response for a request-object.
 * This function should be used in the transport-function to set the response.
 */
NONULL void in3_req_add_response(
    in3_request_t* req,      /**< [in]the the request */
    int            index,    /**< [in] the index of the url, since this request could go out to many urls */
    bool           is_error, /**< [in] if true this will be reported as error. the message should then be the error-message */
    const char*    data,     /**<  the data or the the string*/
    int            data_len, /**<  the length of the data or the the string (use -1 if data is a null terminated string)*/
    uint32_t       time      /**<  the time this request took in ms or 0 if not possible (it will be used to calculate the weights)*/
);

/**
 * adds a response to a context.
 * This function should be used in the transport-function to set the response.
 */
NONULL void in3_ctx_add_response(
    in3_ctx_t*  ctx,      /**< [in]the current context */
    int         index,    /**< [in] the index of the url, since this request could go out to many urls */
    bool        is_error, /**< [in] if true this will be reported as error. the message should then be the error-message */
    const char* data,     /**<  the data or the the string*/
    int         data_len, /**<  the length of the data or the the string (use -1 if data is a null terminated string)*/
    uint32_t    time      /**<  the time this request took in ms or 0 if not possible (it will be used to calculate the weights)*/
);

typedef in3_ret_t (*in3_transport_legacy)(in3_request_t* request);
/**
 * defines a default transport which is used when creating a new client.
 */
void in3_set_default_legacy_transport(
    in3_transport_legacy transport /**< the default transport-function. */
);

// --------- SIGN_ACCOUNT --------------------

/**
 * action context when retrieving the account of a signer.
 */
typedef struct sign_account_ctx {
  struct in3_ctx* ctx;     /**< the context of the request in order report errors */
  address_t       account; /**< the account to use for the signature */
} in3_sign_account_ctx_t;

// ----------- SIGN_PREPARE ---------------

/**
 * action context when retrieving the account of a signer.
 */
typedef struct sign_prepare_ctx {
  struct in3_ctx* ctx;     /**< the context of the request in order report errors */
  address_t       account; /**< the account to use for the signature */
  bytes_t         old_tx;
  bytes_t         new_tx;

} in3_sign_prepare_ctx_t;

// -------------- SIGN -----------------------

/** type of the requested signature */
typedef enum {
  SIGN_EC_RAW  = 0, /**< sign the data directly */
  SIGN_EC_HASH = 1, /**< hash and sign the data */
} d_signature_type_t;

/**
 * signing context. This Context is passed to the signer-function. 
 */
typedef struct sign_ctx {
  uint8_t            signature[65]; /**< the resulting signature needs to be writte into these bytes */
  d_signature_type_t type;          /**< the type of signature*/
  struct in3_ctx*    ctx;           /**< the context of the request in order report errors */
  bytes_t            message;       /**< the message to sign*/
  bytes_t            account;       /**< the account to use for the signature */
} in3_sign_ctx_t;

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
 * creates a signer ctx to be used for async signing.
 */
NONULL in3_sign_ctx_t* create_sign_ctx(
    in3_ctx_t* ctx /**< [in] the rpc context */
);

// -------- SET_CONFIG ---------

/**
 * context used during configure
 */
typedef struct in3_configure_ctx {
  in3_t*     client;    /**< the client to configure */
  d_token_t* token;     /**< the token not handled yet*/
  char*      error_msg; /**< message in case of an incorrect config */
} in3_configure_ctx_t;

// -------- SET_CONFIG ---------

/**
 * context used during get config
 */
typedef struct in3_get_config_ctx {
  in3_t* client; /**< the client to configure */
  sb_t*  sb;     /**< stringbuilder to add json-config*/
} in3_get_config_ctx_t;

// -------- CACHE ---------
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
 * context used during get config
 */
typedef struct in3_cache_ctx {
  in3_ctx_t* ctx;     /**< the request context  */
  char*      key;     /**< the key to fetch */
  bytes_t*   content; /**< the content to set */
} in3_cache_ctx_t;

/**
 * create a new storage handler-object to be set on the client.
 * the caller will need to free this pointer after usage.
 */
NONULL_FOR((1, 2, 3, 4))
void in3_set_storage_handler(
    in3_t*               c,        /**< the incubed client */
    in3_storage_get_item get_item, /**< function pointer returning a stored value for the given key.*/
    in3_storage_set_item set_item, /**< function pointer setting a stored value for the given key.*/
    in3_storage_clear    clear,    /**< function pointer clearing all contents of cache.*/
    void*                cptr      /**< custom pointer which will will be passed to functions */
);

// ----------- VERIFY --------------

#ifdef LOGGING
#define vc_err(vc, msg) vc_set_error(vc, msg)
#else
#define vc_err(vc, msg) vc_set_error(vc, NULL)
#endif

/**
 * verification context holding the pointers to all relevant toknes.
 */
typedef struct {
  in3_ctx_t*   ctx;                   /**< Request context. */
  in3_chain_t* chain;                 /**< the chain definition. */
  d_token_t*   result;                /**< the result to verify */
  d_token_t*   request;               /**< the request sent. */
  d_token_t*   proof;                 /**< the delivered proof. */
  in3_t*       client;                /**< the client. */
  uint64_t     last_validator_change; /**< Block number of last change of the validator list */
  uint64_t     currentBlock;          /**< Block number of latest block */
  int          index;                 /**< the index of the request within the bulk */
} in3_vctx_t;

#ifdef LOGGING
NONULL
#endif

/*
 * creates an error attaching it to the context and returns -1. 
 */
in3_ret_t vc_set_error(
    in3_vctx_t* vc, /**< the verification context. */
    char*       msg /**< the error message. */
);

// ---- PAY_SIGN_REQ -----------

typedef struct {
  in3_ctx_t* ctx;           /**< Request context. */
  d_token_t* request;       /**< the request sent. */
  bytes32_t  request_hash;  /**< the hash to sign */
  uint8_t    signature[65]; /**< the signature */
} in3_pay_sign_req_ctx_t;

#ifdef SENTRY
typedef struct {
  char* msg; /**< the error message. */
} sentry_ctx_t;
#endif

#endif
