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

// @PUBLIC_HEADER
/** @file
 * utility functions.
 * */

#ifndef UTILS_H
#define UTILS_H

#include "bytes.h"
#include <stdint.h>

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
#define IS_APPROX(n1, n2, err) ((n1 > n2) ? ((n1 - n2) <= err) : ((n2 - n1) <= err))

/** converts the bytes to a unsigned long (at least the last max len bytes) */
uint64_t bytes_to_long(const uint8_t* data, int len);

/** converts the bytes to a unsigned int (at least the last max len bytes) */
static inline uint32_t bytes_to_int(const uint8_t* data, int len) {
  switch (len) {
    case 0: return 0;
    case 1: return data[0];
    case 2: return (((uint32_t) data[0]) << 8) | data[1];
    case 3: return (((uint32_t) data[0]) << 16) | (((uint32_t) data[1]) << 8) | data[2];
    default: return (((uint32_t) data[0]) << 24) | (((uint32_t) data[1]) << 16) | (((uint32_t) data[2]) << 8) | data[3];
  }
}
/** converts a character into a uint64_t*/
uint64_t char_to_long(const char* a, int l);

/**  converts a hexchar to byte (4bit) */
uint8_t hexchar_to_int(char c);

/** converts a uint64_t to string (char*); buffer-size min. 21 bytes */
const unsigned char* u64_to_str(uint64_t value, char* pBuf, int szBuf);

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

/** hashes the bytes and creates a new bytes_t */
bytes_t* sha3(const bytes_t* data);

/** writes 32 bytes to the pointer. */
int sha3_to(bytes_t* data, void* dst);

/** converts a long to 8 bytes */
void long_to_bytes(uint64_t val, uint8_t* dst);

/** converts a int to 4 bytes */
void int_to_bytes(uint32_t val, uint8_t* dst);

/** duplicate the string */
char* _strdupn(const char* src, int len);

/** calculate the min number of byte to represents the len */
int min_bytes_len(uint64_t val);

/**
 * sets a variable value to 32byte word.
 */
void uint256_set(const uint8_t* src, wlen_t src_len, bytes32_t dst);

/**
 * replaces a string and returns a copy.
 * @retval 
 */
char* str_replace(const char* orig, const char* rep, const char* with);

/**
 * replaces a string at the given position.
 */
char* str_replace_pos(const char* orig, size_t pos, size_t len, const char* rep);

/**
  * lightweight strstr() replacements
  */
char* str_find(const char* haystack, const char* needle);

/** changes to pointer (a) and it length (l) to remove leading 0 bytes.*/
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

static inline bool memiszero(uint8_t* ptr, size_t l) {
  while (l > 0 && *ptr == 0) {
    l--;
    ptr++;
  }
  return !l;
}

#endif
