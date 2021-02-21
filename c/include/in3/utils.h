/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
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
 * utility functions.
 * */

#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bytes.h"
#include <assert.h>
#include <stdint.h>

#ifdef __ZEPHYR__
#include <zephyr.h>
#define _strtoull(str, endptr, base) strtoul(str, endptr, base)
#else
#define _strtoull(str, endptr, base) strtoull(str, endptr, base)
#endif

/** simple swap macro for integral types */
#define SWAP(a, b) \
  {                \
    void* p = a;   \
    a       = b;   \
    b       = p;   \
  }

#ifndef min
/** simple min macro for interagl types */
#define min(a, b) ((a) < (b) ? (a) : (b))
/** simple max macro for interagl types */
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

/**
 *  Check if n1 & n2 are at max err apart
 * Expects n1 & n2 to be integral types
 */
#define IS_APPROX(n1, n2, err)    ((n1 > n2) ? ((n1 - n2) <= err) : ((n2 - n1) <= err))
#define DIFF_ATMOST(n1, n2, diff) IS_APPROX(n1, n2, diff)
#define DIFF_ATLEAST(n1, n2, err) ((n1 > n2) ? ((n1 - n2) >= err) : ((n2 - n1) >= err))

/**
 * simple macro to stringify other macro defs
 * eg. usage - to concatenate a const with a string at compile time ->
 * #define SOME_CONST_UINT 10U
 * printf("Using default value of " STR(SOME_CONST_UINT));
 */
#define STR_IMPL_(x) #x
#define STR(x)       STR_IMPL_(x)

/** converts the bytes to a unsigned long (at least the last max len bytes). */
uint64_t bytes_to_long(const uint8_t* data, int len);

/** converts the bytes to a unsigned int (at least the last max len bytes) */
static inline uint32_t bytes_to_int(const uint8_t* data, int len) {
  if (data) {
    switch (len) {
      case 0: return 0;
      case 1: return data[0];
      case 2: return (((uint32_t) data[0]) << 8) | data[1];
      case 3: return (((uint32_t) data[0]) << 16) | (((uint32_t) data[1]) << 8) | data[2];
      default: return (((uint32_t) data[0]) << 24) | (((uint32_t) data[1]) << 16) | (((uint32_t) data[2]) << 8) | data[3];
    }
  }
  else
    return 0;
}
/** converts a character into a uint64_t*/
uint64_t char_to_long(const char* a, int l);

/**  converts a hexchar to byte (4bit). In case of a nonhex char 0xff will be returned. */
uint8_t hexchar_to_int(char c);

#ifdef __ZEPHYR__
// this function is only used in zephyr, because there it does not support printf("%ull",u64);

/** converts a uint64_t to string (char*); buffer-size min. 21 bytes */
const char* u64_to_str(uint64_t value, char* pBuf, int szBuf);

#endif

/**
 * convert a c hex string to a byte array storing it into an existing buffer.
 * 
 * @param  hexdata: the hex string
 * @param  hexlen: the len of the string to read. -1 will use strlen to determine the length.
 * @param  out: the byte buffer
 * @param  outlen: the length of the buffer to write into
 * @retval the  number of bytes written
 */
int hex_to_bytes(const char* hexdata, int hexlen, uint8_t* out, int outlen);

/** convert a c string to a byte array creating a new buffer */
bytes_t* hex_to_new_bytes(const char* buf, int len);

/** convefrts a bytes into hex */
int bytes_to_hex(const uint8_t* buffer, int len, char* out);

/** writes 32 bytes to the pointer. */
int keccak(bytes_t data, void* dst);

/** converts a a uin64_t to 8 bytes written to dst using big endian*/
void long_to_bytes(uint64_t val, uint8_t* dst);

/** converts a unsigned int to 4 bytes written to dst using big endian*/
void int_to_bytes(uint32_t val, uint8_t* dst);

/** duplicate the string. A len=-1 will determine the len with strlen. */
char* _strdupn(const char* src, int len);

/** calculate the min number of byte to represents the len */
int min_bytes_len(uint64_t val);

/**
 * sets a variable value to 32byte word.
 * @param src The src data
 * @param src_len the number of bytes 
 * @param dst target pointer
 */
void uint256_set(const uint8_t* src, wlen_t src_len, bytes32_t dst);

/**
 * replaces a string and returns a copy.
 * @retval 
 */
char* str_replace(char* orig, const char* rep, const char* with);

/**
 * replaces a string at the given position.
 */
char* str_replace_pos(char* orig, size_t pos, size_t len, const char* rep);

/**
  * lightweight strstr() replacements
  */
char* str_find(char* haystack, const char* needle);

/**
 * remove all html-tags in the text. This function will modify the orifinal data and return the same pointer as the input.
 */
char* str_remove_html(char* data);

/**
 * current timestamp in ms. 
 */
uint64_t current_ms();

/** changes to pointer (a) and it length (l) to remove leading 0 bytes. it will reduce  it to max len=1*/
#define optimize_len(a, l)   \
  while (l > 1 && *a == 0) { \
    l--;                     \
    a++;                     \
  }

/**
 * executes the expression and expects the return value to be a int indicating the error. 
 * if the return value is negative it will stop and return this value otherwise continue. 
 */
#define TRY(exp)           \
  {                        \
    int _r = (exp);        \
    if (_r < 0) return _r; \
  }

/**
 * executes the expression and expects the return value to be a int indicating the error. 
 * if the return value is negative it will stop and return this value otherwise continue. 
 */
#define TRY_FINAL(exp, final) \
  {                           \
    int _r = (exp);           \
    final;                    \
    if (_r < 0) return _r;    \
  }

/**
 * executes the expression and expects the return value to be a int indicating the error. 
 * if the return value is negative it will stop and return this value otherwise continue.
 */
#define TRY_CATCH(exp, catch) \
  {                           \
    int _r = (exp);           \
    if (_r < 0) {             \
      catch;                  \
      return _r;              \
    }                         \
  }

/**
 * executes the expression and expects value to equal val. 
 * if not it will return IN3_EINVAL
 */
#define EXPECT_EQ(exp, val) \
  if ((exp) != val) return IN3_EINVAL;

/**
 * executes the expression and expects the return value to be a int indicating the error. 
 * the return value will be set to a existing variable (var).
 * if the return value is negative it will stop and return this value otherwise continue. 
 */
#define TRY_SET(var, exp)    \
  {                          \
    var = (exp);             \
    if (var < 0) return var; \
  }

/**
 * executes the expression and expects the return value to be a int indicating the error. 
 * if the return value is negative it will stop and jump (goto) to a marked position "clean".
 * it also expects a previously declared variable "in3_ret_t res".
 */
#define TRY_GOTO(exp)        \
  {                          \
    res = (exp);             \
    if (res < 0) goto clean; \
  }

/**
 * returns true if all pytes (specified by l) of pts have a value of zero.
 */
static inline bool memiszero(uint8_t* ptr, size_t l) {
  assert(l > 0);
  while (l) {
    if (*ptr) return false;
    l--;
    ptr++;
  }
  return true;
}

/**
 * Pluggable functions:
 * Mechanism to replace library functions with custom alternatives. This is particularly useful for
 * embedded systems which have their own time or rand functions.
 *
 * eg.
 * // define function with specified signature
 * uint64_t my_time(void* t) {
 *  // ...
 * }
 *
 * // then call in3_set_func_*()
 * int main() {
 *  in3_set_func_time(my_time);
 *  // Henceforth, all library calls will use my_time() instead of the platform default time function
 * }
 */

/**
 * time function
 * defaults to k_uptime_get() for zeohyr and time(NULL) for other platforms
 * expected to return a u64 value representative of time (from epoch/start)
 */
typedef uint64_t (*time_func)(void* t);
void     in3_set_func_time(time_func fn);
uint64_t in3_time(void* t);

/**
 * rand function
 * defaults to k_uptime_get() for zeohyr and rand() for other platforms
 * expected to return a random number
 */
typedef int (*rand_func)(void* s);
void in3_set_func_rand(rand_func fn);
int  in3_rand(void* s);

/**
 * srand function
 * defaults to NOOP for zephyr and srand() for other platforms
 * expected to set the seed for a new sequence of random numbers to be returned by in3_rand()
 */
typedef void (*srand_func)(unsigned int s);
void in3_set_func_srand(srand_func fn);
void in3_srand(unsigned int s);
void in3_sleep(uint32_t ms);
/**
 * parses a float-string and returns the value as int
 */
int64_t parse_float_val(const char* data, /**< the data string*/
                        int32_t     expo  /**< the exponent */
);

#ifdef THREADSAFE
#define _NAME(x, y) x##y
#if defined(_MSC_VER) || defined(__MINGW32__)

#include <windows.h>

#define INIT_LOCK(NAME)                                                                                               \
  static HANDLE _NAME(_lock_handle_, NAME) = NULL;                                                                    \
  static void   _NAME(_lock, NAME)() {                                                                                \
    if (!_NAME(_lock_handle_, NAME)) {                                                                              \
      HANDLE p = CreateMutex(NULL, FALSE, NULL);                                                                    \
      if (InterlockedCompareExchangePointer((PVOID*) &_NAME(_lock_handle_, NAME), (PVOID) p, NULL)) CloseHandle(p); \
    }                                                                                                               \
    WaitForSingleObject(_NAME(_lock_handle_, NAME), INFINITE);                                                      \
  }

#define LOCK(NAME, code)                          \
  {                                               \
    _NAME(_lock, NAME)                            \
    ();                                           \
    code                                          \
        ReleaseMutex(_NAME(_lock_handle_, NAME)); \
  }

#else
#include <pthread.h>
#define INIT_LOCK(NAME) static pthread_mutex_t _NAME(_lock_handle_, NAME) = PTHREAD_MUTEX_INITIALIZER;
#define LOCK(NAME, code)                                     \
  {                                                          \
    pthread_mutex_lock(&(_NAME(_lock_handle_, NAME)));       \
    code                                                     \
        pthread_mutex_unlock(&(_NAME(_lock_handle_, NAME))); \
  }
#endif
#else
#define INIT_LOCK(NAME) \
  {}
#define LOCK(NAME, code) \
  { code }
#endif

#ifdef __cplusplus
}
#endif

#endif
