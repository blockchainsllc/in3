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

#include "data.h"
#include "bytes.h"
#include "mem.h"
#include "stringbuilder.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "debug.h" // DEBUG !!!

// Here we check the pointer-size, because pointers smaller than 32bit may result in a undefined behavior, when calling d_to_bytes() for a T_INTEGER
#if UINTPTR_MAX == 0xFFFF
#error since we store a uint32_t in a pointer, pointers need to be at least 32bit!
#endif

#ifndef IN3_DONT_HASH_KEYS
static uint8_t __track_keys = 0;
#else
static uint8_t __track_keys = 1;
#endif

// number of tokens to allocate memory for when parsing
#define JSON_INIT_TOKENS 10

/** internal type declared here to assist with key() optimization */
typedef struct keyname {
  char*           name;
  d_key_t         key;
  struct keyname* next;
} keyname_t;

static keyname_t* __keynames = NULL;
#ifdef IN3_DONT_HASH_KEYS
static size_t     __keynames_len = 0;
static keyname_t* __last_keyname = NULL;

d_key_t key(const char* c) {
  keyname_t* kn = __keynames;
  while (kn) {
    if (!strcmp(kn->name, c))
      return kn->key;
    kn = kn->next;
  }
  return __keynames_len;
}
#endif

d_key_t keyn(const char* c, const size_t len) {
  d_key_t val = 0;
#ifndef IN3_DONT_HASH_KEYS
  size_t i = 0;
  for (; i < len; i++) {
    if (*c == 0) return val;
    val ^= *c | val << 7;
    c += 1;
  }
#else
  keyname_t* kn = __keynames;
  while (kn) {
    // input is not expected to be nul terminated
    if (strlen(kn->name) == len && !strncmp(kn->name, c, len))
      return kn->key;
    kn = kn->next;
  }
  val        = __keynames_len;
#endif
  return val;
}

void add_keyname(const char* name, d_key_t value, size_t len) {
  keyname_t* kn = malloc(sizeof(keyname_t));
#ifdef IN3_DONT_HASH_KEYS
  __keynames_len++;
  kn->next = NULL;
  if (__last_keyname)
    __last_keyname->next = kn;
  else
    __keynames = kn;
  __last_keyname = kn;
#else
  kn->next   = __keynames;
  __keynames = kn;
#endif

  kn->key  = value;
  kn->name = malloc(len + 1);
  memcpy(kn->name, name, len);
  kn->name[len] = 0;
}

static d_key_t add_key(const char* c, size_t len) {
  d_key_t k = keyn(c, len);
  if (!__track_keys) return k;
  keyname_t* kn = __keynames;
  while (kn) {
    if (kn->key == k) return k;
    kn = kn->next;
  }
  add_keyname(c, k, len);
  return k;
}

static size_t d_token_size(const d_token_t* item) {
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

bytes_t* d_bytes(const d_token_t* item) {
  return (bytes_t*) item;
}

bytes_t* d_bytesl(d_token_t* item, size_t l) {
  if (item == NULL || d_type(item) != T_BYTES)
    return NULL;
  else if (item->len >= l)
    return d_bytes(item);

  item->data = _realloc(item->data, l, item->len);
  memmove(item->data + l - item->len, item->data, item->len);
  memset(item->data, 0, l - item->len);
  item->len = l;
  return (bytes_t*) item;
}

bytes_t d_to_bytes(d_token_t* item) {
  switch (d_type(item)) {
    case T_BYTES:
      return bytes(item->data, item->len);
    case T_STRING:
      return bytes(item->data, d_len(item));
    case T_INTEGER:
    case T_BOOLEAN: {
      bytes_t dst;
      // why do we need this cast?
      // Because here we set the pointer to the pointer-field in the token.
      // Since a int/bool does not need the data-pointer, we simply reuse that memory and store the
      // bigendian representation of the int there.
      // but since we cannot accept a pointersize that is smaller than 32bit,
      // we have this check at the top of this file.
      dst.data = (uint8_t*) &item->data;
      dst.len  = d_bytes_to(item, dst.data, 4);
      dst.data += 4 - dst.len;
      return dst;
    }
    case T_NULL:
    default:
      return bytes(NULL, 0);
  }
}

int d_bytes_to(d_token_t* item, uint8_t* dst, const int max_size) {
  int max = max_size;
  if (item) {
    int l = d_len(item), i, val;
    if (l > max && max != -1) l = max;
    switch (d_type(item)) {
      case T_BYTES:
        if (max > l) {
          d_bytesl(item, max);
          l = max;
        }
        memcpy(dst, item->data, l);
        return l;

      case T_STRING:
        if (max > l) {
          memset(dst, 0, max - 1 - l);
          dst += max - l - 1;
        }
        memcpy(dst, item->data, l);
        dst[l] = 0;
        return l + 1;
      case T_BOOLEAN:
        memset(dst, 0, max - 1);
        dst[max - 1] = item->len & 1;
        return 1;
      case T_INTEGER:
        val = item->len & 0xFFFFFFF;
        if (max == -1) max = val & 0xFF000000 ? 4 : (val & 0xFF0000 ? 3 : (val & 0xFF00 ? 2 : 1));
        for (i = max < 3 ? max : 3; i >= 0; i--) {
          if (val & 0xFF << (i << 3)) {
            l = i + 1;
            if (max > l) {
              memset(dst, 0, max - l);
              dst += max - l;
            }

            for (; i >= 0; i--)
              dst[l - i - 1] = (val >> (i << 3)) & 0xFF;
            return l;
          }
        }
        break;
      default:
        break;
    }
  }
  memset(dst, 0, max);
  return 0;
}

char* d_string(const d_token_t* item) {
  if (item == NULL) return NULL;
  return (char*) item->data;
}

int32_t d_int(const d_token_t* item) {
  return d_intd(item, 0);
}

int32_t d_intd(const d_token_t* item, const uint32_t def_val) {
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
      return atoi((char*) item->data);
    default:
      return def_val;
  }
}

bytes_t** d_create_bytes_vec(const d_token_t* arr) {
  if (arr == NULL) return NULL;
  int              l   = d_len(arr), i;
  bytes_t**        dst = _calloc(l + 1, sizeof(bytes_t*));
  const d_token_t* t   = arr + 1;
  for (i = 0; i < l; i++, t += d_token_size(t)) dst[i] = d_bytes(t);
  return dst;
}

uint64_t d_long(const d_token_t* item) {
  return d_longd(item, 0L);
}
uint64_t d_longd(const d_token_t* item, const uint64_t def_val) {
  if (item == NULL) return def_val;
  if (d_type(item) == T_INTEGER)
    return item->len & 0x0FFFFFFF;
  else if (d_type(item) == T_BYTES)
    return bytes_to_long(item->data, item->len);
  else if (d_type(item) == T_STRING)
    return _strtoull((char*) item->data, NULL, 10);
  return def_val;
}

bool d_eq(const d_token_t* a, const d_token_t* b) {
  if (a == NULL || b == NULL) return false;
  if (a->len != b->len) return false;
  if (d_type(a) == T_ARRAY) {
    for (d_iterator_t ia = d_iter((d_token_t*) a), ib = d_iter((d_token_t*) b); ia.left; d_iter_next(&ia), d_iter_next(&ib)) {
      if (!d_eq(ia.token, ib.token)) return false;
    }
    return true;
  }
  if (d_type(a) == T_OBJECT) {
    for (d_iterator_t ia = d_iter((d_token_t*) a); ia.left; d_iter_next(&ia)) {
      if (!d_eq(ia.token, d_get((d_token_t*) b, ia.token->key))) return false;
    }
    return true;
  }
  return (a->data && b->data)
             ? b_cmp(d_bytes(a), d_bytes(b))
             : a->data == NULL && b->data == NULL;
}

d_token_t* d_get(d_token_t* item, const uint16_t key) {
  if (item == NULL /*|| item->len & 0xF0000000 != 0x30000000*/) return NULL;
  int i = 0, l = item->len & 0xFFFFFFF;
  item += 1;
  for (; i < l; i++, item += d_token_size(item)) {
    if (item->key == key) return item;
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
  if (item == NULL) return NULL;
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

char next_char(json_ctx_t* jp) {
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

d_token_t* parsed_next_item(json_ctx_t* jp, d_type_t type, d_key_t key, int parent) {
  if (jp->len + 1 > jp->allocated) {
    jp->result = _realloc(jp->result, (jp->allocated << 1) * sizeof(d_token_t), jp->allocated * sizeof(d_token_t));
    jp->allocated <<= 1;
  }
  d_token_t* n = jp->result + jp->len;
  jp->len += 1;
  n->key  = key;
  n->data = NULL;
  n->len  = type << 28;
  if (parent >= 0) jp->result[parent].len++;
  return n;
}

int parse_key(json_ctx_t* jp) {
  const char* start = jp->c;
  int         r;
  while (true) {
    switch (*(jp->c++)) {
      case 0: return -2;
      case '"':
        r = add_key(start, jp->c - start - 1);
        return next_char(jp) == ':' ? r : -2;
      case '\\':
        jp->c++;
        break;
    }
  }
}

int parse_number(json_ctx_t* jp, d_token_t* item) {
  int     i      = 0;
  int64_t i64Val = 0;
  bool    neg    = false;

  if (jp->c[-1] == '-')
    neg = true;
  if (jp->c[-1] != '+' && jp->c[-1] != '-')
    jp->c--;

  for (; i < 20; i++) {
    if (jp->c[i] >= '0' && jp->c[i] <= '9')
      i64Val = i64Val * 10 + (jp->c[i] - '0');
    else {
      // if the value is a float (which we don't support yet), we keep on parsing, but ignoring the rest of the numbers
      if (jp->c[i] == '.') {
        i++;
        while (jp->c[i] >= '0' && jp->c[i] <= '9') i++;
      }

      jp->c += i;

      if (neg) {
        char   tmp[22]; // max => -18446744073709551000
        size_t l   = sprintf(tmp, "-%" PRIi64, i64Val);
        item->len  = l | T_STRING << 28;
        item->data = _malloc(l + 1);
        memcpy(item->data, tmp, l);
        item->data[l] = 0;
      } else if ((i64Val & 0xfffffffff0000000) == 0)
        item->len |= (int) i64Val;
      // 32-bit number / no 64-bit number
      else {
        uint8_t tmp[8];
        // as it is a 64-bit number we have to change the type from T_INTEGER to T_BYTES and treat it accordingly
        long_to_bytes(i64Val, tmp);
        uint8_t *p = tmp, len = 8;
        optimize_len(p, len);
        item->data = _malloc(len);
        item->len  = T_BYTES << 28 | len;
        memcpy(item->data, p, len);
      }
      return 0;
    }
  }
  return -2;
}

int parse_string(json_ctx_t* jp, d_token_t* item) {
  char*  start = jp->c;
  size_t l, i;
  int    n;

  while (true) {
    switch (*(jp->c++)) {
      case 0: return -2;
      case '\'':
      case '"':
        l = jp->c - start - 1;
        if (l > 1 && *start == '0' && start[1] == 'x' && *(start - 1) != '\'') {
          // this is a hex-value
          if (l == 2) {
            // empty byte array
            item->len  = 0;
            item->data = NULL;
          } else if (l < 10 && !(l > 3 && start[2] == '0' && start[3] == '0')) { // we can accept up to 3,4 bytes as integer
            item->len = T_INTEGER << 28;
            for (i = 2; i < l; i++)
              item->len |= hexchar_to_int(start[i]) << ((l - i - 1) << 2);
          } else {
            // we need to allocate bytes for it. and so set the type to bytes
            item->len  = ((l & 1) ? l - 1 : l - 2) >> 1;
            item->data = _malloc(item->len);
            if (l & 1) item->data[0] = hexchar_to_int(start[2]);
            l = (l & 1) + 2;
            for (i = l - 2, n = l; i < item->len; i++, n += 2)
              item->data[i] = hexchar_to_int(start[n]) << 4 | hexchar_to_int(start[n + 1]);
          }
        } else if (l == 6 && *start == '\\' && start[1] == 'u') {
          item->len   = 1;
          item->data  = _malloc(1);
          *item->data = hexchar_to_int(start[4]) << 4 | hexchar_to_int(start[5]);
        } else {
          if (*(start - 1) == '\'') {
            // this is a escape-sequence which forces this to handled as string
            // here we do change or fix the input string because this would be an invalid string otherwise.
            *(jp->c - 1) = (*(start - 1) = '"');
          }
          item->len  = l | T_STRING << 28;
          item->data = _malloc(l + 1);
          memcpy(item->data, start, l);
          item->data[l] = 0;
        }
        return 0;
      case '\\': jp->c++; break;
    }
  }
}

int parse_object(json_ctx_t* jp, int parent, uint32_t key) {
  int res, p_index = jp->len;

  if (jp->depth > DATA_DEPTH_MAX)
    return -3;

  switch (next_char(jp)) {
    case 0: return -2;
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
          default: return -2; // invalid character or end
        }
        res = parse_object(jp, p_index, res); // parse the value
        if (res < 0) return res;
        switch (next_char(jp)) {
          case ',': break; // we continue reading the next property
          case '}': {
            jp->depth--;
            return 0; // this was the last property, so we return successfully.
          }
          default: return -2; // unexpected character, throw.
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
          default: return -2; // unexpected character, throw.
        }
      }
    case '"':
    case '\'':
      return parse_string(jp, parsed_next_item(jp, T_STRING, key, parent));
    case 't':
      if (strncmp(jp->c, "rue", 3) == 0) {
        parsed_next_item(jp, T_BOOLEAN, key, parent)->len |= 1;
        jp->c += 3;
        return 0;
      } else
        return -2;
    case 'f':
      if (strncmp(jp->c, "alse", 4) == 0) {
        parsed_next_item(jp, T_BOOLEAN, key, parent);
        jp->c += 4;
        return 0;
      } else
        return -2;
    case 'n':
      if (strncmp(jp->c, "ull", 3) == 0) {
        parsed_next_item(jp, T_NULL, key, parent);
        jp->c += 3;
        return 0;
      } else
        return -2;
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
      return -2;
  }
}

void json_free(json_ctx_t* jp) {
  if (!jp || jp->result == NULL) return;
  if (!d_is_binary_ctx(jp)) {
    size_t i;
    for (i = 0; i < jp->len; i++) {
      if (jp->result[i].data != NULL && d_type(jp->result + i) < 2)
        _free(jp->result[i].data);
    }
  }
  _free(jp->result);
  _free(jp);
}

json_ctx_t* parse_json(char* js) {
  json_ctx_t* parser = _malloc(sizeof(json_ctx_t));                  // new parser
  if (!parser) return NULL;                                          // not enoug memory?
  parser->len       = 0;                                             // initial length
  parser->depth     = 0;                                             //  initial depth
  parser->c         = js;                                            // the pointer to the string to parse
  parser->allocated = JSON_INIT_TOKENS;                              // keep track of how many tokens we allocated memory for
  parser->result    = _malloc(sizeof(d_token_t) * JSON_INIT_TOKENS); // we allocate memory for the tokens and reallocate if needed.
  if (!parser->result) {                                             // not enough memory?
    _free(parser);                                                   // also free the parse since it does not make sense to parse  now.
    return NULL;                                                     // NULL means no memory
  }                                                                  //
  const int res = parse_object(parser, -1, 0);                       // now parse starting without parent (-1)
  if (res < 0) {                                                     // error parsing?
    json_free(parser);                                               // clean up
    return NULL;                                                     // and return null
  }                                                                  //
  parser->c = js;                                                    // since this pointer changed during parsing, we set it back to the original string
  return parser;
}

static int find_end(const char* str) {
  int         l = 0;
  const char* c = str;
  while (*c != 0) {
    switch (*(c++)) {
      case '{':
      case '[':
        l++;
        break;
      case '}':
      case ']':
        l--;
        break;
    }
    if (l == 0)
      return c - str;
  }
  return c - str;
}

char* d_create_json(d_token_t* item) {
  if (item == NULL) return NULL;
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
      } else {
        sb_t* sb = sb_new(d_type(item) == T_ARRAY ? "[" : "{");
        for (d_iterator_t it = d_iter(item); it.left; d_iter_next(&it)) {
          char* p = d_create_json(it.token);
          if (sb->len > 1) sb_add_char(sb, ',');
          if (d_type(item) == T_OBJECT) {
            char* kn = d_get_keystr(it.token->key);
            if (kn) {
              sb_add_char(sb, '"');
              sb_add_chars(sb, kn);
              sb_add_chars(sb, "\":");
            } else {
              char tmp[8];
              sprintf(tmp, "\"%04x\":", (uint32_t) it.token->key);
              sb_add_chars(sb, tmp);
            }
          }
          sb_add_chars(sb, p);
          _free(p);
        }
        sb_add_char(sb, d_type(item) == T_ARRAY ? ']' : '}');
        dst = sb->data;
        _free(sb);
      }
      return dst;
    case T_BOOLEAN:
      return d_int(item) ? _strdupn("true", 4) : _strdupn("false", 5);
    case T_INTEGER:
      dst = _malloc(16);
      sprintf(dst, "\"0x%x\"", d_int(item));
      return dst;
    case T_NULL:
      return _strdupn("null", 4);
    case T_STRING:
      dst        = _malloc(l + 3);
      dst[0]     = '"';
      dst[l + 1] = '"';
      dst[l + 2] = 0;
      memcpy(dst + 1, item->data, l);
      return dst;
    case T_BYTES:
      dst    = _malloc(l * 2 + 5);
      dst[0] = '"';
      dst[1] = '0';
      dst[2] = 'x';
      bytes_to_hex(item->data, item->len, dst + 3);
      dst[l * 2 + 3] = '"';
      dst[l * 2 + 4] = 0;
      return dst;
  }
  return NULL;
}

str_range_t d_to_json(const d_token_t* item) {
  str_range_t s;
  if (item) {
    s.data = (char*) item->data;
    s.len  = find_end(s.data);
  } else {
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
  } else if (jp->len + 1 > jp->allocated) {
    jp->result = _realloc(jp->result, (jp->allocated << 1) * sizeof(d_token_t), jp->allocated * sizeof(d_token_t));
    jp->allocated <<= 1;
  }
  d_token_t* n = jp->result + jp->len;
  jp->len += 1;
  n->key  = 0;
  n->data = NULL;
  n->len  = type << 28 | len;
  return n;
}

static int read_token(json_ctx_t* jp, const uint8_t* d, size_t* p) {
  uint16_t       key;
  const d_type_t type = d[*p] >> 5; // first 3 bits define the type

  // calculate len
  uint32_t len = d[(*p)++] & 0x1F, i; // the other 5 bits  (0-31) the length
  int      l   = len > 27 ? len - 27 : 0, ll;
  if (len == 28)
    len = d[*p]; // 28 = 1 byte len
  else if (len == 29)
    len = d[*p] << 8 | d[*p + 1]; // 29 = 2 bytes length
  else if (len == 30)
    len = d[*p] << 16 | d[*p + 1] << 8 | d[*p + 2]; // 30 = 3 bytes length
  else if (len == 31)
    len = d[*p] << 24 | d[*p + 1] << 16 | d[*p + 2] << 8 | d[*p + 3]; // 31 = 4 bytes length
  *p += l;

  // special token giving the number of tokens, so we can allocate the exact number
  if (type == T_NULL && len > 0) {
    if (jp->allocated == 0) {
      jp->result    = _malloc(sizeof(d_token_t) * len);
      jp->allocated = len;
    } else if (len > jp->allocated) {
      jp->result    = _realloc(jp->result, len * sizeof(d_token_t), jp->allocated * sizeof(d_token_t));
      jp->allocated = len;
    }
    return 0;
  }
  // special handling for references
  if (type == T_BOOLEAN && len > 1) {
    uint32_t idx = len - 2;
    if (jp->len < idx) return -1;
    // if not bytes or string, it's an error
    if (d_type(jp->result + idx) >= T_ARRAY) return -1;
    memcpy(next_item(jp, type, len), jp->result + idx, sizeof(d_token_t));
    return 0;
  }
  d_token_t* t = next_item(jp, type, len);
  switch (type) {
    case T_ARRAY:
      for (i = 0; i < len; i++) {
        ll = jp->len;
        if (read_token(jp, d, p)) return 1;
        jp->result[ll].key = i;
      }
      break;
    case T_OBJECT:
      for (i = 0; i < len; i++) {
        key = d[(*p)] << 8 | d[*p + 1];
        *p += 2;
        ll = jp->len;
        if (read_token(jp, d, p)) return 1;
        jp->result[ll].key = key;
      }
      break;
    case T_STRING:
      t->data = (uint8_t*) d + ((*p)++);
      if (t->data[len] != 0) return 1;
      *p += len;
      break;
    case T_BYTES:
      t->data = (uint8_t*) d + (*p);
      *p += len;
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
  size_t      p = 0, error = 0;
  json_ctx_t* jp = _calloc(1, sizeof(json_ctx_t));
  jp->c          = (char*) data->data;

  while (!error && p < data->len)
    error = read_token(jp, data->data, &p);

  if (error) {
    _free(jp->result);
    _free(jp);
    return NULL;
  }
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
    memcpy(r->data, p, l);
    return r;
  }

  return next_item(jp, T_INTEGER, value);
}
d_token_t* json_create_string(json_ctx_t* jp, char* value) {
  d_token_t* r = next_item(jp, T_STRING, strlen(value));
  strcpy((char*) (r->data = _malloc(d_len(r) + 1)), value);
  return r;
}
d_token_t* json_create_bytes(json_ctx_t* jp, bytes_t value) {
  d_token_t* r = next_item(jp, T_BYTES, value.len);
  memcpy(r->data = _malloc(value.len), value.data, value.len);
  return r;
}

d_token_t* json_create_object(json_ctx_t* jp) {
  return next_item(jp, T_OBJECT, 0);
}

d_token_t* json_create_array(json_ctx_t* jp) {
  return next_item(jp, T_ARRAY, 0);
}

d_token_t* json_object_add_prop(d_token_t* object, d_key_t key, d_token_t* value) {
  object->len++;
  value->key = key;
  return object;
}

d_token_t* json_array_add_value(d_token_t* object, d_token_t* value) {
  value->key = object->len;
  object->len++;
  return object;
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
      bb_write_raw_bytes(bb, t->data, len + 1);
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

char* d_get_keystr(d_key_t k) {
  keyname_t* kn = __keynames;
  while (kn) {
    if (kn->key == k) return kn->name;
    kn = kn->next;
  }
  return NULL;
}

void d_track_keynames(uint8_t v) {
#ifndef IN3_DONT_HASH_KEYS
  __track_keys = v;
#else
  UNUSED_VAR(v);
#endif
}

void d_clear_keynames() {
#ifndef IN3_DONT_HASH_KEYS
  keyname_t* kn = NULL;
  while (__keynames) {
    kn = __keynames;
    free(kn->name);
    __keynames = kn->next;
    free(kn);
  }
#endif
}

bytes_t* d_get_byteskl(d_token_t* r, d_key_t k, uint32_t minl) {
  d_token_t* t = d_get(r, k);
  return d_bytesl(t, minl);
}

d_token_t* d_getl(d_token_t* item, uint16_t k, uint32_t minl) {
  d_get_byteskl(item, k, minl);
  return d_get(item, k);
}
