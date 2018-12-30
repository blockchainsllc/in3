/** @file 
 * Verification Context.
 * This context is passed to the verifier.
 * */

#include "../util/data.h"
#include "../util/stringbuilder.h"
#include "../util/utils.h"
#include "client.h"
#include "context.h"
#include <stdbool.h>
#include <stdint.h>

#ifndef VERIFIER_H
#define VERIFIER_H
/**
 * creates an error attaching it to the context and returns -1 as a return value.
 */
#define vc_err(v, e) ctx_set_error(v->ctx, e, -1)

/**
 * verification context holding the pointers to all relevant toknes.
 */
typedef struct {
  in3_ctx_t*            ctx;     /**< Request context. */
  in3_chain_t*          chain;   /**< the chain definition. */
  d_token_t*            result;  /**< the result to verify */
  d_token_t*            request; /**< the request sent. */
  d_token_t*            proof;   /**< the delivered proof. */
  in3_request_config_t* config;  /**< Request configuration. */
} in3_vctx_t;

/**
 * function to verify the result.
 */
typedef int (*in3_verify)(in3_vctx_t* c);

typedef struct verifier {
  in3_verify       verify;
  in3_chain_type_t type;
  struct verifier* next;
} in3_verifier_t;

/*! returns the verifier for the given chainType */
in3_verifier_t* in3_get_verifier(in3_chain_type_t type);
void            in3_register_verifier(in3_verifier_t* verifier);

//in3_vc_equals()

#endif