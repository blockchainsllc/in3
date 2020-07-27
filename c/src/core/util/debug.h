
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

/** @file 
 * logs debug data only if the DEBUG-flag is set.
 * */

#ifndef IN3_EXPORT_TEST
#define IN3_EXPORT_TEST static
#endif
#include <assert.h>

#ifdef DEBUG
#define dbg_log(msg, ...)     __dbg_log(0, __FILE__, __func__, __LINE__, msg, ##__VA_ARGS__)
#define dbg_log_raw(msg, ...) __dbg_log(1, __FILE__, __func__, __LINE__, msg, ##__VA_ARGS__)
void __dbg_log(int raw, char* file, const char* func, int line, char* fmt, ...);
#else
/** logs a debug-message including file and linenumber*/
#define dbg_log(msg, ...)
/** logs a debug-message without the  filename */
#define dbg_log_raw(msg, ...)
#endif

#ifdef DBG_FNCTRACE
#define DBG_FNCTRACE_ENTER \
  { printk("ENTER %s::%s\n", __FILE__, __func__); }
#define DBG_FNCTRACE_LEAVE \
  { printk("LEAVE %s::%s\n", __FILE__, __func__); }
#endif //DBG_FNCTRACE

/** dumps the given data as hex coded bytes to stdout */
extern void msg_dump(const char* s, const unsigned char* data, unsigned len);

//#define assunme(var,expr)

#if defined(ASSERTIONS) || defined(DEBUG)
#define _assert(exp) assert(exp)
#else
#define _assert(exp)
#endif

#define EXPECT(cond, exit) \
  do {                     \
    if (!(cond))           \
      (exit);              \
  } while (0)

#define EXPECT_CFG(cond, err) EXPECT(cond, { \
  res = malloc(strlen(err) + 1);             \
  if (res) strcpy(res, err);                 \
  goto cleanup;                              \
})
#define EXPECT_CFG_NCP_ERR(cond, err) EXPECT(cond, { res = err; goto cleanup; })
#define EXPECT_TOK(token, cond, err)  EXPECT_CFG_NCP_ERR(cond, config_err(d_get_keystr(token->key), err))
#define EXPECT_TOK_BOOL(token)        EXPECT_TOK(token, d_type(token) == T_BOOLEAN, "expected boolean value")
#define EXPECT_TOK_STR(token)         EXPECT_TOK(token, d_type(token) == T_STRING, "expected string value")
#define EXPECT_TOK_ARR(token)         EXPECT_TOK(token, d_type(token) == T_ARRAY, "expected array")
#define EXPECT_TOK_OBJ(token)         EXPECT_TOK(token, d_type(token) == T_OBJECT, "expected object")
#define EXPECT_TOK_ADDR(token)        EXPECT_TOK(token, d_type(token) == T_BYTES && d_len(token) == 20, "expected address")
#define EXPECT_TOK_B256(token)        EXPECT_TOK(token, d_type(token) == T_BYTES && d_len(token) == 32, "expected 256 bit data")
#define IS_D_UINT64(token)            ((d_type(token) == T_INTEGER || (d_type(token) == T_BYTES && d_len(token) <= 8)) && d_long(token) <= UINT64_MAX)
#define IS_D_UINT32(token)            ((d_type(token) == T_INTEGER || d_type(token) == T_BYTES) && d_long(token) <= UINT32_MAX)
#define IS_D_UINT16(token)            (d_type(token) == T_INTEGER && d_int(token) >= 0 && d_int(token) <= UINT16_MAX)
#define IS_D_UINT8(token)             (d_type(token) == T_INTEGER && d_int(token) >= 0 && d_int(token) <= UINT8_MAX)
#define EXPECT_TOK_U8(token)          EXPECT_TOK(token, IS_D_UINT8(token), "expected uint8 value")
#define EXPECT_TOK_U16(token)         EXPECT_TOK(token, IS_D_UINT16(token), "expected uint16 value")
#define EXPECT_TOK_U32(token)         EXPECT_TOK(token, IS_D_UINT32(token), "expected uint32 value")
#define EXPECT_TOK_U64(token)         EXPECT_TOK(token, IS_D_UINT64(token), "expected uint64 value")
#define EXPECT_TOK_KEY_HEXSTR(token)  EXPECT_TOK(token, is_hex_str(d_get_keystr(token->key)), "expected hex str")

static inline char* config_err(const char* keyname, const char* err) {
  char* s = _malloc(strlen(keyname) + strlen(err) + 4);
  sprintf(s, "%s: %s!", keyname, err);
  return s;
}

static inline bool is_hex_str(const char* str) {
  if (str[0] == '0' && str[1] == 'x')
    str += 2;
  return str[strspn(str, "0123456789abcdefABCDEF")] == 0;
}
