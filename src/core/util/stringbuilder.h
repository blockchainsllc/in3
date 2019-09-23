/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 *
 * Copyright (C) 2019 Blockchains, LLC
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
 * 
 * If you cannot meet the requirements of AGPL, 
 * you should contact us to inquire about a commercial license.
 *******************************************************************************/

// @PUBLIC_HEADER
/** @file
 * simple string buffer used to dynamicly add content.
 * */

#ifndef __STR_BUILDER_H__
#define __STR_BUILDER_H__

#include "bytes.h"
#include <stdbool.h>
#include <stddef.h>

#define sb_add_hexuint(sb, i) sb_add_hexuint_l(sb, i, sizeof(i))

#ifdef __ZEPHYR__
typedef unsigned long long uintmax_t;
#endif

typedef struct sb {
  char*  data;
  size_t allocted;
  size_t len;
} sb_t;

sb_t* sb_new(char* chars);
sb_t* sb_init(sb_t* sb);
void  sb_free(sb_t* sb);

sb_t* sb_add_char(sb_t* sb, char c);
sb_t* sb_add_chars(sb_t* sb, char* chars);
sb_t* sb_add_range(sb_t* sb, const char* chars, int start, int len);
sb_t* sb_add_key_value(sb_t* sb, char* key, char* value, int lv, bool as_string);
sb_t* sb_add_bytes(sb_t* sb, char* prefix, bytes_t* bytes, int len, bool as_array);
sb_t* sb_add_hexuint_l(sb_t* sb, uintmax_t uint, size_t l);

#endif
