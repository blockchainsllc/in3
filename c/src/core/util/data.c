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
#define IN3_INTERNAL
#include "data.h"
#include "bytes.h"
#include "crypto.h"
#include "debug.h"
#include "mem.h"
#include "stringbuilder.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef LOGGING
#include "used_keys.h"
#endif
// Here we check the pointer-size, because pointers smaller than 32bit may result in a undefined behavior, when calling d_bytes() for a T_INTEGER
// verify(sizeof(void*) >= 4);

// number of tokens to allocate memory for when parsing
#define JSON_INIT_TOKENS        10
#define JSON_INDEXD_PAGE        128
#define JSON_MAX_ALLOWED_TOKENS 1000000
#define JSON_E_MAX_DEPTH        -3
#define JSON_E_INVALID_CHAR     -2
#define JSON_E_NUMBER_TOO_LONG  -4
#define JSON_E_END_OF_STRING    -1

d_key_t keyn(const char* c, const size_t len) {
  d_key_t val = 0;
  size_t  i   = 0;
  for (; i < len; i++) {
    if (*c == 0) return val;
    val ^= *c | val << 7;
    c += 1;
  }
  return val;
}

static d_key_t get_key(json_ctx_t* ctx, const char* name, size_t len) {
  if (!ctx->keys) return keyn(name, len);
  if (len == 0) len = strlen(name);
  for (unsigned int p = 0; p < ctx->keys_last;) {
    size_t l = ctx->keys[p];
    if (l == len + 2 && strncmp(name, (char*) ctx->keys + p + 1, len) == 0) return p + 1;
    p += l;
  }
  return 0;
}

d_key_t ikey(json_ctx_t* ctx, const char* name) {
  return get_key(ctx, name, 0);
}

static char* get_key_str(json_ctx_t* ctx, d_key_t k) {
  return !ctx || !ctx->keys || !k || k > ctx->keys_last ? NULL : ((char*) ctx->keys + (int) k);
}

d_key_t d_add_key(json_ctx_t* ctx, const char* name, size_t len) {
  if (!ctx->keys) return keyn(name, len);
  d_key_t found = get_key(ctx, name, len);
  if (found) return found;
  int k        = ctx->keys_last;
  int old_page = k / JSON_INDEXD_PAGE;
  int new_page = (k + len + 2) / JSON_INDEXD_PAGE;
  if (new_page != old_page) ctx->keys = _realloc(ctx->keys, (new_page + 1) * JSON_INDEXD_PAGE, (old_page + 1) * JSON_INDEXD_PAGE);
  uint8_t* p            = ctx->keys + k;
  p[len + 1]            = 0;
  ctx->keys_last += * p = len + 2;
  memcpy(p + 1, name, len);
  return (d_key_t) k + 1;
}

size_t d_token_size(const d_token_t* item) {
  if (item == NULL) return 0;
  size_t i, c = 1;
  switch (d_type(item)) {
    case T_ARRAY:
    case T_OBJECT:
      for (i = 0; i < (item->len & 0xFFFFFFF); i++)
        c += d_token_size(item + c);
      return c;
    default:
      return 1;
  }
}

bytes_t* d_as_bytes(d_token_t* item) {
  if (d_type(item) != T_BYTES && d_is_bytes(item)) d_bytes(item);
  return d_type(item) == T_BYTES ? (void*) item : NULL;
}

bytes_t d_bytesl(d_token_t* item, uint32_t len) {
  bytes_t b = d_bytes(item);
  if (!b.data) return b;
  if (b.len < len && len > 4 && d_type(item) == T_BYTES) {
    bytes_t t = bytes(_calloc(1, len), len);
    memcpy(t.data + len - b.len, b.data, b.len);
    if (item->state & TOKEN_STATE_ALLOCATED) _free(item->data);
    item->data  = t.data;
    item->len   = len;
    item->state = TOKEN_STATE_ALLOCATED | TOKEN_STATE_CONVERTED;
    return t;
  }
  if (b.len > len) b.len = len;
  return b;
}
bytes_t d_bytes_enc(d_token_t* item, in3_encoding_type_t enc) {
  if (enc && d_type(item) == T_STRING) {
    uint8_t* dst = _malloc(decode_size(enc, d_len(item)));
    int      l   = decode(enc, (char*) (void*) item->data, d_len(item), dst);
    if (l >= 0) {
      if (item->state & TOKEN_STATE_ALLOCATED) _free(item->data);
      item->len   = l;
      item->data  = dst;
      item->state = TOKEN_STATE_ALLOCATED | TOKEN_STATE_CONVERTED;
      return bytes(item->data, l);
    }
    else {
      _free(dst);
      return NULL_BYTES;
    }
  }
  return d_bytes(item);
}

bytes_t d_bytes(d_token_t* item) {
  switch (d_type(item)) {
    case T_BYTES:
      return bytes(item->data, item->len);
    case T_STRING: {
      int   l     = d_len(item);
      char* start = (char*) item->data;
      if (item->state & TOKEN_STATE_CONVERTED) return bytes(item->data, l);

      // is it a hex-string?
      bool ishex = l > 1 && *start == '0' && start[1] == 'x';
      if (ishex)
        for (int n = 2; n < l; n++) {
          char cc = start[n];
          if (!((cc >= '0' && cc <= '9') || (cc >= 'a' && cc <= 'f') || (cc >= 'A' && cc <= 'F'))) {
            ishex = false;
            break;
          }
        }
      if (!ishex) {
        item->state |= TOKEN_STATE_CONVERTED;
        return bytes(item->data, l);
      }

      // we have a hex string, we need to convert.
      //  we can store it as number ?
      if (l == 2) { //  empty bytes, so we don't allocate anything
        if (item->state & TOKEN_STATE_ALLOCATED) _free(item->data);
        item->data  = NULL;
        item->len   = 0;
        item->state = TOKEN_STATE_CONVERTED;
        return bytes((uint8_t*) &item->data, 0);
      }
      else if (l < 10) { //  integer-type, no allocation
        if (item->state & TOKEN_STATE_ALLOCATED) _free(item->data);
        item->data  = NULL;
        int tl      = hex_to_bytes(start, l, (uint8_t*) &item->data, 4);
        item->len   = T_INTEGER << 28 | bytes_to_int((uint8_t*) &item->data, tl);
        item->state = TOKEN_STATE_CONVERTED;
        return bytes((uint8_t*) &item->data, tl);
      }
      else {
        item->len   = (l - 1) / 2;
        item->data  = item->state & TOKEN_STATE_ALLOCATED ? item->data : _malloc(item->len);
        item->len   = hex_to_bytes(start, l, item->data, item->len);
        item->state = TOKEN_STATE_ALLOCATED | TOKEN_STATE_CONVERTED;
        return bytes(item->data, item->len);
      }
    }
    case T_INTEGER:
    case T_BOOLEAN: {
      uint8_t  tmp[4];
      uint32_t val = item->len & 0xFFFFFFF;
      int      l   = val & 0xFF000000 ? 4 : (val & 0xFF0000 ? 3 : (val & 0xFF00 ? 2 : 1));
      int_to_bytes(val, tmp);
      memcpy(&item->data, tmp + 4 - l, l);
      return bytes((uint8_t*) &item->data, l);
    }
    case T_NULL:
    default:
      return NULL_BYTES;
  }
}

char* d_string(d_token_t* item) {
  if (item == NULL) return NULL;
  switch (d_type(item)) {
    case T_STRING: {
      if ((item->state & TOKEN_STATE_ALLOCATED) == 0 && item->data) {
        char* src = (char*) item->data;
        int   l   = d_len(item);
        // is the string already null-terminated?
        if (src[l] == 0) return src;
        item->data    = _malloc(l + 1);
        item->data[l] = 0;
        item->state |= TOKEN_STATE_ALLOCATED;
        memcpy(item->data, src, l);
      }
      return (char*) item->data;
    }
    case T_BYTES: {
      char* p = bytes_to_hex_string(_malloc(item->len * 2 + 3), "0x", bytes(item->data, item->len), NULL);
      if (item->state & TOKEN_STATE_ALLOCATED && item->data) _free(item->data);
      item->data  = (uint8_t*) p;
      item->len   = (T_STRING << 28) | (item->len * 2 + 2);
      item->state = TOKEN_STATE_ALLOCATED;
      return (char*) item->data;
    }

    case T_BOOLEAN: {
      item->data  = (uint8_t*) _strdupn(d_len(item) ? "true" : "false", -1);
      item->len   = (T_STRING << 28) | strlen((char*) item->data);
      item->state = TOKEN_STATE_ALLOCATED;
      return (char*) item->data;
    }
    case T_NULL: return NULL;
    case T_INTEGER: {
      item->data  = (void*) sprintx("%d", (uint32_t) d_len(item));
      item->len   = (T_STRING << 28) | strnlen((char*) item->data, 20);
      item->state = TOKEN_STATE_ALLOCATED;
      return (char*) item->data;
    }
    case T_ARRAY:
    case T_OBJECT:
      return (char*) item->data;

    default:
      return NULL;
  }
}

int32_t d_int(d_token_t* item) {
  return d_intd(item, 0);
}

int32_t d_intd(d_token_t* item, const uint32_t def_val) {
  if (item == NULL) return def_val;
  switch (d_type(item)) {
    case T_INTEGER:
    case T_BOOLEAN:
      return item->len & 0xFFFFFFF;
    case T_BYTES: {
      if (item->len == 0) return 0;
      return bytes_to_int(item->data, min(4, item->len));
    }
    case T_STRING:
      if (d_is_bytes(item)) {
        bytes_t b = d_bytes(item);
        return bytes_to_int(b.data, min(4, b.len));
      }
      return atoi((char*) item->data);
    default:
      return def_val;
  }
}

bytes_t** d_create_bytes_vec(d_token_t* arr) {
  if (arr == NULL) return NULL;
  int        l   = d_len(arr), i;
  bytes_t**  dst = _calloc(l + 1, sizeof(bytes_t*));
  d_token_t* t   = arr + 1;
  for (i = 0; i < l; i++, t += d_token_size(t)) {
    d_bytes(t);
    dst[i] = (bytes_t*) (void*) (t);
  }
  return dst;
}

uint64_t d_long(d_token_t* item) {
  return d_longd(item, 0L);
}
uint64_t d_longd(d_token_t* item, const uint64_t def_val) {
  if (item == NULL) return def_val;
  if (d_type(item) == T_INTEGER)
    return item->len & 0x0FFFFFFF;
  else if (d_is_bytes(item)) {
    bytes_t b = d_bytes(item);
    return bytes_to_long(b.data, b.len);
  }
  else if (d_type(item) == T_STRING)
    return _strtoull((char*) item->data, NULL, 10);
  return def_val;
}

bool d_eq(d_token_t* a, d_token_t* b) {
  if (a == NULL || b == NULL) return false;
  if (d_is_bytes(a) && d_type(a) == T_STRING) d_bytes(a);
  if (d_is_bytes(b) && d_type(b) == T_STRING) d_bytes(b);
  if (d_type(a) == T_BYTES && d_type(b) == T_INTEGER && d_len(a) < 5 && d_int(a) == d_int(b)) return true;
  if (d_type(b) == T_BYTES && d_type(a) == T_INTEGER && d_len(b) < 5 && d_int(a) == d_int(b)) return true;
  if (a->len != b->len) return false;
  if (d_type(a) <= T_STRING && a->len == 0) return true;
  if (d_type(a) == T_ARRAY) {
    for (d_iterator_t ia = d_iter((d_token_t*) a), ib = d_iter((d_token_t*) b); ia.left; d_iter_next(&ia), d_iter_next(&ib)) {
      if (!d_eq(ia.token, ib.token)) return false;
    }
    return true;
  }
  if (d_type(a) == T_OBJECT) {
    for_children_of(ia, a) {
      if (!d_eq(ia.token, d_get((d_token_t*) b, ia.token->key))) return false;
    }
    return true;
  }
  if (d_type(a) == T_STRING) return strncmp((char*) a->data, (char*) b->data, d_len(b)) == 0;

  return (a->data && b->data && d_type(a) == T_BYTES)
             ? bytes_cmp(d_bytes(a), d_bytes(b))
             : d_type(a) > T_OBJECT && d_type(b) > T_OBJECT;
}

d_token_t* d_get(d_token_t* item, const uint16_t key) {
  if (item == NULL || (item->len & 0xF0000000) != 0x30000000) return NULL; // is it an object?
  int i = 0, l = item->len & 0xFFFFFFF;                                    // l is the number of properties in the object
  item += 1;                                                               // we start with the first, which is the next token
  for (; i < l; i++, item += d_token_size(item)) {                         // and iterate through all
    if (item->key == key) return item;                                     // until we find the one with the matching key
  }
  return NULL;
}
d_token_t* d_get_or(d_token_t* item, const uint16_t key, const uint16_t key2) {
  if (item == NULL) return NULL;
  d_token_t* s = NULL;
  int        i = 0, l = item->len & 0xFFFFFFF;
  item += 1;
  for (; i < l; i++, item += d_token_size(item)) {
    if (item->key == key) return item;
    if (item->key == key2) s = item;
  }
  return s;
}

d_token_t* d_get_at(d_token_t* item, const uint32_t index) {
  if (item == NULL || (item->len & 0xF0000000) != 0x20000000) return NULL; // is it an array?
  uint32_t i = 0, l = item->len & 0xFFFFFFF;
  item += 1;
  for (; i < l; i++, item += d_token_size(item)) {
    if (i == index) return item;
  }
  return NULL;
}

d_token_t* d_next(d_token_t* item) {
  return item == NULL ? NULL : item + d_token_size(item);
}

static NONULL char next_char(json_ctx_t* jp) {
  while (true) {
    switch (*jp->c) {
      case ' ':
      case '\n':
      case '\r':
      case '\t':
        jp->c++;
        break;
      default:
        return *(jp->c++);
    }
  }
}

static RETURNS_NONULL NONULL d_token_t* parsed_next_item(json_ctx_t* jp, d_type_t type, d_key_t key, int parent) {
  if (jp->len + 1 > jp->allocated) {
    jp->result = _realloc(jp->result, (jp->allocated << 1) * sizeof(d_token_t), jp->allocated * sizeof(d_token_t));
    jp->allocated <<= 1;
  }
  d_token_t* n = jp->result + jp->len;
  jp->len += 1;
  n->state = 0;
  n->key   = key;
  n->data  = NULL;
  n->len   = type << 28;
  if (parent >= 0) jp->result[parent].len++;
  return n;
}

static NONULL int parse_key(json_ctx_t* jp) {
  const char* start = jp->c;
  int         r;
  while (true) {
    switch (*(jp->c++)) {
      case 0: return JSON_E_INVALID_CHAR;
      case '"':
        r = d_add_key(jp, start, jp->c - start - 1);
        return next_char(jp) == ':' ? r : -2;
      case '\\':
        jp->c++;
        break;
    }
  }
}

static NONULL int parse_number(json_ctx_t* jp, d_token_t* item) {
  uint64_t value = 0; // the resulting value (if it is a integer)
  jp->c--;            // NOSONAR -> we also need to include the previous character!

  for (int i = 0; i < 21; i++) {             // we are not accepting more than 20 characters, since a uint64 can hold up to 18446744073709552000 (which has 20 digits)
    if (jp->c[i] >= '0' && jp->c[i] <= '9')  // NOSONAR - as long as this is a digit
      value = value * 10 + (jp->c[i] - '0'); // NOSONAR - we handle it and add it to the value.
    else {
      switch (jp->c[i]) { // we found a non digit character
        case '.':
        case '-':
        case '+':
        case 'e':
        case 'E': {
          // this is still a number, but not a simple integer, so we find the end and add it as string
          bool has_e = jp->c[i] == 'E' || jp->c[i] == 'e';
          i++;
          while ((jp->c[i] >= '0' && jp->c[i] <= '9') || jp->c[i] == '-' || jp->c[i] == '.' || (!has_e && (jp->c[i] == 'E' || jp->c[i] == 'e') && (has_e = true))) i++;
          switch (jp->c[i]) {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
            case '}':
            case ']':
            case ',':
            case 0:
              break;
            default:
              jp->c += i;
              return JSON_E_INVALID_CHAR;
          }
          item->state = TOKEN_STATE_CONVERTED | TOKEN_STATE_ALLOCATED;
          item->data  = _malloc(i + 1);
          item->len   = T_STRING << 28 | (unsigned) i;
          memcpy(item->data, jp->c, i);
          item->data[i] = 0;
          break;
        }

        case ' ':
        case '\n':
        case '\r':
        case '\t':
        case '}':
        case ']':
        case ',':
        case 0:

          if ((value & 0xfffffffff0000000) == 0) // is it small ennough to store it in the length ?
            item->len |= (uint32_t) value;       // 32-bit number / no 64-bit number
          else {
            // as it is a 64-bit number we have to change the type from T_INTEGER to T_BYTES and treat it accordingly
            uint8_t tmp[8];
            long_to_bytes(value, tmp);
            uint8_t *p = tmp, len = 8;
            optimize_len(p, len);
            item->state = TOKEN_STATE_CONVERTED | TOKEN_STATE_ALLOCATED;
            item->data  = _malloc(len);
            item->len   = T_BYTES << 28 | len;
            memcpy(item->data, p, len);
          }
          break;
        default:
          jp->c += i;
          return JSON_E_INVALID_CHAR;
      }

      jp->c += i;
      return 0;
    }
  }
  return JSON_E_NUMBER_TOO_LONG;
}

static NONULL int parse_string(json_ctx_t* jp, d_token_t* item) {
  char*  start = jp->c;
  size_t l;
  int    escape = 0;

  while (true) {
    switch (*(jp->c++)) {
      case 0: return JSON_E_END_OF_STRING;
      case '"':
        l          = jp->c - start - 1 - escape;
        item->len  = l | T_STRING << 28;
        item->data = escape ? _malloc(l + 1) : start;
        if (escape) {
          char* x = start;
          for (size_t n = 0; n < l; n++, x++) {
            if (*x == '\\') {
              switch (x[1]) {
                case 'u':
                  item->data[n] = hexchar_to_int(x[4]) << 4 | hexchar_to_int(x[5]);
                  x += 4;
                  break;
                case 'n':
                  item->data[n] = '\n';
                  break;
                case 't':
                  item->data[n] = '\t';
                  break;
                case 'b':
                  item->data[n] = '\b';
                  break;
                case 'f':
                  item->data[n] = '\f';
                  break;
                case 'r':
                  item->data[n] = '\r';
                  break;
                default:
                  item->data[n] = x[1];
              }
              x++;
            }
            else
              item->data[n] = *x;
          }
          item->state   = TOKEN_STATE_ALLOCATED | TOKEN_STATE_CONVERTED;
          item->data[l] = 0;
        }
        return 0;
      case '\\':
        if (*jp->c == 'u') {
          for (int n = 0; n < 4; n++) {
            if (hexchar_to_int(*(++jp->c)) == 255)
              return JSON_E_INVALID_CHAR;
          }
          escape += 4;
        }
        else if (*jp->c != '"' && *jp->c != '/' && *jp->c != '\\' && *jp->c != 'b' && *jp->c != 'f' && *jp->c != 'n' && *jp->c != 'r' && *jp->c != 't')
          return JSON_E_INVALID_CHAR;
        jp->c++;
        escape++;
        break;
    }
  }
}

static NONULL int parse_object(json_ctx_t* jp, int parent, uint32_t key) {
  int res, p_index = jp->len;

  if (jp->depth > DATA_DEPTH_MAX)
    return JSON_E_MAX_DEPTH;

  switch (next_char(jp)) {
    case 0:
      return JSON_E_END_OF_STRING;
    case '{':
      jp->depth++;
      parsed_next_item(jp, T_OBJECT, key, parent)->data = (uint8_t*) jp->c - 1;
      while (true) {
        switch (next_char(jp)) {
          case '"':
            res = parse_key(jp);
            if (res < 0) return res;
            break;
          case '}': {
            jp->depth--;
            return 0;
          }
          default:
            return JSON_E_INVALID_CHAR; // invalid character or end
        }
        res = parse_object(jp, p_index, res); // parse the value
        if (res < 0) return res;
        switch (next_char(jp)) {
          case ',': break; // we continue reading the next property
          case '}': {
            jp->depth--;
            return 0; // this was the last property, so we return successfully.
          }
          default:
            return JSON_E_INVALID_CHAR; // unexpected character, throw.
        }
      }
    case '[':
      jp->depth++;
      parsed_next_item(jp, T_ARRAY, key, parent)->data = (uint8_t*) jp->c - 1;
      if (next_char(jp) == ']') {
        jp->depth--;
        return 0;
      }
      jp->c--;

      while (true) {
        res = parse_object(jp, p_index, jp->result[p_index].len & 0xFFFFFF); // parse the value
        if (res < 0) return res;
        switch (next_char(jp)) {
          case ',': break; // we continue reading the next property
          case ']': {
            jp->depth--;
            return 0; // this was the last element, so we return successfully.
          }
          default:
            return JSON_E_INVALID_CHAR; // unexpected character, throw.
        }
      }
    case '"':
      return parse_string(jp, parsed_next_item(jp, T_STRING, key, parent));
    case 't':
      if (strncmp(jp->c, "rue", 3) == 0) {
        parsed_next_item(jp, T_BOOLEAN, key, parent)->len |= 1;
        jp->c += 3;
        return 0;
      }
      else
        return JSON_E_INVALID_CHAR;
    case 'f':
      if (strncmp(jp->c, "alse", 4) == 0) {
        parsed_next_item(jp, T_BOOLEAN, key, parent);
        jp->c += 4;
        return 0;
      }
      else
        return JSON_E_INVALID_CHAR;
    case 'n':
      if (strncmp(jp->c, "ull", 3) == 0) {
        parsed_next_item(jp, T_NULL, key, parent);
        jp->c += 3;
        return 0;
      }
      else
        return JSON_E_INVALID_CHAR;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '+':
    case '-':
      return parse_number(jp, parsed_next_item(jp, T_INTEGER, key, parent));
    default:
      return JSON_E_INVALID_CHAR;
  }
}

void json_free(json_ctx_t* jp) {
  if (!jp || jp->result == NULL) return;
  for (size_t i = 0; i < jp->len; i++) {
    if (jp->result[i].data != NULL && (jp->result[i].state & TOKEN_STATE_ALLOCATED))
      _free(jp->result[i].data);
  }
  if (jp->keys) _free(jp->keys);
  _free(jp->result);
  _free(jp);
}

char* parse_json_error(const char* js) {
  json_ctx_t parser = {0};                                           // new parser
  parser.c          = (char*) js;                                    // the pointer to the string to parse
  parser.allocated  = JSON_INIT_TOKENS;                              // keep track of how many tokens we allocated memory for
  parser.result     = _malloc(sizeof(d_token_t) * JSON_INIT_TOKENS); // we allocate memory for the tokens and reallocate if needed.
  const int res     = parse_object(&parser, -1, 0);                  // now parse starting without parent (-1)
  for (size_t i = 0; i < parser.len; i++) {
    if (parser.result[i].data != NULL && parser.result[i].state & TOKEN_STATE_ALLOCATED)
      _free(parser.result[i].data);
  }
  _free(parser.result);
  if (res == 0 || res < JSON_E_NUMBER_TOO_LONG) return NULL;
  const char* messages[] = {
      "premature end of json-string",
      "Unexpected character",
      "Reached max depth for parsing json",
      "Number too long to parse"};
  sb_t sb = {0};
  sb_print(&sb, "Error parsing json : %s\n", messages[-1 - res]);
  int l   = (int) (parser.c - js) - 1;
  int len = strlen(js);
  int s   = max(l - 30, 0);
  sb_add_range(&sb, js, s, min(l - s + 30, len - s));
  sb_add_char(&sb, '\n');
  for (int n = 0; n < 30 && n < l; n++) sb_add_char(&sb, '-');
  sb_add_char(&sb, '^');
  return sb.data;
}

json_ctx_t* parse_json(const char* js) {
  json_ctx_t* parser = _calloc(1, sizeof(json_ctx_t));                // new parser
  parser->c          = (char*) js;                                    // the pointer to the string to parse
  parser->allocated  = JSON_INIT_TOKENS;                              // keep track of how many tokens we allocated memory for
  parser->result     = _malloc(sizeof(d_token_t) * JSON_INIT_TOKENS); // we allocate memory for the tokens and reallocate if needed.
  const int res      = parse_object(parser, -1, 0);                   // now parse starting without parent (-1)
  if (res < 0) {                                                      // error parsing?
    json_free(parser);                                                // clean up
    return NULL;                                                      // and return null
  }                                                                   //
  parser->c = (char*) js;                                             // since this pointer changed during parsing, we set it back to the original string
  for (size_t i = 0; i < parser->len; i++) {
    //    if (d_is_bytes(parser->result + i) && d_type(parser->result + i) == T_STRING) d_bytes(parser->result + i);
  }
  return parser;
}

json_ctx_t* parse_json_indexed(const char* js) {
  json_ctx_t* parser = _calloc(1, sizeof(json_ctx_t));                // new parser
  parser->c          = (char*) js;                                    // the pointer to the string to parse
  parser->allocated  = JSON_INIT_TOKENS;                              // keep track of how many tokens we allocated memory for
  parser->result     = _malloc(sizeof(d_token_t) * JSON_INIT_TOKENS); // we allocate memory for the tokens and reallocate if needed.
  parser->keys       = _malloc(JSON_INDEXD_PAGE);
  const int res      = parse_object(parser, -1, 0); // now parse starting without parent (-1)
  if (res < 0) {                                    // error parsing?
    json_free(parser);                              // clean up
    return NULL;                                    // and return null
  }                                                 //
  parser->c = (char*) js;                           // since this pointer changed during parsing, we set it back to the original string
  return parser;
}

static int find_end(const char* str) {
  int         l         = 0;
  const char* c         = str;
  bool        in_string = false;
  while (*c != 0) {
    switch (*(c++)) {
      case '\\':
        if (in_string && !*(c++)) return 0; // just in case the string ends after a escape character
        break;
      case '"':
        in_string = !in_string;
        break;
      case '{':
      case '[':
        if (!in_string) l++;
        break;
      case '}':
      case ']':
        if (!in_string) l--;
        break;
    }
    if (l == 0)
      return c - str;
  }
  return c - str;
}

char* d_create_json(json_ctx_t* ctx, d_token_t* item) {
  char*       dst = NULL;
  int         l   = d_len(item);
  str_range_t s;
  switch (d_type(item)) {
    case T_ARRAY:
    case T_OBJECT:
      if (item->data) {
        s   = d_to_json(item);
        dst = _malloc(s.len + 1);
        memcpy(dst, s.data, s.len);
        dst[s.len] = 0;
      }
      else {
        sb_t sb = {0};
        sb_add_char(&sb, d_type(item) == T_ARRAY ? '[' : '{');
        for_children_of(it, item) {
          char* p = d_create_json(ctx, it.token);
          if (sb.len > 1) sb_add_char(&sb, ',');
          if (d_type(item) == T_OBJECT) {
            char* kn = d_get_keystr(ctx, it.token->key);
            if (kn)
              sb_printx(&sb, "\"%s\":", kn);
            else
              sb_printx(&sb, "\"%04x\":", (uint64_t) it.token->key);
          }
          sb_add_chars(&sb, p);
          _free(p);
        }
        sb_add_char(&sb, d_type(item) == T_ARRAY ? ']' : '}');
        dst = sb.data;
      }
      return dst;
    case T_BOOLEAN:
      return d_int(item) ? _strdupn("true", 4) : _strdupn("false", 5);
    case T_INTEGER: {
      bytes_t b  = d_bytes(item);
      sb_t    sb = {.allocted = 15, .data = _malloc(16), .len = 0};
      sb_add_rawbytes(&sb, "\"0x", b, -1);
      return sb_add_char(&sb, '"')->data;
      //      return sb_add_int(&sb, d_int(item))->data;
    }
    case T_NULL:
      return _strdupn("null", 4);
    case T_STRING: {
      sb_t sb = {.allocted = l + 1, .len = 0, .data = _malloc(l + 3)};
      sb_add_char(&sb, '"');
      sb_add_escaped_chars(&sb, (char*) item->data, l);
      sb_add_char(&sb, '"');
      return sb.data;
    }
    case T_BYTES: {
      bytes_t b  = d_bytes(item);
      sb_t    sb = {.allocted = l * 2 + 5, .len = 0};
      sb.data    = _malloc(sb.allocted);
      sb_add_rawbytes(&sb, "\"0x", b, b.len < 4 && !(b.len && b.data[0] == 0) ? -1 : 0);
      return sb_add_char(&sb, '"')->data;
    }
  }
  return NULL;
}

str_range_t d_to_json(const d_token_t* item) {
  str_range_t s;
  if (item && item->data) {
    s.data = (char*) item->data;
    s.len  = find_end(s.data);
  }
  else {
    s.data = NULL;
    s.len  = 0;
  }
  return s;
}

//    bytes-parser

static d_token_t* next_item(json_ctx_t* jp, d_type_t type, int len) {
  if (jp->allocated == 0) {
    jp->result    = _malloc(10 * sizeof(d_token_t));
    jp->allocated = 10;
  }
  else if (jp->len >= jp->allocated) {
    jp->result = _realloc(jp->result, (jp->allocated << 1) * sizeof(d_token_t), jp->allocated * sizeof(d_token_t));
    jp->allocated <<= 1;
  }
  assert(jp->len < jp->allocated);
  d_token_t* n = jp->result + jp->len;
  jp->len += 1;
  n->key   = 0;
  n->state = 0;
  n->data  = NULL;
  n->len   = type << 28 | len;
  return n;
}

static int read_token(json_ctx_t* jp, const uint8_t* d, size_t* p, size_t max) {
  if (*p >= max) return -3;                          // check limits
  uint16_t       key;                                // represents the key or hash of the propertyname (use in objects only)
  const d_type_t type = d[*p] >> 5;                  // first 3 bits define the type
  uint32_t       len  = d[(*p)++] & 0x1F, i;         // the other 5 bits  (0-31) the length
  uint32_t       l    = len > 27 ? len - 27 : 0, ll; // since len is max 31 we use the last 4 number for the number of bytes describing the length
  if ((*p + l) > max) return -3;                     // check limits

  if (len == 28)
    len = d[*p];                                                      // 28 = 1 byte len
  else if (len == 29)                                                 //
    len = d[*p] << 8 | d[*p + 1];                                     // 29 = 2 bytes length
  else if (len == 30)                                                 //
    len = d[*p] << 16 | d[*p + 1] << 8 | d[*p + 2];                   // 30 = 3 bytes length
  else if (len == 31)                                                 //
    len = d[*p] << 24 | d[*p + 1] << 16 | d[*p + 2] << 8 | d[*p + 3]; // 31 = 4 bytes length
  *p += l;                                                            // jump to the data

  if (type == T_NULL && len > 0) {                      // special token giving the number of tokens, so we can allocate the exact number
    if (len > JSON_MAX_ALLOWED_TOKENS) return -4;       // security check so we are not allocating too much memory
    if (jp->allocated == 0) {                           // first time?
      jp->result    = _malloc(sizeof(d_token_t) * len); // use malloc
      jp->allocated = len;                              //
    }
    else if (len > jp->allocated) {                                                                     // otherwise
      jp->result    = _realloc(jp->result, len * sizeof(d_token_t), jp->allocated * sizeof(d_token_t)); // realloc
      jp->allocated = len;
    }
    return 0;
  }

  if (type == T_BOOLEAN && len > 1) {                                      // special handling for references
    uint32_t idx = len - 2;                                                // -2 because the first 2 values are reserved for true and false
    if (jp->len < idx) return -1;                                          // make sure the index exists
    if (d_type(jp->result + idx) >= T_ARRAY) return -1;                    // it must be a bytes or string or it's an error
    memcpy(next_item(jp, type, len), jp->result + idx, sizeof(d_token_t)); // copy data including pointers
    return 0;
  }

  d_token_t* t = next_item(jp, type, len);
  switch (type) {
    case T_ARRAY:
      for (i = 0; i < len; i++) {
        ll = jp->len;
        TRY(read_token(jp, d, p, max));
        assert(ll < jp->allocated);
        jp->result[ll].key = i;
      }
      break;
    case T_OBJECT:
      for (i = 0; i < len; i++) {
        if (*p + 2 >= max) return -3;
        key = d[(*p)] << 8 | d[*p + 1];
        *p += 2;
        ll = jp->len;
        TRY(read_token(jp, d, p, max));
        assert(ll < jp->allocated);
        jp->result[ll].key = key;
      }
      break;
    case T_STRING:
      t->data = (uint8_t*) d + ((*p)++);
      if ((*p + len) > max || t->data[len] != 0) return -4; // must be null terminated
      *p += len;
      break;
    case T_BYTES:
      t->data = (uint8_t*) d + (*p);
      *p += len;
      if (*p > max) return -3;
      break;
    default:
      break;
  }
  return 0;
}

json_ctx_t* parse_binary_str(const char* data, int len) {
  const bytes_t b = {.data = (uint8_t*) data, .len = len};
  return parse_binary(&b);
}

json_ctx_t* parse_binary(const bytes_t* data) {
  size_t      p  = 0;                              // the current index within the data, which will be updated by read_token()
  json_ctx_t* jp = _calloc(1, sizeof(json_ctx_t)); // the resulting ctx
  jp->c          = (char*) data->data;             // data
  int error      = 0;                              // keep track of errors, but currently we don't report them since we return the json_ctx;

  // parse data
  while (!error && p < data->len)
    error = read_token(jp, data->data, &p, data->len);

  if (error) {
    // in case of an error we simply return NULL
    _free(jp->result);
    _free(jp);
    jp = NULL;
  }
  else
    // we use the allocated as marker for binary.
    // allocated == 0 means we don't need to free the bytes and strings in json_free()
    jp->allocated = 0;

  return jp;
}

json_ctx_t* json_create() {
  return _calloc(1, sizeof(json_ctx_t));
}
d_token_t* json_create_null(json_ctx_t* jp) {
  return next_item(jp, T_NULL, 0);
}
d_token_t* json_create_bool(json_ctx_t* jp, bool value) {
  return next_item(jp, T_BOOLEAN, value);
}

d_token_t* json_create_int(json_ctx_t* jp, uint64_t value) {
  if (value > 0xF0000000) {
    uint8_t tmp[8], *p = tmp, l = 8;
    long_to_bytes(value, tmp);
    optimize_len(p, l);
    d_token_t* r = next_item(jp, T_BYTES, l);
    r->data      = _malloc(l);
    r->state     = TOKEN_STATE_ALLOCATED;
    memcpy(r->data, p, l);
    return r;
  }

  return next_item(jp, T_INTEGER, value);
}
d_token_t* json_create_string(json_ctx_t* jp, char* value, int len) {
  if (len == -1) len = strlen(value);
  d_token_t* r = next_item(jp, T_STRING, len);
  r->state     = TOKEN_STATE_ALLOCATED;
  r->data      = _malloc(len + 1);
  memcpy(r->data, value, len);
  r->data[len] = 0;
  return r;
}
d_token_t* json_create_bytes(json_ctx_t* jp, bytes_t value) {
  d_token_t* r = next_item(jp, T_BYTES, value.len);
  r->state     = TOKEN_STATE_ALLOCATED;
  memcpy(r->data = _malloc(value.len), value.data, value.len);
  return r;
}

d_token_t* json_create_ref_item(json_ctx_t* jp, d_type_t type, void* data, int len) {
  d_token_t* r = next_item(jp, type, len);
  r->data      = data;
  return r;
}

int json_create_object(json_ctx_t* jp) {
  next_item(jp, T_OBJECT, 0);
  return jp->len - 1;
}

int json_create_array(json_ctx_t* jp) {
  next_item(jp, T_ARRAY, 0);
  return jp->len - 1;
}

void json_object_add_prop(json_ctx_t* jp, int ob_index, d_key_t key, d_token_t* value) {
  jp->result[ob_index].len++;
  value->key = key;
}

void json_array_add_value(json_ctx_t* jp, int ob_index, d_token_t* value) {
  d_token_t* object = jp->result + ob_index;
  value->key        = object->len;
  object->len++;
}

static void write_token_count(bytes_builder_t* bb, int len) {
  bb_write_byte(bb, T_NULL << 5 | (len < 28 ? len : min_bytes_len(len) + 27));
  if (len > 27)
    bb_write_long_be(bb, len, min_bytes_len(len));
}

static void write_token(bytes_builder_t* bb, d_token_t* t) {
  int        len = d_len(t), i;
  d_token_t* c   = NULL;
  bb_write_byte(bb, d_type(t) << 5 | (len < 28 ? len : min_bytes_len(len) + 27));
  if (len > 27)
    bb_write_long_be(bb, len, min_bytes_len(len));

  switch (d_type(t)) {
    case T_ARRAY:
      for (i = 0, c = t + 1; i < len; i++, c = d_next(c)) write_token(bb, c);
      break;
    case T_BYTES:
      bb_write_raw_bytes(bb, t->data, len);
      break;
    case T_OBJECT:
      for (i = 0, c = t + 1; i < len; i++, c = d_next(c)) {
        bb_write_long_be(bb, c->key, 2);
        write_token(bb, c);
      }
      break;
    case T_STRING:
      bb_write_raw_bytes(bb, t->data, len);
      bb_write_byte(bb, 0);
      break;
    case T_BOOLEAN:
    case T_INTEGER:
    case T_NULL:
      break;
  }
}

void d_serialize_binary(bytes_builder_t* bb, d_token_t* t) {
  write_token_count(bb, d_token_size(t));
  write_token(bb, t);
}
static const char _hex[] = "0123456789abcdef";
static char       _tmp[7];

char* d_get_property_name(d_token_internal_t* ob, d_key_t k) {
  str_range_t r = d_to_json(ob);
  if (!r.data) return NULL;
  for (char* s = strchr(r.data, '"'); s && r.data + r.len > s; s = s ? strchr(s + 1, '"') : NULL) {
    char* e = s + 1;
    for (e = strchr(e, '"'); e && r.data + r.len > e; e = strchr(e + 1, '"')) {
      if (e[-1] == '\\') continue; // NOSONAR - this is safe, because e >= s + 1
      if (keyn(s + 1, e - s - 1) == k) return _strdupn(s + 1, e - s - 1);
      break;
    }
    s = e;
  }
  return NULL;
}

char* d_get_keystr(json_ctx_t* ctx, d_key_t k) {
  if (ctx && ctx->keys) return get_key_str(ctx, k);
#ifdef LOGGING
  for (int i = 0; USED_KEYS[i]; i++) {
    if (key(USED_KEYS[i]) == k) return USED_KEYS[i];
  }
#endif

  _tmp[0] = '0';
  _tmp[1] = 'x';
  for (int i = 0; i < 0xFFFF; i++) {
    if (i <= 0xF) {
      _tmp[2] = _hex[i];
      _tmp[3] = 0;
    }
    else if (i <= 0xFF) {
      _tmp[2] = _hex[i >> 4];
      _tmp[3] = _hex[i & 0xf];
      _tmp[4] = 0;
    }
    else if (i <= 0xFFF) {
      _tmp[2] = _hex[i >> 8];
      _tmp[3] = _hex[i >> 4 & 0xf];
      _tmp[4] = _hex[i & 0xf];
      _tmp[5] = 0;
    }
    else {
      _tmp[2] = _hex[i >> 12 & 0xf];
      _tmp[3] = _hex[i >> 8 & 0xf];
      _tmp[4] = _hex[i >> 4 & 0xf];
      _tmp[5] = _hex[i & 0xf];
      _tmp[6] = 0;
    }
    if (key(_tmp) == k) return _tmp;
  }
  return NULL;
}
int d_bytes_to(d_token_t* item, uint8_t* dst, int max) {
  bytes_t val = d_bytes(item);
  if (max == -1) max = (int) val.len;
  if (max && (uint32_t) max < val.len) val.len = (uint32_t) max;
  if (val.len < (uint32_t) max) memset(dst, 0, max - val.len);
  if (val.data) memcpy(dst + max - val.len, val.data, val.len);
  return max;
}
bytes_t d_get_byteskl(d_token_t* r, d_key_t k, uint32_t minl) {
  d_token_t* t = d_get(r, k);
  return d_bytesl(t, minl);
}

d_token_t* d_getl(d_token_t* item, uint16_t k, uint32_t minl) {
  d_get_byteskl(item, k, minl);
  return d_get(item, k);
}

d_iterator_t d_iter(d_token_t* parent) {
  return (d_iterator_t){.left = d_len(parent), .token = parent + 1};
} /**< creates a iterator for a object or array */

d_token_t* token_from_string(char* val, d_token_t* d, bytes32_t buffer) {
  if (!val)
    d->len = T_NULL << 28;
  else {
    if (val[0] == '0' && val[1] == 'x') {
      int l = hex_to_bytes(val + 2, strlen(val + 2), buffer, 32);
      if (l < 5) {
        d->data = NULL;
        d->len  = bytes_to_int(buffer, 4) | (T_INTEGER << 28);
      }
      else {
        d->data = buffer;
        d->len  = l;
      }
    }
    else {
      d->data = (uint8_t*) val;
      d->len  = strlen(val) | (T_STRING << 28);
    }
  }
  return d;
}

d_token_t* token_from_bytes(bytes_t b, d_token_t* d) {
  if (!b.data)
    d->len = T_NULL << 28;
  else {
    d->data = b.data;
    d->len  = b.len;
  }
  return d;
}
d_token_t* token_from_int(uint32_t val, d_token_t* d) {
  d->data = NULL;
  d->len  = T_INTEGER << 28 | val;
  return d;
}

bytes_t d_num_bytes(d_token_t* f) {
  bytes_t bb = d_bytes(f);
  if (bb.data && d_type(f) == T_STRING && d_len(f) < 80) {
    // still a string?, then we need to look for numeric values
    bytes32_t dst = {0};
    size_t    dst_len;
    if (parse_decimal((void*) bb.data, bb.len, dst, &dst_len)) return bb;
    bytes_t b = bytes(dst, dst_len);
    b_optimize_len(&b);

    if (b.len < 4) {
      if (f->state & TOKEN_STATE_ALLOCATED) _free(f->data);
      f->data  = NULL;
      f->state = 0;
      f->len   = (T_INTEGER << 28) | bytes_to_int(b.data, b.len);
      memcpy(&f->data, b.data, b.len);
      return bytes((uint8_t*) &f->data, b.len);
    }

    f->data  = (f->state & TOKEN_STATE_ALLOCATED) ? f->data : _malloc(b.len);
    f->len   = b.len;
    f->state = TOKEN_STATE_CONVERTED | TOKEN_STATE_ALLOCATED;
    memcpy(f->data, b.data, b.len);
    return bytes(f->data, b.len);
  }
  return bb;
}

bool d_is_bytes(const d_token_t* item) {
  switch (d_type(item)) {
    case T_STRING: return (item->state & TOKEN_STATE_CONVERTED) == 0 && item->data[0] == '0' && item->data[1] == 'x';
    case T_BYTES:
    case T_INTEGER: return true;
    default: return false;
  }
}