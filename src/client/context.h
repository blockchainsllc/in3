#include <stdint.h>  
#include <stdbool.h>
#include "../util/utils.h"

#ifndef CONTEXT_H
#define CONTEXT_H



typedef struct {
    /* the incoming request (this will not be freed when cleaning up!)*/
   char* request_data;

   /* the response-string */
   char* response_data;

   /* in case of an error this will hold the message */
   char* error;

   /* the number of requests */
   int len;

   /* full list of tokens, which will be cleaned up*/
   jsmntok_t* tok_req;
   /* full list of tokens, which will be cleaned up*/
   jsmntok_t* tok_res;

    /* references to the tokens representring the requests*/
   jsmntok_t** responses;
   
    /* references to the tokens representring the responses*/
   jsmntok_t** requests;

} in3_ctx_t;

in3_ctx_t* new_ctx(char* req_data);
int ctx_parse_response(in3_ctx_t* ctx, char* response_data);
void free_ctx(in3_ctx_t* ctx);

jsmntok_t* ctx_get_token(char* str, jsmntok_t* root, char* key);
bool ctx_equals(char* str, jsmntok_t* root, char* val);
int ctx_cpy_string(char* str, jsmntok_t* root, char* dst);
u_int64_t ctx_to_long(char* str, jsmntok_t* root, u_int64_t defVal);
/* creates a new bytes-array which must be cleaned up*/
bytes_t* ctx_to_bytes(char* str, jsmntok_t* root, int min_len);

#endif
