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

#include "client.h"
#include "../util/data.h"
#include "../util/mem.h"
#include "context.h"
#include "keys.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

in3_ctx_t* in3_client_rpc_ctx_raw(in3_t* c, char* req) {
  // create a new context by parsing the request
  in3_ctx_t* ctx = ctx_new(c, req);

  // this happens if the request is not parseable (JSON-error in params)
  if (ctx->error) {
    ctx->verification_state = IN3_EINVAL;
    return ctx;
  }

  // execute it
  in3_ret_t ret = in3_send_ctx(ctx);
  if (ret == IN3_OK) {
    // the request was succesfull, so we delete interim errors (which can happen in case in3 had to retry)
    if (ctx->error) _free(ctx->error);
    ctx->error = NULL;
  } else
    ctx->verification_state = ret;

  return ctx; // return context and hope the calle will clean it.
}

in3_ctx_t* in3_client_rpc_ctx(in3_t* c, char* method, char* params) {
  // generate the rpc-request
  const int  max  = strlen(method) + strlen(params) + 200;                                              // determine the max length of the request string
  const bool heap = max > 500;                                                                          // if we need more than 500 bytes, we better put it in the heap
  char*      req  = heap ? _malloc(max) : alloca(max);                                                  // allocate memory in heap or stack
  if (!req) return NULL;                                                                                // if we don't have the memory for a string, we stop here
  snprintX(req, max, "{\"method\":\"%s\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":%s}", method, params); // create request

  in3_ctx_t* ctx = in3_client_rpc_ctx_raw(c, req);

  if (heap) _free(req); // free request string if we created it in heap
  return ctx;           // return context and hope the calle will clean it.
}

static in3_ret_t ctx_rpc(in3_ctx_t* ctx, char** result, char** error) {
  if (result) result[0] = 0;
  *error = NULL;

  in3_ret_t res = ctx ? ctx->verification_state : IN3_ENOMEM;
  if (!ctx) return res;

  // check parse-errors
  if (ctx->error) {
    // we have an error
    *error = _malloc(strlen(ctx->error) + 1);
    strcpy(*error, ctx->error);
    if (res == IN3_OK) res = IN3_EUNKNOWN;
    goto clean;
  }

  // there was an error, but no message, so we create one
  if (res != IN3_OK) {
    *error = _strdupn(in3_errmsg(res), -1);
    goto clean;
  }

  // do we have an error-property in the response?
  d_token_t* r = d_get(ctx->responses[0], K_ERROR);
  if (r) {
    if (d_type(r) == T_STRING)
      *error = _strdupn(d_string(r), -1);
    else if (d_type(r) == T_OBJECT) {
      char* msg = d_get_stringk(r, K_MESSAGE);
      *error    = msg ? _strdupn(msg, -1) : d_create_json(r);
    } else
      *error = d_create_json(r);
    res = IN3_ERPC;
    goto clean;
  }

  if ((r = d_get(ctx->responses[0], K_RESULT)) == NULL) {
    // we have no result
    *error = _strdupn("no result or error in rpc-response", -1);
    res    = IN3_ERPC;
    goto clean;
  }

  // we have a result and copy it
  if (result) *result = d_create_json(r);

clean:
  ctx_free(ctx);

  // if we have an error, we always return IN3_EUNKNOWN
  return res;
}

in3_ret_t in3_client_rpc(in3_t* c, char* method, char* params, char** result, char** error) {
  if (!error) return IN3_EINVAL;
  return ctx_rpc(in3_client_rpc_ctx(c, method, params), result, error);
}

in3_ret_t in3_client_rpc_raw(in3_t* c, char* request, char** result, char** error) {
  if (!error) return IN3_EINVAL;
  return ctx_rpc(in3_client_rpc_ctx_raw(c, request), result, error);
}

static char* create_rpc_error(uint32_t id, int code, char* error) {
  char* res = _malloc(strlen(error) + 100);
  if (res) sprintf(res, "{\"id\":%d,\"jsonrpc\":\"2.0\",\"error\":{\"code\":%i,\"message\":\"%s\"}}", id, code, error);
  return res;
}
char* in3_client_exec_req(
    in3_t* c,  /**< [in] the pointer to the incubed client config. */
    char*  req /**< [in] the request as rpc. */
) {
  // parse it
  char*      res = NULL;
  in3_ctx_t* ctx = ctx_new(c, req);
  in3_ret_t  ret;

  //  not enough memory
  if (!ctx) return NULL;

  // make sure result & error are clean
  // check parse-errors
  if (ctx->error) {
    res = create_rpc_error(0, -32700, ctx->error);
    goto clean;
  }

  uint32_t id = d_get_intk(ctx->requests[0], K_ID);
  ret         = in3_send_ctx(ctx);

  // do we have an error?
  if (ctx->error) {
    res = create_rpc_error(id, ret ? ret : ctx->verification_state, ctx->error);
    goto clean;
  }

  // no error message, but an error-code?
  if (ret != IN3_OK) {
    res = create_rpc_error(id, ret, in3_errmsg(ret));
    goto clean;
  }

  // looks good, so we use the resonse and return it
  str_range_t rr = d_to_json(ctx->responses[0]), rin3;
  if ((c->flags & FLAGS_KEEP_IN3) == 0 && (rin3 = d_to_json(d_get(ctx->responses[0], K_IN3))).data) {
    while (*rin3.data != ',' && rin3.data > rr.data) rin3.data--;
    *rin3.data = '}';
    rr.len     = rin3.data - rr.data + 1;
  }
  res         = _malloc(rr.len + 1);
  res[rr.len] = 0; // we can now manipulating the response, since we will free it anyway.
  if (res) memcpy(res, rr.data, rr.len);

clean:

  ctx_free(ctx);
  return res;
}

/**
 * create a new signer-object to be set on the client.
 * the caller will need to free this pointer after usage.
 */
in3_signer_t* in3_create_signer(
    in3_sign       sign,       /**< function pointer returning a stored value for the given key.*/
    in3_prepare_tx prepare_tx, /**< function pointer returning capable of manipulating the transaction before signing it. This is needed in order to support multisigs.*/
    void*          wallet      /**<custom object whill will be passed to functions */
) {
  in3_signer_t* signer = _calloc(1, sizeof(in3_signer_t));
  signer->wallet       = wallet;
  signer->sign         = sign;
  signer->prepare_tx   = prepare_tx;
  return signer;
}

in3_storage_handler_t* in3_create_storage_handler(
    in3_storage_get_item get_item, /**< function pointer returning a stored value for the given key.*/
    in3_storage_set_item set_item, /**< function pointer setting a stored value for the given key.*/
    in3_storage_clear    clear,    /**< function pointer setting a stored value for the given key.*/
    void*                cptr      /**< custom pointer which will will be passed to functions */
) {
  in3_storage_handler_t* handler = _calloc(1, sizeof(in3_storage_handler_t));
  handler->cptr                  = cptr;
  handler->get_item              = get_item;
  handler->set_item              = set_item;
  handler->clear                 = clear;
  return handler;
}
