/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
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

#ifdef LOGGING
#define vc_err(vc, msg) vc_set_error(vc, msg)
#else
#define vc_err(vc, msg) vc_set_error(vc, NULL)
#endif

/**
 * verification context holding the pointers to all relevant toknes.
 */
typedef struct {
  in3_ctx_t*            ctx;                   /**< Request context. */
  in3_chain_t*          chain;                 /**< the chain definition. */
  d_token_t*            result;                /**< the result to verify */
  d_token_t*            request;               /**< the request sent. */
  d_token_t*            proof;                 /**< the delivered proof. */
  in3_request_config_t* config;                /**< Request configuration. */
  uint64_t              last_validator_change; /**< Block number of last change of the validator list */
  uint64_t              currentBlock;          /**< Block number of latest block */
} in3_vctx_t;

/**
 * function to verify the result.
 */
typedef in3_ret_t (*in3_verify)(in3_vctx_t* c);
/**
 * function which is called to fill the response before a request is triggered.
 * This can be used to handle requests which don't need a node to response.
 */
typedef in3_ret_t (*in3_pre_handle)(in3_ctx_t* ctx, in3_response_t** response);
/**
 * function which is called when the client is being configured. This allows the verifier to add a custom-config to the chain based on the configuration.
 */
typedef in3_ret_t (*in3_vc_set_config)(in3_t* c, d_token_t* conf, in3_chain_t* chain);

/**
 * Function which is called before the chain-instance is freed. Here the verifier should clean up resources.
 */
typedef void (*in3_vc_free)(in3_t* c, in3_chain_t* chain);

/**
 * a Verifier.
 * 
 * Verifiers are registered globaly, but for a specific chain_type. Depending on the chain_type the first verifier is picked.
 */
typedef struct verifier {
  in3_verify        verify;     /**< the verify-function is called by the core. The result is either IN3_OK, IN3_WAITING (if a subrequest was added) or an error code. */
  in3_pre_handle    pre_handle; /**< called before sending the request and allows to manipulate or provide a raw_response to handle it internally. */
  in3_vc_set_config set_confg;  /**< When configuring the client, each verifier will be passed a config-object. */
  in3_vc_free       free_chain; /**< if this function is set, it will be called whenever a chain-instance is freed. */
  in3_chain_type_t  type;       /**< type of the chain, which is used when find a matching verifier. */
  struct verifier*  next;       /**< Since verifiers are organized in a linked list the next-pointer connects the registered verifiers. */
} in3_verifier_t;

/*! returns the verifier for the given chainType */
NONULL in3_verifier_t* in3_get_verifier(in3_chain_type_t type);
NONULL void            in3_register_verifier(in3_verifier_t* verifier);
#ifdef LOGGING
NONULL
#endif
in3_ret_t vc_set_error(in3_vctx_t* vc, char* msg); /* creates an error attaching it to the context and returns -1. */

#endif