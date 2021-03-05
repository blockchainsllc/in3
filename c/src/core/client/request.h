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
 * Request Context.
 * This is used for each request holding request and response-pointers but also controls the execution process.
 * */

#ifndef CONTEXT_H
#define CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../util/data.h"
#include "../util/scache.h"
#include "../util/stringbuilder.h"
#include "../util/utils.h"
#include "client.h"
#include <stdbool.h>
#include <stdint.h>
/**
 * type of the request context,
 */
typedef enum ctx_type {
  RT_RPC  = 0, /**< a json-rpc request, which needs to be send to a incubed node */
  RT_SIGN = 1  /**< a sign request */
} req_type_t;

/**
 * the weight of a certain node as linked list. 
 * 
 * This will be used when picking the nodes to send the request to. A linked list of these structs desribe the result.
 */
typedef struct weight {
  unsigned int   index;   /**< index of the node in the nodelist */
  uint32_t       s;       /**< The starting value */
  uint32_t       w;       /**< weight value */
  char*          url;     /**< the url of the node */
  address_t      address; /**< address of the server */
  struct weight* next;    /**< next in the linked-list or NULL if this is the last element*/
} node_match_t;

/** response-object. 
 * 
 * if the error has a length>0 the response will be rejected
 */
typedef struct in3_response {
  uint32_t  time;  /**< measured time (in ms) which will be used for ajusting the weights */
  in3_ret_t state; /**< the state of the response */
  sb_t      data;  /**< a stringbuilder to add the result */
} in3_response_t;

/**
 * The Request config.
 * 
 * This is generated for each request and represents the current state. it holds the state until the request is finished and must be freed afterwards.
 * */
typedef struct in3_req {
  uint_fast8_t    signers_length;     /**< number or addresses */
  uint_fast16_t   len;                /**< the number of requests */
  uint_fast16_t   attempt;            /**< the number of attempts */
  uint32_t        id;                 /**< JSON RPC id of request at index 0 */
  req_type_t      type;               /**< the type of the request */
  in3_ret_t       verification_state; /**< state of the verification */
  char*           error;              /**< in case of an error this will hold the message, if not it points to `NULL` */
  json_ctx_t*     request_context;    /**< the result of the json-parser for the request.*/
  json_ctx_t*     response_context;   /**< the result of the json-parser for the response.*/
  d_token_t**     requests;           /**< references to the tokens representring the requests*/
  d_token_t**     responses;          /**< references to the tokens representring the parsed responses*/
  in3_response_t* raw_response;       /**< the raw response-data, which should be verified. */
  uint8_t*        signers;            /**< the addresses of servers requested to sign the blockhash */
  node_match_t*   nodes;              /**< selected nodes to process the request, which are stored as linked list.*/
  cache_entry_t*  cache;              /**<optional cache-entries.  These entries will be freed when cleaning up the context.*/
  struct in3_req* required;           /**< pointer to the next required context. if not NULL the data from this context need get finished first, before being able to resume this context. */
  in3_t*          client;             /**< reference to the client*/
} in3_req_t;

/**
 * The current state of the context.
 * 
 * you can check this state after each execute-call.
 */
typedef enum state {
  REQ_SUCCESS              = 0,  /**< The ctx has a verified result. */
  REQ_WAITING_TO_SEND      = 1,  /**< the request has not been sent yet */
  REQ_WAITING_FOR_RESPONSE = 2,  /**< the request is sent but not all of the response are set () */
  REQ_ERROR                = -1, /**< the request has a error */
} in3_req_state_t;

/** 
 * creates a new request.
 * 
 * the request data will be parsed and represented in the context.
 * calling this function will only parse the request data, but not send anything yet.
 * 
 *  *Important*: the req_data will not be cloned but used during the execution. The caller of the this function is also responsible for freeing this string afterwards.
 */
NONULL in3_req_t* req_new(
    in3_t*      client,  /**< [in] the client-config. */
    const char* req_data /**< [in] the rpc-request as json string. */
);
/**
 * sends a previously created request to nodes and verifies it.
 * 
 * The execution happens within the same thread, thich mean it will be blocked until the response ha beedn received and verified.
 * In order to handle calls asynchronously, you need to call the `in3_req_execute` function and provide the data as needed.
 */
NONULL in3_ret_t in3_send_req(
    in3_req_t* req /**< [in] the request context. */
);

/**
 * finds the last waiting request-context.
 */
NONULL in3_req_t* in3_req_last_waiting(
    in3_req_t* req /**< [in] the request context. */
);

/**
 * executes the request and returns its state.
 */
NONULL in3_req_state_t in3_req_exec_state(
    in3_req_t* req /**< [in] the request context. */
);
/**
 * execute the context, but stops whenever data are required.
 * 
 * This function should be used in order to call data in a asyncronous way, 
 * since this function will not use the transport-function to actually send it. 
 * 
 * The caller is responsible for delivering the required responses.
 * After calling you need to check the return-value:
 * - IN3_WAITING : provide the required data and then call in3_req_execute again.
 * - IN3_OK : success, we have a result.
 * - any other status = error
 * 
 * ```rust,ignore
 * digraph G {
 node[fontname="Helvetica",   shape=Box, color=lightblue, style=filled ]
  edge[fontname="Helvetica",   style=solid,  fontsize=8 , color=grey]
  rankdir = LR;
  
  RPC[label="RPC-Request"]
  CTX[label="in3_req_t"]
  
  sign[label="sign data",color=lightgrey, style=""]
  request[label="fetch data",color=lightgrey, style=""]
  
  exec[ label="in3_req_execute()",color=lightgrey, style="", shape=circle ]
  free[label="req_free()",color=lightgrey, style=""]


  RPC -> CTX [label="req_new()"]
  CTX -> exec
  
  
  exec -> error [label="IN3_..."]
  exec -> response[label="IN3_OK"]
  exec -> waiting[label="IN3_WAITING"]
  
  waiting -> sign[label=RT_SIGN]
  waiting -> request[label=RT_RPC] 
  
  sign -> exec [label="in3_ctx_add_response()"]
  request -> exec[label="in3_ctx_add_response()"]
  
  response -> free
  error->free
  

 { rank = same; exec, sign, request }



}
 * ```
 * 
 * Here is a example how to use this function:
 * 
 * ```c
 * 
 in3_ret_t in3_send_req(in3_req_t* req) {
  in3_ret_t ret;
  // execute the context and store the return value.
  // if the return value is 0 == IN3_OK, it was successful and we return,
  // if not, we keep on executing
  while ((ret = in3_req_execute(ctx))) {
    // error we stop here, because this means we got an error
    if (ret != IN3_WAITING) return ret;

    // handle subcontexts first, if they have not been finished
    while (ctx->required && in3_req_state(ctx->required) != REQ_SUCCESS) {
      // exxecute them, and return the status if still waiting or error
      if ((ret = in3_send_req(ctx->required))) return ret;

      // recheck in order to prepare the request.
      // if it is not waiting, then it we cannot do much, becaus it will an error or successfull.
      if ((ret = in3_req_execute(ctx)) != IN3_WAITING) return ret;
    }

    // only if there is no response yet...
    if (!ctx->raw_response) {

      // what kind of request do we need to provide?
      switch (ctx->type) {

        // RPC-request to send to the nodes
        case RT_RPC: {

            // build the request
            in3_http_request_t* request = in3_create_request(ctx);

            // here we use the transport, but you can also try to fetch the data in any other way.
            ctx->client->transport(request);

            // clean up
            request_free(request);
            break;
        }

        // this is a request to sign a transaction
        case RT_SIGN: {
            // read the data to sign from the request
            d_token_t* params = d_get(ctx->requests[0], K_PARAMS);
            // the data to sign
            bytes_t    data   = d_to_bytes(d_get_at(params, 0));
            // the account to sign with
            bytes_t    from   = d_to_bytes(d_get_at(params, 1));

            // prepare the response
            ctx->raw_response = _malloc(sizeof(in3_response_t));
            sb_init(&ctx->raw_response[0].error);
            sb_init(&ctx->raw_response[0].result);

            // data for the signature 
            uint8_t sig[65];
            // use the signer to create the signature
            ret = ctx->client->signer->sign(ctx, SIGN_EC_HASH, data, from, sig);
            // if it fails we report this as error
            if (ret < 0) return req_set_error(ctx, ctx->raw_response->error.data, ret);
            // otherwise we simply add the raw 65 bytes to the response.
            sb_add_range(&ctx->raw_response->result, (char*) sig, 0, 65);
        }
      }
    }
  }
  // done...
  return ret;
}
 * ```
 * 
 * 
 * 
 * 
 */
NONULL in3_ret_t in3_req_execute(
    in3_req_t* req /**< [in] the request context. */
);

/**
 * returns the current state of the context.
 */
NONULL in3_req_state_t in3_req_state(
    in3_req_t* req /**< [in] the request context. */
);

/**
 * returns the error of the context.
 */
char* req_get_error_data(
    in3_req_t* req /**< [in] the request context. */
);

/**
 * returns json response for that context
 */
char* req_get_response_data(
    in3_req_t* req /**< [in] the request context. */
);

/**
 * returns the type of the request
 */
NONULL req_type_t req_get_type(
    in3_req_t* req /**< [in] the request context. */
);

/**
 * frees all resources allocated during the request.
 * 
 * But this will not free the request string passed when creating the context!
 */
NONULL void req_free(
    in3_req_t* req /**< [in] the request context. */
);
/**
 * adds a new context as a requirment.
 * 
 * Whenever a verifier needs more data and wants to send a request, we should create the request and add it as dependency and stop.
 * 
 * If the function is called again, we need to search and see if the required status is now useable.
 * 
 * Here is an example of how to use it:
 * 
 * ```c
in3_ret_t get_from_nodes(in3_req_t* parent, char* method, char* params, bytes_t* dst) {
  // check if the method is already existing
  in3_req_t* req = req_find_required(parent, method);
  if (ctx) {
    // found one - so we check if it is useable.
    switch (in3_req_state(ctx)) {
      // in case of an error, we report it back to the parent context
      case REQ_ERROR:
        return req_set_error(parent, ctx->error, IN3_EUNKNOWN);
      // if we are still waiting, we stop here and report it.
      case CTX_WAITING_FOR_REQUIRED_CTX:
      case REQ_WAITING_FOR_RESPONSE:
        return IN3_WAITING;

      // if it is useable, we can now handle the result.
      case REQ_SUCCESS: {
        d_token_t* r = d_get(ctx->responses[0], K_RESULT);
        if (r) {
          // we have a result, so write it back to the dst
          *dst = d_to_bytes(r);
          return IN3_OK;
        } else
          // or check the error and report it
          return req_check_response_error(parent, 0);
      }
    }
  }

  // no required context found yet, so we create one:

  // since this is a subrequest it will be freed when the parent is freed.
  // allocate memory for the request-string
  char* req = _malloc(strlen(method) + strlen(params) + 200);
  // create it
  sprintf(req, "{\"method\":\"%s\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":%s}", method, params);
  // and add the request context to the parent.
  return req_add_required(parent, req_new(parent->client, req));
}
 * ```
 */
NONULL in3_ret_t req_add_required(
    in3_req_t* parent, /**< [in] the current request context. */
    in3_req_t* req     /**< [in] the new request context to add. */
);
/**
 * searches within the required request contextes for one with the given method.
 * 
 * This method is used internaly to find a previously added context.
 */
NONULL in3_req_t* req_find_required(
    const in3_req_t* parent, /**< [in] the current request context. */
    const char*      method  /**< [in] the method of the rpc-request. */
);
/**
 * removes a required context after usage.
 * removing will also call free_ctx to free resources.
 */
NONULL in3_ret_t req_remove_required(
    in3_req_t* parent, /**< [in] the current request context. */
    in3_req_t* req,    /**< [in] the request context to remove. */
    bool       rec     /**< [in] if true all sub contexts will aösp be removed*/
);
/**
 * check if the response contains a error-property and reports this as error in the context.
 */
NONULL in3_ret_t req_check_response_error(
    in3_req_t* c, /**< [in] the current request context. */
    int        i  /**< [in] the index of the request to check (if this is a batch-request, otherwise 0). */
);

/**
 * determins the errorcode for the given request.
 */
NONULL in3_ret_t req_get_error(
    in3_req_t* req, /**< [in] the current request context. */
    int        id   /**< [in] the index of the request to check (if this is a batch-request, otherwise 0). */
);

/** 
 * sends a request and returns a context used to access the result or errors. 
 * 
 * This context *MUST* be freed with req_free(ctx) after usage to release the resources.
*/
NONULL in3_req_t* in3_client_rpc_ctx_raw(
    in3_t*      c,      /**< [in] the client config. */
    const char* request /**< [in] rpc request. */
);

/**
 * sends a request and returns a context used to access the result or errors.
 *
 * This context *MUST* be freed with req_free(ctx) after usage to release the resources.
*/
NONULL in3_req_t* in3_client_rpc_ctx(
    in3_t*      c,      /**< [in] the clientt config. */
    const char* method, /**< [in] rpc method. */
    const char* params  /**< [in] params as string. */
);

/**
 * determines the proof as set in the request.
 */
NONULL in3_proof_t in3_req_get_proof(
    in3_req_t* req, /**< [in] the current request. */
    int        i    /**< [in] the index within the request. */
);

#ifdef __cplusplus
}
#endif
#endif
