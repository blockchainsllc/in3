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

#ifndef IN3_API_UTILS_PRIV_H
#define IN3_API_UTILS_PRIV_H

#include "../../core/client/context.h"
#include "../../core/client/keys.h"
#include "../../core/util/data.h"
#include "../../core/util/error.h"
#include "../../core/util/log.h"
#include "../../core/util/utils.h"
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

void api_set_error(int err, const char* msg);

/** returns the result from a previously executed ctx */
d_token_t* get_result(in3_ctx_t* ctx);

#endif //IN3_API_UTILS_PRIV_H
