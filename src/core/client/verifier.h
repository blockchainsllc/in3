/** @file 
 * Verification Context.
 * This context is passed to the verifier.
 * */ 


#include <stdint.h>  
#include <stdbool.h>
#include "../util/utils.h"
#include "../util/stringbuilder.h"
#include "client.h"
#include "context.h"
#include "../util/data.h"

#ifndef VERIFIER_H
#define VERIFIER_H
/** gets a child of the given request-token with the given name*/
/*
#define req_get(v,t,c) ctx_get_token(v->ctx->request_data,t,c)
#define res_get(v,t,c) ctx_get_token(v->ctx->response_data,t,c)
#define req_eq(v,t,c) ctx_equals(v->ctx->request_data,t,c)
#define res_eq(v,t,c) ctx_equals(v->ctx->response_data,t,c)
#define req_get_param(v,i) ctx_get_array_token(req_get(v,v->request,"params"),i)
#define req_to_bytes(v,t) ctx_to_bytes(v->ctx->request_data,t,0)
#define res_to_bytes(v,t) ctx_to_bytes(v->ctx->response_data,t,0)
#define res_to_long(v,t,def) ctx_to_long(v->ctx->response_data,t,def)
#define res_get_long(v,t,n,def) ctx_to_long(v->ctx->response_data, res_get(v,t,n) ,def)
#define res_get_int(v,t,n,def) ctx_to_int(v->ctx->response_data, res_get(v,t,n) ,def)
#define res_prop_to_bytes_a(v,t,prop) ctx_to_byte_a(v->ctx->response_data, ctx_get_token(v->ctx->response_data,t,prop))
*/
#define vc_err(v,e) ctx_set_error(v->ctx,e,-1)

typedef struct {

   in3_ctx_t* ctx;

   in3_chain_t* chain;

   d_token_t* result;

   d_token_t* request;

   d_token_t* proof;

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