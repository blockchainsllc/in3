/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2019 slock.it GmbH, Blockchains LLC
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
 * This is used for each request holding request and response-pointers.
 * */

#include "data.h"
#include "scache.h"
#include "stringbuilder.h"
#include "utils.h"
#include "client.h"
#include <stdbool.h>
#include <stdint.h>
#ifndef CONTEXT_H
#define CONTEXT_H

/**
 * the weight of a ceertain node as linked list
 */
typedef struct weight {
  in3_node_t*        node;   /**< the node definition including the url */
  in3_node_weight_t* weight; /**< the current weight and blacklisting-stats */
  float              s;      /**< The starting value */
  float              w;      /**< weight value */
  struct weight*     next;   /**< next in the linkedlistt or NULL if this is the last element*/
} node_weight_t;

/**
 * The Request config.
 * This is generated for each request and represents the current state.
 * */
typedef struct {
  /*! reference to the client*/
  in3_t* client;

  json_ctx_t* request_context;
  json_ctx_t* response_context;

  /* in case of an error this will hold the message */
  char* error;

  /* the number of requests */
  int len;

  /* the number of attempts */
  int attempt;

  /* references to the tokens representring the requests*/
  d_token_t** responses;

  /* references to the tokens representring the responses*/
  d_token_t** requests;

  /* configs adjusted for each request */
  in3_request_config_t* requests_configs;

  /* selected nodes to process the request*/
  node_weight_t* nodes;

  /** optional cache-entries */
  cache_entry_t* cache;

} in3_ctx_t;

/** 
 * creates a new context.
 * 
 * the request data will be parsed and represented in the context.
 */
in3_ctx_t* new_ctx(in3_t* client, char* req_data);
in3_ret_t  ctx_parse_response(in3_ctx_t* ctx, char* response_data, int len);
void       free_ctx(in3_ctx_t* ctx);
in3_ret_t  ctx_create_payload(in3_ctx_t* c, sb_t* sb);
in3_ret_t  ctx_set_error(in3_ctx_t* c, char* msg, in3_ret_t errnumber);
in3_ret_t  ctx_get_error(in3_ctx_t* ctx, int id);

/** 
 * sends a request and returns a context used to access the result or errors. 
 * 
 * This context *MUST* be freed with free_ctx(ctx) after usage to release the resources.
*/
in3_ctx_t* in3_client_rpc_ctx(in3_t* c, char* method, char* params);

// weights
void free_ctx_nodes(node_weight_t* c);
int  ctx_nodes_len(node_weight_t* root);

#endif
