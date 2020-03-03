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

#include "stringbuilder.h"
#include "../util/bytes.h"
#include "../util/utils.h"
#include "debug.h"
#include "mem.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static const size_t MIN_SIZE = 32;

sb_t* sb_new(const char* chars) {
  sb_t* sb     = _malloc(sizeof(sb_t));
  sb->data     = _malloc(MIN_SIZE);
  sb->allocted = MIN_SIZE;
  sb->data[0]  = 0;
  sb->len      = 0;
  if (chars != NULL) sb_add_chars(sb, chars);
  return sb;
}
sb_t* sb_init(sb_t* sb) {
  sb->data     = _malloc(MIN_SIZE);
  sb->allocted = MIN_SIZE;
  sb->data[0]  = 0;
  sb->len      = 0;
  return sb;
}
static void check_size(sb_t* sb, size_t len) {
  if (sb == NULL || len == 0 || sb->len + len < sb->allocted) return;
#ifdef __ZEPHYR__
  size_t l = sb->allocted;
#endif
  while (sb->len + len >= sb->allocted) sb->allocted <<= 1;
#ifdef __ZEPHYR__
  sb->data = _realloc(sb->data, sb->allocted, l);
#else
  sb->data = _realloc(sb->data, sb->allocted, 0);
#endif
}

sb_t* sb_add_chars(sb_t* sb, const char* chars) {
  int l = strlen(chars);
  if (l == 0 || chars == NULL) return sb;
  check_size(sb, l);
  memcpy(sb->data + sb->len, chars, l);
  sb->len += l;
  sb->data[sb->len] = 0;
  return sb;
}
sb_t* sb_add_char(sb_t* sb, char c) {
  check_size(sb, 1);
  sb->data[sb->len++] = c;
  sb->data[sb->len]   = 0;
  return sb;
}

sb_t* sb_add_range(sb_t* sb, const char* chars, int start, int len) {
  if (chars == NULL) return sb;
  check_size(sb, len);
  memcpy(sb->data + sb->len, chars + start, len);
  sb->len += len;
  sb->data[sb->len] = 0;
  return sb;
}
sb_t* sb_add_key_value(sb_t* sb, const char* key, const char* value, int lv, bool as_string) {
  if (lv == 0) return sb;
  int p = sb->len, lk = strlen(key);
  check_size(sb, (as_string ? 2 : 0) + lk + 3 + lv);
  sb->data[p++] = '"';
  memcpy(sb->data + p, key, lk);
  p += lk;
  sb->data[p++] = '"';
  sb->data[p++] = ':';
  if (as_string) sb->data[p++] = '"';
  memcpy(sb->data + p, value, lv);
  p += lv;
  if (as_string) sb->data[p++] = '"';
  sb->len           = p;
  sb->data[sb->len] = 0;
  return sb;
}

sb_t* sb_add_bytes(sb_t* sb, const char* prefix, const bytes_t* bytes, int len, bool as_array) {
  int p = sb->len, lk = prefix == NULL ? 0 : strlen(prefix), s = 0, i;
  for (i = 0; i < len; i++) s += bytes[i].len * 2 + 4 + (i > 0 ? 1 : 0);
  check_size(sb, s + lk + (as_array ? 2 : 0));
  if (prefix != NULL) memcpy(sb->data + p, prefix, lk);
  p += lk;

  if (as_array) sb->data[p++] = '[';
  for (i = 0; i < len; i++) {
    if (i > 0) sb->data[p++] = ',';
    sb->data[p++] = '"';
    sb->data[p++] = '0';
    sb->data[p++] = 'x';
    bytes_to_hex(bytes[i].data, bytes[i].len, sb->data + p);
    p += bytes[i].len * 2;
    sb->data[p++] = '"';
  }
  if (as_array) sb->data[p++] = ']';
  sb->data[p] = 0;
  sb->len     = p;
  return sb;
}

sb_t* sb_add_hexuint_l(sb_t* sb, uintmax_t uint, size_t l) {
  char tmp[19]; // UINT64_MAX => 18446744073709551615 => 0xFFFFFFFFFFFFFFFF
  switch (l) {
    case 1: l = sprintf(tmp, "0x%" PRIx8, (uint8_t) uint); break;
    case 2: l = sprintf(tmp, "0x%" PRIx16, (uint16_t) uint); break;
    case 4: l = sprintf(tmp, "0x%" PRIx32, (uint32_t) uint); break;
    case 8: l = sprintf(tmp, "0x%" PRIx64, (uint64_t) uint); break;
    default: return sb; /** Other types not supported */
  }
  check_size(sb, l);
  memcpy(sb->data + sb->len, tmp, l);
  sb->len += l;
  sb->data[sb->len] = 0;
  return sb;
}

void sb_free(sb_t* sb) {
  if (sb == NULL) return;
  if (sb->data != NULL) _free(sb->data);
  _free(sb);
}