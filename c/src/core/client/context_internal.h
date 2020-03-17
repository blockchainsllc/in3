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
#ifndef CONTEXT_INTERNAL_H
#define CONTEXT_INTERNAL_H

#include "context.h"

#ifdef ERR_MSG
#define ctx_set_error(c, msg, err) ctx_set_error_intern(c, msg, err)
#else
#define ctx_set_error(c, msg, err) ctx_set_error_intern(c, NULL, err)
#endif

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
in3_request_t* in3_create_request(
    in3_ctx_t* ctx /**< [in] the request context. */
);

/**
 * frees a previuosly allocated request.
 */
void request_free(
    in3_request_t*   req,          /**< [in] the request. */
    const in3_ctx_t* ctx,          /**< [in] the request context. */
    bool             response_free /**< [in] if true the responses will freed also, but usually this is done when the ctx is freed. */
);

/**
 * sets the error message in the context.
 *
 * If there is a previous error it will append it.
 * the return value will simply be passed so you can use it like
 *
 * ```c
 *   return ctx_set_error(ctx, "wrong number of arguments", IN3_EINVAL)
 * ```
 */
in3_ret_t ctx_set_error_intern(
    in3_ctx_t* c,        /**< [in] the current request context. */
    char*      msg,      /**< [in] the error message. (This string will be copied) */
    in3_ret_t  errnumber /**< [in] the error code to return */
);

/**
 * handles a failable context
 *
 * This context *MUST* be freed with ctx_free(ctx) after usage to release the resources.
*/
in3_ret_t ctx_handle_failable(
    in3_ctx_t* ctx /**< [in] the current request context. */
);

#endif // CONTEXT_INTERNAL_H