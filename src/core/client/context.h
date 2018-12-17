/** @file 
 * Request Context.
 * This is used for each request holding request and response-pointers.
 * */ 


#include <stdint.h>  
#include <stdbool.h>
#include "../util/utils.h"
#include "../util/stringbuilder.h"
#include "client.h"

#ifndef CONTEXT_H
#define CONTEXT_H

/**
 * the weight of a ceertain node as linked list
 */
typedef struct weight {
  in3_node_t* node;           /**< the node definition including the url */
  in3_node_weight_t* weight;  /**< the current weight and blacklisting-stats */ 
  float s;                    /**< The starting value */
  float w;                    /**< weight value */
  struct weight * next;       /**< next in the linkedlistt or NULL if this is the last element*/
} node_weight_t;


/**
 * The Request config.
 * This is generated for each request and represents the current state.
 * */
typedef struct {
   /*! reference to the client*/
   in3* client;

    /* the incoming request (this will not be freed when cleaning up!)*/
   char* request_data;

   /* the response-string */
   char* response_data;

   /* in case of an error this will hold the message */
   char* error;

   /* the number of requests */
   int len;

   /* the number of attempts */
   int attempt;

   /* full list of tokens, which will be cleaned up*/
   jsmntok_t* tok_req;
   /* full list of tokens, which will be cleaned up*/
   jsmntok_t* tok_res;

    /* references to the tokens representring the requests*/
   jsmntok_t** responses;
   
    /* references to the tokens representring the responses*/
   jsmntok_t** requests;

   /* configs adjusted for each request */
   in3_request_config_t* requests_configs;

   /* selected nodes to process the request*/
   node_weight_t* nodes;

} in3_ctx_t;

/** 
 * creates a new context.
 * 
 * the request data will be parsed and represented in the context.
 */
in3_ctx_t* new_ctx(in3* client, char* req_data);
int ctx_parse_response(in3_ctx_t* ctx, char* response_data);
void free_ctx(in3_ctx_t* ctx);

jsmntok_t* ctx_get_token(char* str, jsmntok_t* root, char* key);
jsmntok_t* ctx_get_array_token(jsmntok_t* root, int index);

bool ctx_equals(char* str, jsmntok_t* root, char* val);
int ctx_cpy_string(char* str, jsmntok_t* root, char* dst);
uint64_t ctx_to_long(char* str, jsmntok_t* root, uint64_t defVal);
uint32_t ctx_to_int(char* str, jsmntok_t* root, uint32_t defVal);
/* creates a new bytes-array which must be cleaned up*/
bytes_t* ctx_to_bytes(char* str, jsmntok_t* root, int min_len);
bytes_t** ctx_to_byte_a(char* str, jsmntok_t* root);

int ctx_create_payload(in3_ctx_t* c, sb_t* sb);

int ctx_set_error(in3_ctx_t* c, char* msg, int errnumber);
int ctx_nodes_len (node_weight_t* root);
void free_ctx_nodes (node_weight_t* c) ;

#endif
