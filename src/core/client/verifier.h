// @PUBLIC_HEADER
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
 * verification context holding the pointers to all relevant toknes.
 */
typedef struct {
  in3_ctx_t*            ctx;                   /**< Request context. */
  in3_chain_t*          chain;                 /**< the chain definition. */
  d_token_t*            result;                /**< the result to verify */
  d_token_t*            request;               /**< the request sent. */
  uint64_t              id;                    /**< the id of sent request. */
  d_token_t*            proof;                 /**< the delivered proof. */
  in3_request_config_t* config;                /**< Request configuration. */
  uint64_t              last_validator_change; /**< Block number of last change of the validator list */
  uint64_t              currentBlock;          /**< Block number of latest block */
} in3_vctx_t;

/**
 * function to verify the result.
 */
typedef in3_ret_t (*in3_verify)(in3_vctx_t* c);
typedef in3_ret_t (*in3_pre_handle)(in3_ctx_t* ctx, in3_response_t** response);

typedef struct verifier {
  in3_verify       verify;
  in3_pre_handle   pre_handle;
  in3_chain_type_t type;
  struct verifier* next;
} in3_verifier_t;

/*! returns the verifier for the given chainType */
in3_verifier_t* in3_get_verifier(in3_chain_type_t type);
void            in3_register_verifier(in3_verifier_t* verifier);
in3_ret_t       vc_err(in3_vctx_t* vc, char* msg); /* creates an error attaching it to the context and returns -1. */

#endif