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
in3_ret_t in3_plugin_register(in3_t* c, in3_plugin_supp_acts_t acts, in3_plugin_act_fn action_fn, void* data, bool replace_ex);

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
    int            data_len  /**<  the length of the data or the the string (use -1 if data is a null terminated string)*/
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
  in3_t*     client; /**< the client to configure */
  d_token_t* token;  /**< the token not handled yet*/
  char*      err;    /**< the error string to return to user */
} in3_configure_ctx_t;

// -------- GET_CONFIG ---------

/**
 * context used during get config
 */
typedef struct in3_get_config_ctx {
  in3_t* client; /**< the client to configure */
  sb_t*  sb;     /**< stringbuilder to add json-config*/
} in3_get_config_ctx_t;

// -------- CACHE_SET ---------

typedef enum {
  CACHE_NODELIST,
  CACHE_WHITELIST,
  CACHE_VER_HASHES,
} in3_cache_set_type_t;
/**
 * context used during cache actions
 */
typedef struct in3_cache_ctx {
  in3_cache_set_type_t type;
  in3_t*               client; /**< client instance whose cache is to be used */
  void*                data;   /**< stringbuilder to add json-config */
} in3_cache_ctx_t;

#endif
