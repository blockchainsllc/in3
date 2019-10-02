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

#define SWAP(a, b) \
  {                \
    void* p = a;   \
    a       = b;   \
    b       = p;   \
  }

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef uint32_t      pb_size_t;
typedef uint_least8_t pb_byte_t;

/** converts the bytes to a unsigned long (at least the last max len bytes) */
uint64_t bytes_to_long(uint8_t* data, int len);

/** converts the bytes to a unsigned int (at least the last max len bytes) */
static inline uint32_t bytes_to_int(uint8_t* data, int len) {
  switch (len) {
    case 0: return 0;
    case 1: return data[0];
    case 2: return (((uint32_t) data[0]) << 8) | data[1];
    case 3: return (((uint32_t) data[0]) << 16) | (((uint32_t) data[1]) << 8) | data[2];
    default: return (((uint32_t) data[0]) << 24) | (((uint32_t) data[1]) << 16) | (((uint32_t) data[2]) << 8) | data[3];
  }
}
/** converts a character into a uint64_t*/
uint64_t c_to_long(char* a, int l);

/** the number of bytes used for a conerting a hex into bytes. */
int size_of_bytes(int str_len);

/**  converts a hexchar to byte (4bit) */
uint8_t strtohex(char c);

/** converts a uint64_t to string (char*); buffer-size min. 21 bytes */
extern const unsigned char* u64tostr(uint64_t value, char* pBuf, int szBuf);

/** convert a c string to a byte array storing it into a existing buffer */
int hex2byte_arr(char* buf, int len, uint8_t* out, int outbuf_size);

/** convert hex to long */
uint64_t hex2long(char* buf);

/** convert a c string to a byte array creating a new buffer */
bytes_t* hex2byte_new_bytes(char* buf, int len);

/** convefrts a bytes into hex */
int bytes_to_hex(uint8_t* buffer, int len, char* out);

/** hashes the bytes and creates a new bytes_t */
bytes_t* sha3(bytes_t* data);

/** writes 32 bytes to the pointer. */
int sha3_to(bytes_t* data, void* dst);

/** converts a long to 8 bytes */
void long_to_bytes(uint64_t val, uint8_t* dst);
/** converts a int to 4 bytes */
void int_to_bytes(uint32_t val, uint8_t* dst);
/** compares 32 bytes and returns 0 if equal*/
int hash_cmp(uint8_t* a, uint8_t* b);

/** duplicate the string */
char* _strdupn(char* src, int len);

/** calculate the min number of byte to represents the len */
int min_bytes_len(uint64_t val);

/**
 * sets a variable value to 32byte word.
 */
void uint256_set(uint8_t* src, wlen_t src_len, uint8_t dst[32]);

char* str_replace(char* orig, char* rep, char* with);

char* str_replace_pos(char* orig, size_t pos, size_t len, const char* rep);

// lightweight strstr() replacement
char* str_find(char* haystack, const char* needle);

#define optimize_len(a, l)   \
  while (l > 1 && *a == 0) { \
    l--;                     \
    a++;                     \
  }

#define TRY(exp)           \
  {                        \
    int _r = (exp);        \
    if (_r < 0) return _r; \
  }
#define TRY_SET(var, exp)    \
  {                          \
    var = (exp);             \
    if (var < 0) return var; \
  }

#define TRY_GOTO(exp)        \
  {                          \
    res = (exp);             \
    if (res < 0) goto clean; \
  }

#endif
