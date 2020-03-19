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

#include "api_utils.h"
#include "../../core/util/mem.h"
#include "api_utils_priv.h"

// forward decl
void  set_error(int err, const char* msg);
char* get_error(void);

// last error string
static char* last_error = NULL;
// API get error function
static get_error_fn get_error_impl = get_error;
// API set error function
static set_error_fn set_error_impl = set_error;

void api_get_error_fn(get_error_fn fn) {
  get_error_impl = fn;
}

void api_set_error_fn(set_error_fn fn) {
  set_error_impl = fn;
}

// sets the error and a message
static void set_errorn(int std_error, const char* msg, int len) {
  errno = std_error;
  if (last_error) _free(last_error);
  last_error = _malloc(len + 1);
  memcpy(last_error, msg, len);
  last_error[len] = 0;
}

// sets the error and a message
static void set_error_intern(int std_error, const char* msg) {
#ifndef __ZEPHYR__
  in3_log_error("Request failed due to %s - %s\n", strerror(std_error), msg);
#else
  in3_log_error("Request failed due to %s\n", msg);
#endif
  set_errorn(std_error, msg, strlen(msg));
}

char* get_error(void) {
  return last_error;
}

char* api_last_error(void) {
  return get_error_impl();
}

void set_error(int err, const char* msg) {
#ifdef ERR_MSG
  return set_error_intern(err, msg);
#else
  return set_error_intern(err, "E");
#endif
}

void api_set_error(int err, const char* msg) {
  return set_error_impl(err, msg ? msg : "unknown error");
}

d_token_t* get_result(in3_ctx_t* ctx) {
  if (ctx->error) {                       // error means something went wrong during verification or a timeout occured.
    api_set_error(ETIMEDOUT, ctx->error); // so we copy the error as last_error
    return NULL;
  } else if (!ctx->responses) {
    api_set_error(IN3_ERPC, "No response");
    return NULL;
  }

  d_token_t* t = d_get(ctx->responses[0], K_RESULT);
  if (t) return t; // everthing is good, we have a result

  // if no result, we expect an error
  t = d_get(ctx->responses[0], K_ERROR); // we we have an error...
  api_set_error(ETIMEDOUT, !t
                               ? "No result or error in response"
                               : (d_type(t) != T_OBJECT ? d_string(t) : d_get_stringk(t, K_MESSAGE)));
  return NULL;
}

uint256_t to_uint256(uint64_t value) {
  uint256_t data;
  memset(data.data, 0, 32);
  long_to_bytes(value, data.data + 24);
  return data;
}

/** converts a uint256 to a long double */
long double as_double(uint256_t d) {
  uint8_t* p = d.data;
  int      l = 32;
  optimize_len(p, l);
  if (l < 9)
    return bytes_to_long(p, l);
  else {
    long double val = 6277101735386680763835789423207666416102355444464034512896.0L * bytes_to_long(d.data, 8);
    val += 340282366920938463463374607431768211456.0L * bytes_to_long(d.data + 8, 8);
    val += 18446744073709551616.0L * bytes_to_long(d.data + 16, 8);
    return val + bytes_to_long(d.data + 24, 8);
  }
}
/** converts a uint256_t to a int (by taking the last 8 bytes) */
uint64_t as_long(uint256_t d) {
  return bytes_to_long(d.data + 32 - 8, 8);
}
