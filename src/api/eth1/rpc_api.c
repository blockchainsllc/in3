/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2019 slock.it GmbH, Blockchains LLC
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

#include "../../core/client/context.h"
#include "../../core/client/keys.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "../../verifier/eth1/full/eth_full.h"
#include "../../verifier/eth1/nano/rlp.h"
#include "abi.h"
#include "eth_api.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#define RESPONSE_START()                                                             \
  do {                                                                               \
    *response = _malloc(sizeof(in3_response_t));                                     \
    sb_init(&response[0]->result);                                                   \
    sb_init(&response[0]->error);                                                    \
    sb_add_chars(&response[0]->result, "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":"); \
  } while (0)

#define RESPONSE_END() \
  do { sb_add_char(&response[0]->result, '}'); } while (0)

static in3_ret_t eth_handle_intern(in3_ctx_t* ctx, in3_response_t** response) {
  if (ctx->len > 1) return IN3_ENOTSUP; // internal handling is only possible for single requests (at least for now)
  d_token_t* r      = ctx->requests[0];
  char*      method = d_get_stringk(r, K_METHOD);
  d_token_t* params = d_get(r, K_PARAMS);
  if (strcmp(method, "in3_abiEncode") == 0) {
    RESPONSE_START();
    call_request_t* req = parseSignature(d_get_string_at(params, 0));
    if (!req) return ctx_set_error(ctx, "invalid function signature", IN3_EINVAL);

    d_token_t* para = d_get_at(params, 1);
    if (!para) {
      req_free(req);
      return ctx_set_error(ctx, "invalid json data", IN3_EINVAL);
    }

    if (set_data(req, para, req->in_data) < 0) {
      req_free(req);
      return ctx_set_error(ctx, "invalid input data", IN3_EINVAL);
    }
    sb_add_bytes(&response[0]->result, NULL, &req->call_data->b, 1, false);
    RESPONSE_END();
  }

  return IN3_OK;
}

static int verify(in3_vctx_t* v) {
  char* method = d_get_stringk(v->request, K_METHOD);
  if (!method) return vc_err(v, "no method in the request!");

  if (strcmp(method, "in3_abiEncode") == 0)
    return IN3_OK;

  return in3_verify_eth_full(v);
}

void in3_register_eth_api() {
  in3_verifier_t* v = _calloc(1, sizeof(in3_verifier_t));
  v->type           = CHAIN_ETH;
  v->pre_handle     = eth_handle_intern;
  v->verify         = verify;
  ;
  in3_register_verifier(v);
}
