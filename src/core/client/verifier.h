/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 *
 * Copyright (C) 2019 Blockchains, LLC
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
 * 
 * If you cannot meet the requirements of AGPL, 
 * you should contact us to inquire about a commercial license.
 *******************************************************************************/

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