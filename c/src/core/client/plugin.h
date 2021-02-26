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
 * this file defines the plugin-contexts
 * 
 * 
 * */

#ifndef PLUGIN_H
#define PLUGIN_H
#ifdef __cplusplus
extern "C" {
#endif
#include "client.h"
#include "request.h"

// ---------- plugin management -----------------

/** checks if a plugin for specified action is registered with the client */
#define in3_plugin_is_registered(client, action) (((client)->plugin_acts & (action)) == (action))

/** registers a plugin with the client */
in3_ret_t in3_plugin_register(
    in3_t*                 c,         /**< the client */
    in3_plugin_supp_acts_t acts,      /**< the actions to register for combined with OR */
    in3_plugin_act_fn      action_fn, /**< the plugin action function */
    void*                  data,      /**< an optional data or config struct which will be passed to the action function when executed */
    bool                   replace_ex /**< if this is true and an plugin with the same action is already registered, it will replace it */
);

/**
 * adds a plugin rregister function to the default. All defaults functions will automaticly called and registered for every new in3_t instance.
 */
void in3_register_default(plgn_register reg_fn);

/** executes all plugins irrespective of their return values, returns first error (if any) */
in3_ret_t in3_plugin_execute_all(in3_t* c, in3_plugin_act_t action, void* plugin_ctx);

/**
 * executes all plugin actions one-by-one, stops when a plugin returns anything other than IN3_EIGNORE.
 * returns IN3_EPLGN_NONE if no plugin was able to handle specified action, otherwise returns IN3_OK
 * plugin errors are reported via the in3_req_t
 */
in3_ret_t in3_plugin_execute_first(in3_req_t* req, in3_plugin_act_t action, void* plugin_ctx);

/**
 * same as in3_plugin_execute_first(), but returns IN3_OK even if no plugin could handle specified action
 */
in3_ret_t in3_plugin_execute_first_or_none(in3_req_t* req, in3_plugin_act_t action, void* plugin_ctx);

/**
 * get direct access to plugin data (if registered) based on action function
 */
static inline void* in3_plugin_get_data(in3_t* c, in3_plugin_act_fn fn) {
  for (in3_plugin_t* p = c->plugins; p; p = p->next) {
    if (p->action_fn == fn) return p->data;
  }
  return NULL;
}

// ----------- RPC HANDLE -----------

/**
 * verification context holding the pointers to all relevant toknes.
 */
typedef struct {
  in3_req_t*       req;      /**< Request context. */
  d_token_t*       request;  /**< request */
  in3_response_t** response; /**< the responses which a prehandle-method should set*/
  char*            method;   /**< the method of the request */
  d_token_t*       params;   /**< the params */
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

/**
 * optional request headers
 */
typedef struct in3_req_header {
  char*                  value; /**< the value */
  struct in3_req_header* next;  /**< pointer to next header */
} in3_req_header_t;

/** request-object. 
 * 
 * represents a RPC-request
 */
typedef struct in3_http_request {
  char*             method;      /**< the http-method to be used */
  char*             payload;     /**< the payload to send */
  char**            urls;        /**< array of urls */
  uint_fast16_t     urls_len;    /**< number of urls */
  uint32_t          payload_len; /**< length of the payload in bytes. */
  struct in3_req*   req;         /**< the current context */
  void*             cptr;        /**< a custom ptr to hold information during */
  uint32_t          wait;        /**< time in ms to wait before sending out the request */
  in3_req_header_t* headers;     /**< optional additional headers to be send with the request */
} in3_http_request_t;

/**
 * getter to retrieve the payload from a in3_http_request_t struct
 */
char* in3_get_request_payload(
    in3_http_request_t* request /**< request struct */
);
/**
 * getter to retrieve the length of the payload from a in3_http_request_t struct
 */
uint32_t in3_get_request_payload_len(
    in3_http_request_t* request /**< request struct */
);

/**
 * getter to retrieve the urls list length from a in3_http_request_t struct
 */
int in3_get_request_headers_len(
    in3_http_request_t* request /**< request struct */
);
/**
 * getter to retrieve the urls list length from a in3_http_request_t struct
 */
char* in3_get_request_headers_at(
    in3_http_request_t* request, /**< request struct */
    int                 index    /**< the inde xof the header */
);

/**
 * getter to retrieve the http-method from a in3_http_request_t struct
 */
char* in3_get_request_method(
    in3_http_request_t* request /**< request struct */
);

/**
 * getter to retrieve the urls list from a in3_http_request_t struct
 */
char** in3_get_request_urls(
    in3_http_request_t* request /**< request struct */
);

/**
 * getter to retrieve the urls list length from a in3_http_request_t struct
 */
int in3_get_request_urls_len(
    in3_http_request_t* request /**< request struct */
);

/**
 * getter to retrieve the urls list length from a in3_http_request_t struct
 */
uint32_t in3_get_request_timeout(
    in3_http_request_t* request /**< request struct */
);

/**
 * adds a response for a request-object.
 * This function should be used in the transport-function to set the response.
 */
NONULL void in3_req_add_response(
    in3_http_request_t* req,      /**< [in]the the request */
    int                 index,    /**< [in] the index of the url, since this request could go out to many urls */
    int                 error,    /**< [in] if <0 this will be reported as error. the message should then be the error-message */
    const char*         data,     /**<  the data or the the string*/
    int                 data_len, /**<  the length of the data or the the string (use -1 if data is a null terminated string)*/
    uint32_t            time      /**<  the time this request took in ms or 0 if not possible (it will be used to calculate the weights)*/
);

/**
 * adds a response to a context.
 * This function should be used in the transport-function to set the response.
 */
NONULL void in3_ctx_add_response(
    in3_req_t*  req,      /**< [in]the current context */
    int         index,    /**< [in] the index of the url, since this request could go out to many urls */
    int         error,    /**< [in] if <0 this will be reported as error. the message should then be the error-message */
    const char* data,     /**<  the data or the the string*/
    int         data_len, /**<  the length of the data or the the string (use -1 if data is a null terminated string)*/
    uint32_t    time      /**<  the time this request took in ms or 0 if not possible (it will be used to calculate the weights)*/
);

typedef in3_ret_t (*in3_transport_legacy)(in3_http_request_t* request);
/**
 * defines a default transport which is used when creating a new client.
 */
void in3_set_default_legacy_transport(
    in3_transport_legacy transport /**< the default transport-function. */
);

// --------- SIGN_ACCOUNT --------------------
/**
 * defines the type of signer used
 */
typedef enum {
  SIGNER_ECDSA   = 1,
  SIGNER_EIP1271 = 2
} in3_signer_type_t;
/**
 * action context when retrieving the account of a signer.
 */
typedef struct sign_account_ctx {
  struct in3_req*   req;          /**< the context of the request in order report errors */
  uint8_t*          accounts;     /**< the account to use for the signature */
  int               accounts_len; /**< number of accounts */
  in3_signer_type_t signer_type;  /**< the type of the signer used for this account.*/
} in3_sign_account_ctx_t;

// ----------- SIGN_PREPARE ---------------

/**
 * action context when retrieving the account of a signer.
 */
typedef struct sign_prepare_ctx {
  struct in3_req* req;     /**< the context of the request in order report errors */
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
  bytes_t            signature; /**< the resulting signature  */
  d_signature_type_t type;      /**< the type of signature*/
  struct in3_req*    req;       /**< the context of the request in order report errors */
  bytes_t            message;   /**< the message to sign*/
  bytes_t            account;   /**< the account to use for the signature */
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
void in3_sign_ctx_set_signature_hex(
    in3_sign_ctx_t* ctx,      /**< the signer context */
    const char*     signature /**< the signature in hex */
);

/**
 * creates a signer ctx to be used for async signing.
 */
NONULL in3_sign_ctx_t* create_sign_ctx(
    in3_req_t* req /**< [in] the rpc context */
);

// -------- SET_CONFIG ---------

/**
 * context used during configure
 */
typedef struct in3_configure_ctx {
  in3_t*      client;    /**< the client to configure */
  json_ctx_t* json;      /**< the json ctx corresponding to below token */
  d_token_t*  token;     /**< the token not handled yet*/
  char*       error_msg; /**< message in case of an incorrect config */
} in3_configure_ctx_t;

// -------- GET_CONFIG ---------

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
  in3_req_t* req;     /**< the request context  */
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
  in3_req_t*    req;                   /**< Request context. */
  in3_chain_t*  chain;                 /**< the chain definition. */
  d_token_t*    result;                /**< the result to verify */
  d_token_t*    request;               /**< the request sent. */
  d_token_t*    proof;                 /**< the delivered proof. */
  in3_t*        client;                /**< the client. */
  uint64_t      last_validator_change; /**< Block number of last change of the validator list */
  uint64_t      currentBlock;          /**< Block number of latest block */
  int           index;                 /**< the index of the request within the bulk */
  node_match_t* node;                  /**< the node who delivered this response */
  bool          dont_blacklist;        /**< indicates whether the plugin would like the node to be blacklisted */
  char*         method;                /**< the rpc-method to verify agains */
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

// ---- PLGN_ACT_PAY_FOLLOWUP -----------

typedef struct {
  in3_req_t*    req;        /**< Request context. */
  node_match_t* node;       /**< the responding node. */
  d_token_t*    resp_in3;   /**< the response's in3 section */
  d_token_t*    resp_error; /**< the response's error section */
} in3_pay_followup_ctx_t;

// ---- PLGN_ACT_PAY_HANDLE -----------

typedef struct {
  in3_req_t* req;     /**< Request context. */
  sb_t*      payload; /**< the request payload */
  bytes32_t  pk;      /**< the private-key to sign with */
} in3_pay_handle_ctx_t;

// ---- PAY_SIGN_REQ -----------

typedef struct {
  in3_req_t* req;           /**< Request context. */
  d_token_t* request;       /**< the request sent. */
  bytes32_t  request_hash;  /**< the hash to sign */
  uint8_t    signature[65]; /**< the signature */
} in3_pay_sign_req_ctx_t;

// ---- PLGN_ACT_ADD_PAYLOAD -----------

typedef struct {
  in3_req_t* req;     /**< Request context. */
  d_token_t* request; /**< the request sent. */
  sb_t*      sb;      /**< the string builder in the in3-section */
} in3_pay_payload_ctx_t;

// ---- LOG_ERROR -----------

typedef struct {
  char*      msg;   /**< the error message. */
  uint16_t   error; /**< error code. */
  in3_req_t* req;   /**< ctx . */
} error_log_ctx_t;

// -------- NL_PICK ---------
typedef enum {
  NL_DATA,  /**< data provider node. */
  NL_SIGNER /**< signer node. */
} in3_nl_pick_type_t;

typedef struct {
  in3_nl_pick_type_t type; /**< type of node to pick. */
  in3_req_t*         req;  /**< Request context. */
} in3_nl_pick_ctx_t;

// -------- NL_FOLLOWUP ---------
typedef struct {
  in3_req_t*    req;  /**< Request context. */
  node_match_t* node; /**< Node that gave us a valid response */
} in3_nl_followup_ctx_t;

// -------- NL_BLACKLIST ---------
typedef struct {
  union {
    uint8_t*    address; /**< address of node that is to be blacklisted */
    const char* url;     /**< URL of node that is to be blacklisted */
  };
  bool is_addr; /**< Specifies whether the identifier is an address or a url */
} in3_nl_blacklist_ctx_t;

// -------- NL_OFFLINE ---------
typedef struct {
  in3_vctx_t*  vctx;    /**< Request context. */
  unsigned int missing; /**< bitmask representing nodes - a reset bit indicates missing signatures */
} in3_nl_offline_ctx_t;

// -------- GET_DATA ---------
typedef enum {
  GET_DATA_REGISTRY_ID,         /* returns a pointer to an internal bytes32_t representation; NO cleanup required */
  GET_DATA_NODE_MIN_BLK_HEIGHT, /* returns a pointer to an internal bitmask; NO cleanup required */
  GET_DATA_CLIENT_DATA,         /* returns an opaque pointer that was previously set by caller */
} in3_get_data_type_t;

/**
 * context used during get data
 * sample usage -
 *     in3_get_data_ctx_t dctx = {.type = GET_DATA_REGISTRY_ID};
 *     in3_plugin_execute_first(ctx, PLGN_ACT_GET_DATA, &dctx);
 *     // use dctx->data as required
 *     if (dctx.cleanup) dctx.cleanup(dctx.data);
 */
typedef struct {
  in3_get_data_type_t type; /**< type of data that the caller wants. */
  void*               data; /**< output param set by plugin code - pointer to data requested. */
  void (*cleanup)(void*);   /**< output param set by plugin code - if not NULL use it to cleanup the data. */
} in3_get_data_ctx_t;
#ifdef __cplusplus
}
#endif
#endif //PLUGIN_H
