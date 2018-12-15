#include <stdint.h>  
#include <stdbool.h>
#include "../util/utils.h"
#include "../util/stringbuilder.h"
#include "client.h"
#include "context.h"

#ifndef VERIFIER_H
#define VERIFIER_H

#define req_get(v,t,c) ctx_get_token(v->ctx->request_data,t,c)
#define res_get(v,t,c) ctx_get_token(v->ctx->response_data,t,c)
#define req_eq(v,t,c) ctx_equals(v->ctx->request_data,t,c)
#define res_eq(v,t,c) ctx_equals(v->ctx->response_data,t,c)
#define vc_err(v,e) ctx_set_error(v->ctx,e,-1)
#define req_get_param(v,i) ctx_get_array_token(req_get(v,v->request,"params"),i)
#define res_to_bytes(v,t) ctx_to_bytes(v->ctx->response_data,t,0)
#define res_to_long(v,t,def) ctx_to_long(v->ctx->response_data,t,def)


typedef struct {

   in3_ctx_t* ctx;

   in3_chain_t* chain;

   jsmntok_t* result;

   jsmntok_t* request;

   jsmntok_t* proof;

   in3_request_config_t* config;

} in3_vctx_t;

typedef int (*in3_verify)(in3_vctx_t* c );


typedef struct verifier {
   in3_verify verify;
   in3_chain_type_t type;
   struct verifier* next;
} in3_verifier_t;

/*! returns the verifier for the given chainType */
in3_verifier_t* in3_get_verifier(in3_chain_type_t type);
void in3_register_verifier(in3_verifier_t* verifier);


//in3_vc_equals()

#endif