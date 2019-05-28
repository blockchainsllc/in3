/** @file 
 * Request Context.
 * This is used for each request holding request and response-pointers.
 * */

#include "../util/data.h"
#include "../util/scache.h"
#include "../util/stringbuilder.h"
#include "../util/utils.h"
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
in3_ctx_t*  new_ctx(in3_t* client, char* req_data);
in3_error_t ctx_parse_response(in3_ctx_t* ctx, char* response_data, int len);
void        free_ctx(in3_ctx_t* ctx);
in3_error_t ctx_create_payload(in3_ctx_t* c, sb_t* sb);
in3_error_t ctx_set_error(in3_ctx_t* c, char* msg, int errnumber);
in3_error_t ctx_get_error(in3_ctx_t* ctx, int id);

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
