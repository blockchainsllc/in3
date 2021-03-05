/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
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
#ifndef REQ_INTERNAL_H
#define REQ_INTERNAL_H

#include "plugin.h"
#include "request.h"

#ifdef LOGGING
#define req_set_error(c, msg, err) req_set_error_intern(c, msg, err)
#else
#define req_set_error(c, msg, err) req_set_error_intern(c, NULL, err)
#endif
#define REQUIRE_EXPERIMENTAL(req, feature) \
  if ((req->client->flags & FLAGS_ALLOW_EXPERIMENTAL) == 0) return req_set_error(req, "The feature " feature " is still experimental. You need to explicitly allow it in the config.", IN3_ECONFIG);
/**
 * creates a request-object, which then need to be filled with the responses.
 *
 * each request object contains a array of reponse-objects. In order to set the response, you need to call
 *
 * ```c
 * // set a succesfull response
 * sb_add_chars(&request->results[0].result, my_response);
 * // set a error response
 * sb_add_chars(&request->results[0].error, my_error);
 * ```
 */
NONULL in3_http_request_t* in3_create_request(
    in3_req_t* req /**< [in] the request context. */
);

/**
 * frees a previuosly allocated request.
 */
NONULL void request_free(
    in3_http_request_t* req /**< [in] the request. */
);

/**
 * sets the error message in the context.
 *
 * If there is a previous error it will append it.
 * the return value will simply be passed so you can use it like
 *
 * ```c
 *   return req_set_error(ctx, "wrong number of arguments", IN3_EINVAL)
 * ```
 */
in3_ret_t req_set_error_intern(
    in3_req_t* c,        /**< [in] the current request context. */
    char*      msg,      /**< [in] the error message. (This string will be copied) */
    in3_ret_t  errnumber /**< [in] the error code to return */
);

/**
 * handles a failable context
 *
 * This context *MUST* be freed with req_free(ctx) after usage to release the resources.
*/
in3_ret_t req_handle_failable(
    in3_req_t* req /**< [in] the current request context. */
);

NONULL_FOR((1, 2, 3, 5))
in3_ret_t        req_send_sub_request(in3_req_t* parent, char* method, char* params, char* in3, d_token_t** result);
NONULL in3_ret_t req_require_signature(in3_req_t* req, d_signature_type_t type, bytes_t* sig, bytes_t raw_data, bytes_t from);
NONULL in3_ret_t in3_retry_same_node(in3_req_t* req);

#define assert_in3_req(ctx)                                                                    \
  assert(ctx);                                                                                 \
  assert_in3(ctx->client);                                                                     \
  assert(ctx->signers_length <= (ctx->type == RT_RPC ? ctx->client->signature_count + 1 : 0)); \
  assert(ctx->signers_length ? (ctx->signers != NULL) : (ctx->signers == NULL));               \
  assert(ctx->len >= 1 || ctx->error);                                                         \
  assert(ctx->attempt <= ctx->client->max_attempts);                                           \
  assert(!ctx->len || ctx->request_context);                                                   \
  assert(!ctx->len || ctx->requests);                                                          \
  assert(!ctx->len || ctx->requests[0]);                                                       \
  assert(!ctx->len || ctx->requests[ctx->len - 1]);                                            \
  assert(ctx->error ? (ctx->verification_state < 0) : (ctx->verification_state == IN3_OK || ctx->verification_state == IN3_WAITING));

#define assert_in3_response(r) \
  assert(r);                   \
  assert(r->state != IN3_OK || r->data.data);

NONULL void in3_req_free_nodes(node_match_t* c);
int         req_nodes_len(node_match_t* root);
NONULL bool req_is_method(const in3_req_t* req, const char* method);

#endif // REQ_INTERNAL_H