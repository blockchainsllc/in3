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

#ifndef IN3_API_UTILS_H
#define IN3_API_UTILS_H

#include "../core/client/context.h"
#include "../core/client/keys.h"
#include "../core/util/data.h"
#include "../core/util/error.h"
#include "../core/util/log.h"
#include "../core/util/utils.h"
#ifdef __ZEPHYR__
#include <zephyr.h>
#else
#include <errno.h>
#endif
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#else
#include <time.h>
#endif

// create the params as stringbuilder
#define rpc_init sb_t* params = sb_new("[")

// execute the request after the params have been set.
#define rpc_exec(METHOD, RETURN_TYPE, HANDLE_RESULT)                                      \
  errno              = 0;                                                                 \
  in3_ctx_t*  _ctx_  = in3_client_rpc_ctx(in3, (METHOD), sb_add_char(params, ']')->data); \
  d_token_t*  result = get_result(_ctx_);                                                 \
  RETURN_TYPE _res_;                                                                      \
  if (result)                                                                             \
    _res_ = (HANDLE_RESULT);                                                              \
  else                                                                                    \
    memset(&_res_, 0, sizeof(RETURN_TYPE));                                               \
  ctx_free(_ctx_);                                                                        \
  sb_free(params);                                                                        \
  return _res_;

#define params_add_key_pair(params, key, sb_add_func, quote_val, prefix_comma) \
  do {                                                                         \
    if (prefix_comma) sb_add_chars(params, ", ");                              \
    sb_add_char(params, '\"');                                                 \
    sb_add_chars(params, key);                                                 \
    sb_add_chars(params, "\": ");                                              \
    if (quote_val) sb_add_char(params, '\"');                                  \
    sb_add_func;                                                               \
    if (quote_val) sb_add_char(params, '\"');                                  \
  } while (0)

#define params_add_first_pair(params, key, sb_add_func, quote_val) params_add_key_pair(params, key, sb_add_func, quote_val, false)
#define params_add_next_pair(params, key, sb_add_func, quote_val) params_add_key_pair(params, key, sb_add_func, quote_val, true)

// last error string
static char* last_error = NULL;
char*        eth_last_error() { return last_error; }

// sets the error and a message
static void set_errorn(int std_error, char* msg, int len) {
  errno = std_error;
  if (last_error) _free(last_error);
  last_error = _malloc(len + 1);
  memcpy(last_error, msg, len);
  last_error[len] = 0;
}

// sets the error and a message
static void set_error_intern(int std_error, char* msg) {
#ifndef __ZEPHYR__
  in3_log_error("Request failed due to %s - %s\n", strerror(std_error), msg);
#else
  in3_log_error("Request failed due to %s\n", msg);
#endif
  set_errorn(std_error, msg, strlen(msg));
}

#ifdef ERR_MSG
#define set_error(e, msg) set_error_intern(e, msg)
#else
#define set_error(e, msg) set_error_intern(e, "E")
#endif

/** returns the result from a previously executed ctx*/
static inline d_token_t* get_result(in3_ctx_t* ctx) {
  if (ctx->error) {                   // error means something went wrong during verification or a timeout occured.
    set_error(ETIMEDOUT, ctx->error); // so we copy the error as last_error
    return NULL;
  } else if (!ctx->responses) {
    set_error(IN3_ERPC, "No response");
    return NULL;
  }

  d_token_t* t = d_get(ctx->responses[0], K_RESULT);
  if (t) return t; // everthing is good, we have a result

  // if no result, we expect an error
  t = d_get(ctx->responses[0], K_ERROR); // we we have an error...
  set_error(ETIMEDOUT, !t
                           ? "No result or error in response"
                           : (d_type(t) == T_OBJECT ? d_string(t) : d_get_stringk(t, K_MESSAGE)));
  return NULL;
}

#endif //IN3_API_UTILS_H
