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
 * simple string buffer used to dynamicly add content.
 * */

#ifndef __STR_BUILDER_H__
#define __STR_BUILDER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bytes.h"
#include "data.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

/** shortcut macro for adding a uint to the stringbuilder using sizeof(i) to automaticly determine the size*/
#define sb_add_hexuint(sb, i) sb_add_hexuint_l(sb, i, sizeof(i))

#ifdef __ZEPHYR__
typedef unsigned long long uintmax_t;
#endif

/**
 * string build struct, which is able to hold and modify a growing string.
 */
typedef struct sb {
  char*  data;     /**< the current string (null terminated)*/
  size_t allocted; /**< number of bytes currently allocated */
  size_t len;      /**< the current length of the string */
} sb_t;
/**
 * creates a stringbuilder which is allocating any new memory, but uses an existing string and is used directly on the stack.
 * Since it will not grow the memory you need to pass a char* which allocated enough memory.
 */
NONULL static inline sb_t sb_stack(char* p) { return (sb_t){.allocted = 0xffffff, .len = 0, .data = p}; }

sb_t*        sb_new(const char* chars); /**< creates a new stringbuilder and copies the inital characters into it.*/
NONULL sb_t* sb_init(sb_t* sb);         /**< initializes a stringbuilder by allocating memory. */
NONULL void  sb_free(sb_t* sb);         /**< frees all resources of the stringbuilder */

NONULL sb_t* sb_add_char(sb_t* sb, char c);                                                                 /**< add a single character */
NONULL sb_t* sb_add_chars(sb_t* sb, const char* chars);                                                     /**< adds a string */
NONULL sb_t* sb_add_range(sb_t* sb, const char* chars, int start, int len);                                 /**< add a string range */
NONULL sb_t* sb_add_key_value(sb_t* sb, const char* key, const char* value, int value_len, bool as_string); /**< adds a value with an optional key. if as_string is true the value will be quoted. */
NONULL_FOR((1, 3))
sb_t*        sb_add_bytes(sb_t* sb, const char* prefix, const bytes_t* bytes, int len, bool as_array); /**< add bytes as 0x-prefixed hexcoded string (including an optional prefix), if len>1 is passed bytes maybe an array ( if as_array==true)  */
NONULL sb_t* sb_add_hexuint_l(sb_t* sb, uintmax_t uint, size_t l);                                     /**< add a integer value as hexcoded, 0x-prefixed string*/
NONULL sb_t* sb_add_escaped_chars(sb_t* sb, const char* chars, int l);                                 /**< add chars but escapes all quotes, if l==-1 the length will be uaws from strlen */
NONULL sb_t* sb_add_int(sb_t* sb, int64_t val);                                                        /**< adds a numeric value to the stringbuilder */
NONULL char* format_json(const char* json);                                                            /**< format a json string and returns a new string, which needs to be freed */
NONULL_FOR((1))
sb_t* sb_add_rawbytes(sb_t* sb, char* prefix, bytes_t b, int fix_size);
sb_t* sb_print(sb_t* sb, const char* fmt, ...);
sb_t* sb_vprint(sb_t* sb, const char* fmt, va_list args);
sb_t* sb_add_json(sb_t* sb, const char* prefix, d_token_t* token);
/**
 * adds the arguments as defined in the formt-string.
 * it works similiar to sprintf
 *
 * The format supports:
 *
 * %s - expects a char* and inserts the string
 * %S - expects a char* and inserts a escaped string (replacing quotes and newlines to be included in json)
 * %i - expects int32_t and inserts a decimal representation
 * %d - expects int32_t and inserts a decimal representation
 * %u - expects uint32_t and inserts a decimal representation
 * %I - expects int64_t and inserts a decimal representation
 * %D - expects int64_t and inserts a decimal representation
 * %U - expects uint64_t and inserts a decimal representation
 * %x - expects uint64_t and inserts a hex representation with a 0x-prefix
 * %b - expects a bytes_t and inserts the data as hex without 0x-prefix
 * %B - expects a bytes_t and inserts the data as hex with a 0x-prefix
 * %v - expects a bytes_t and inserts the data as hex without 0x-prefix after removing all leading zeros
 * %V - expects a bytes_t and inserts the data as hex with a 0x-prefix  removing all leading zeros
 * %j - expects a d_token_t* and inserts the json-representation
 * %w - expects a bytes_t and inserts the decimal representation of bytes  (max 32 bytes)
 * %W- expects a dec_t and inserts the decimal representation of those bytes including the decimal (max 32 bytes)
 *
 */
sb_t* sb_printx(sb_t* sb, const char* fmt, ...);

/**
 * creates a new string by allocating memory and formating the arguments based on the formast string.
 * if works similiar to sprintf, but allocates the memory and returns the pointer to the new string, which must be freed after usage.
 *
 * The format supports:
 *
 * %s - expects a char* and inserts the string
 * %S - expects a char* and inserts a escaped string (replacing quotes and newlines to be included in json)
 * %i - expects int32_t and inserts a decimal representation
 * %d - expects int32_t and inserts a decimal representation
 * %u - expects uint32_t and inserts a decimal representation
 * %I - expects int64_t and inserts a decimal representation
 * %D - expects int64_t and inserts a decimal representation
 * %U - expects uint64_t and inserts a decimal representation
 * %x - expects uint64_t and inserts a hex representation with a 0x-prefix
 * %b - expects a bytes_t and inserts the data as hex without 0x-prefix
 * %B - expects a bytes_t and inserts the data as hex with a 0x-prefix
 * %v - expects a bytes_t and inserts the data as hex without 0x-prefix after removing all leading zeros
 * %V - expects a bytes_t and inserts the data as hex with a 0x-prefix  removing all leading zeros
 * %j - expects a d_token_t* and inserts the json-representation
 * %w - expects a bytes_t and inserts the decimal representation of bytes as bugendian (max 32 bytes)
 * %W- expects a dec_t and inserts the decimal representation of those bytes including the decimal (max 32 bytes)
 *
 */
char* sprintx(const char* fmt, ...);

/**
 * prints a string based a var args.
 */
void sb_vprintx(sb_t* sb, const char* fmt, va_list args);
#ifdef __cplusplus
}
#endif
#endif
