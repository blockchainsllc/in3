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

#include "stringbuilder.h"
#include "../../third-party/crypto/bignum.h"
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
NONULL static void check_size(sb_t* sb, size_t len) {
  if ((len == 0 || sb->len + len < sb->allocted) && sb->data) return;
  if (sb->allocted == 0) {
    sb->allocted = len + 1,
    sb->data     = _malloc(sb->allocted);
    return;
  }
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

sb_t* sb_add_escaped_chars(sb_t* sb, const char* chars, int l) {
  if (l == -1) l = strlen(chars);
  int escapes = 0;
  if (l == 0 || chars == NULL) return sb;
  for (int i = 0; i < l; i++) {
    if (chars[i] == '"' || chars[i] == '\n' || chars[i] == '\\') escapes++;
  }
  check_size(sb, l + escapes);
  memcpy(sb->data + sb->len, chars, l);
  if (escapes) {
    escapes = 0;
    for (int i = 0; i < l; i++) {
      if (chars[i] == '"' || chars[i] == '\\') {
        sb->data[sb->len + i + escapes] = '\\';
        memcpy(sb->data + sb->len + i + escapes + 1, chars + i, l - i);
        escapes++;
      }
      if (chars[i] == '\n') {
        memcpy(sb->data + sb->len + i + escapes + 1, chars + i, l - i);
        sb->data[sb->len + i + escapes]     = '\\';
        sb->data[sb->len + i + escapes + 1] = 'n';
        escapes++;
      }
    }
  }
  sb->len += l + escapes;
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

sb_t* sb_add_int(sb_t* sb, int64_t val) {
  char tmp[30]; // UINT64_MAX => 18446744073709551615 => 0xFFFFFFFFFFFFFFFF
  int  l = sprintf(tmp, "%" PRIi64, val);
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

char* format_json(const char* json) {
  sb_t  _sb = {0}, level = {0};
  sb_t* sb       = &_sb;
  bool  in_quote = false;
  sb_add_char(&level, '\n');
  for (char c = *json; c; c = *(++json)) {
    if (in_quote && c != '\\' && c != '"') {
      sb_add_char(sb, c);
      continue;
    }
    switch (c) {
      case '"':
        in_quote = !in_quote;
        sb_add_char(sb, c);
        break;
      case '\\':
        sb_add_char(sb, c);
        if (!(c = *(++json)))
          --json;
        else
          sb_add_char(sb, c);
        break;
      case '{':
        sb_add_char(sb, c);
        sb_add_chars(&level, "  ");
        sb_add_range(sb, level.data, 0, level.len);
        break;
      case '}':
        if (level.len > 2) level.len -= 2;
        sb_add_range(sb, level.data, 0, level.len);
        sb_add_char(sb, c);
        break;
      case ',':
        sb_add_char(sb, c);
        sb_add_range(sb, level.data, 0, level.len);
        break;
      case '\n':
      case ' ':
        break;
      case ':':
        sb_add_chars(sb, ": ");
        break;
      default:
        sb_add_char(sb, c);
    }
  }
  _free(level.data);
  return _sb.data;
}

static const uint8_t zero = 0;

sb_t* sb_add_rawbytes(sb_t* sb, char* prefix, bytes_t b, int fix_size) {
  if (fix_size == -1) {
    if (b.len == 0) b = bytes((uint8_t*) &zero, 1);
    b_optimize_len(&b);
  }
  int l  = prefix ? strlen(prefix) : 0;
  int bl = b.len * 2;
  if (fix_size > (int) b.len) bl = fix_size * 2;
  if (fix_size == -1 && b.len && *b.data < 16) bl--;
  l += bl;
  if (l == 0) return sb;
  check_size(sb, l);
  if (prefix) memcpy(sb->data + sb->len, prefix, l - bl);
  sb->len += l;
  sb->data[sb->len] = 0;
  int p             = sb->len - bl;
  for (int i = (int) b.len; i < fix_size; i++, p += 2)
    sb->data[p] = sb->data[p + 1] = '0';
  if (fix_size == -1 && b.len && *b.data < 16) {
    char tmp[3];
    bytes_to_hex(b.data, 1, tmp);
    sb->data[p] = tmp[1];
    bytes_to_hex(b.data + 1, b.len - 1, sb->data + p + 1);
  }
  else
    bytes_to_hex(b.data, b.len, sb->data + p);
  return sb;
}

sb_t* sb_vprint(sb_t* sb, const char* fmt, va_list args) {
  int n = sb->allocted - sb->len - 1;
  if (n < (int) strlen(fmt)) {
    check_size(sb, strlen(fmt) + 30);
    n = sb->allocted - sb->len - 1;
  }

  va_list cpy;
  va_copy(cpy, args);
  int w = vsnprintf(sb->data + sb->len, n + 1, fmt, args);
  if (w > n) {
    check_size(sb, w + 1);
    vsprintf(sb->data + sb->len, fmt, cpy);
    va_end(cpy);
  }
  sb->len += w;
  return sb;
}

sb_t* sb_print(sb_t* sb, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  sb_vprint(sb, fmt, args);
  va_end(args);
  return sb;
}

sb_t* sb_add_json(sb_t* sb, const char* prefix, d_token_t* token) {
  if (!token) return sb;
  if (prefix && *prefix) sb_add_chars(sb, prefix);
  switch (d_type(token)) {
    case T_ARRAY:
    case T_OBJECT: {
      str_range_t r = d_to_json(token);
      if (r.data) return sb_add_range(sb, r.data, 0, r.len);
      const char* brackets = d_type(token) == T_ARRAY ? "[]" : "{}";
      sb_add_char(sb, brackets[0]);
      for (d_iterator_t iter = d_iter(token); iter.left; d_iter_next(&iter))
        sb_add_json(sb, iter.token != token + 1 ? "," : "", iter.token);
      return sb_add_char(sb, brackets[1]);
    }
    case T_BOOLEAN:
      return sb_add_chars(sb, d_int(token) ? "true" : "false");
    case T_INTEGER:
      return sb_add_int(sb, d_int(token));
    case T_BYTES: {
      bytes_t b = d_to_bytes(token);
      sb_add_rawbytes(sb, "\"0x", b, b.len < 20 ? -1 : 0);
      return sb_add_char(sb, '"');
    }
    case T_STRING: {
      sb_add_char(sb, '\"');
      sb_add_escaped_chars(sb, (char*) token->data, d_len(token));
      return sb_add_char(sb, '\"');
    }
    case T_NULL:
      return sb_add_chars(sb, "null");
  }
  return sb;
}

void sb_vprintx(sb_t* sb, const char* fmt, va_list args) {
  check_size(sb, strlen(fmt));
  for (const char* c = fmt; *c; c++) {
    if (*c == '%') {
      c++;
      switch (*c) {
        case 's':
          sb_add_chars(sb, va_arg(args, char*));
          break;
        case 'S':
          sb_add_escaped_chars(sb, va_arg(args, char*), -1);
          break;
        case 'i':
        case 'd':
          sb_add_int(sb, (int64_t) va_arg(args, int32_t));
          break;
        case 'u':
          sb_add_int(sb, (int64_t) va_arg(args, uint32_t));
          break;
        case 'I':
        case 'D':
        case 'U':
          sb_add_int(sb, va_arg(args, int64_t));
          break;
        case 'w': {
          bytes_t wei = va_arg(args, bytes_t);
          if (wei.len > 32) {
            sb_add_char(sb, 'X');
            break;
          }
          char*     tmp = _malloc(wei.len * 3 + 1);
          bytes32_t val = {0};
          memcpy(val + 32 - wei.len, wei.data, wei.len);
          bignum256 bn;
          bn_read_be(val, &bn);
          size_t l = bn_format(&bn, "", "", 0, 0, false, tmp, wei.len * 3);
          sb_add_range(sb, tmp, 0, l);
          _free(tmp);
          break;
        }
        case 'x':
          sb_add_hexuint_l(sb, va_arg(args, uint64_t), sizeof(uint64_t));
          break;
        case 'b':
          sb_add_rawbytes(sb, "", va_arg(args, bytes_t), 0);
          break;
        case 'B':
          sb_add_rawbytes(sb, "0x", va_arg(args, bytes_t), 0);
          break;
        case 'v':
          sb_add_rawbytes(sb, "", va_arg(args, bytes_t), -1);
          break;
        case 'V':
          sb_add_rawbytes(sb, "0x", va_arg(args, bytes_t), -1);
          break;
        case 'j':
          sb_add_json(sb, "", va_arg(args, d_token_t*));
          break;
        case 'J': {
          sb_t tmp = {0};
          sb_add_json(&tmp, "", va_arg(args, d_token_t*));
          if (tmp.data) {
            char* t2 = format_json(tmp.data);
            sb_add_chars(sb, t2);
            _free(t2);
            _free(tmp.data);
          }
          break;
        }
        case 0:
          return;
        default:
          break;
      }
      continue;
    }
    if (sb->len + 1 >= sb->allocted) check_size(sb, 1);
    sb->data[sb->len++] = *c;
  }
  sb->data[sb->len] = 0;
}
sb_t* sb_printx(sb_t* sb, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  sb_vprintx(sb, fmt, args);
  va_end(args);
  return sb;
}

char* sprintx(const char* fmt, ...) {
  sb_t    s = {0};
  va_list args;
  va_start(args, fmt);
  sb_vprintx(&s, fmt, args);
  va_end(args);
  return s.data;
}
