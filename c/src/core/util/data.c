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

#include "data.h"
#include "bytes.h"
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
// Here we check the pointer-size, because pointers smaller than 32bit may result in a undefined behavior, when calling d_to_bytes() for a T_INTEGER
// verify(sizeof(void*) >= 4);

// number of tokens to allocate memory for when parsing
#define JSON_INIT_TOKENS        10
#define JSON_INDEXD_PAGE        128
#define JSON_MAX_ALLOWED_TOKENS 1000000

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

static d_key_t add_key(json_ctx_t* ctx, const char* name, size_t len) {
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
  return d_type(item) == T_BYTES ? (bytes_t*) item : NULL;
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
          memset(dst, 0, max - l);
          memcpy(dst + max - l, item->data, l);
          d_bytesl(item, max); //TODO we should not need this!
          l = max;
        }
        else
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
        l   = 0;
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
        if (l == 0) {
          memset(dst, 0, max);
          return 1;
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
    for (d_iterator_t ia = d_iter((d_token_t*) a); ia.left; d_iter_next(&ia)) {
      if (!d_eq(ia.token, d_get((d_token_t*) b, ia.token->key))) return false;
    }
    return true;
  }
  if (d_type(a) == T_STRING) return strcmp((char*) a->data, (char*) b->data) == 0;

  return (a->data && b->data && d_type(a) == T_BYTES)
             ? b_cmp(d_bytes(a), d_bytes(b))
             : a->data == NULL && b->data == NULL;
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

NONULL char next_char(json_ctx_t* jp) {
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

RETURNS_NONULL NONULL d_token_t* parsed_next_item(json_ctx_t* jp, d_type_t type, d_key_t key, int parent) {
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

NONULL int parse_key(json_ctx_t* jp) {
  const char* start = jp->c;
  int         r;
  while (true) {
    switch (*(jp->c++)) {
      case 0: return -2;
      case '"':
        r = add_key(jp, start, jp->c - start - 1);
        return next_char(jp) == ':' ? r : -2;
      case '\\':
        jp->c++;
        break;
    }
  }
}

NONULL int parse_number(json_ctx_t* jp, d_token_t* item) {
  uint64_t value = 0; // the resulting value (if it is a integer)
  jp->c--;            // we also need to include hte previous character!

  for (int i = 0; i < 20; i++) {             // we are not accepting more than 20 characters, since a uint64 can hold up to 18446744073709552000 (which has 20 digits)
    if (jp->c[i] >= '0' && jp->c[i] <= '9')  // as long as this is a digit
      value = value * 10 + (jp->c[i] - '0'); // we handle it and add it to the value.
    else {
      switch (jp->c[i]) { // we found a non digit character
        case '.':
        case '-':
        case '+':
        case 'e':
        case 'E':
          // this is still a number, but not a simple integer, so we find the end and add it as string
          i++;
          while ((jp->c[i] >= '0' && jp->c[i] <= '9') || jp->c[i] == 'E' || jp->c[i] == 'e' || jp->c[i] == '-') i++;
          item->data = _malloc(i + 1);
          item->len  = T_STRING << 28 | (unsigned) i;
          memcpy(item->data, jp->c, i);
          item->data[i] = 0;
          break;

        default:
          if ((value & 0xfffffffff0000000) == 0) // is it small ennough to store it in the length ?
            item->len |= (uint32_t) value;       // 32-bit number / no 64-bit number
          else {
            // as it is a 64-bit number we have to change the type from T_INTEGER to T_BYTES and treat it accordingly
            uint8_t tmp[8];
            long_to_bytes(value, tmp);
            uint8_t *p = tmp, len = 8;
            optimize_len(p, len);
            item->data = _malloc(len);
            item->len  = T_BYTES << 28 | len;
            memcpy(item->data, p, len);
          }
          break;
      }

      jp->c += i;
      return 0;
    }
  }
  return -2;
}

NONULL int parse_string(json_ctx_t* jp, d_token_t* item) {
  char*  start = jp->c;
  size_t l, i;
  int    n;

  while (true) {
    switch (*(jp->c++)) {
      case 0:
        return -2;
      case '\'':
      case '"':
        if (start[-1] != jp->c[-1]) continue;
        l = jp->c - start - 1;
        if (l > 1 && *start == '0' && start[1] == 'x' && *(start - 1) != '\'') {
          // this is a hex-value
          if (l == 2) {
            // empty byte array
            item->len  = 0;
            item->data = NULL;
          }
          else if (l < 10 && !(l > 3 && start[2] == '0' && start[3] == '0')) { // we can accept up to 3,4 bytes as integer
            item->len = T_INTEGER << 28;
            for (i = 2; i < l; i++)
              item->len |= hexchar_to_int(start[i]) << ((l - i - 1) << 2);
          }
          else {
            // we need to allocate bytes for it. and so set the type to bytes
            item->len  = ((l & 1) ? l - 1 : l - 2) >> 1;
            item->data = _malloc(item->len);
            if (l & 1) item->data[0] = hexchar_to_int(start[2]);
            l = (l & 1) + 2;
            for (i = l - 2, n = l; i < item->len; i++, n += 2)
              item->data[i] = hexchar_to_int(start[n]) << 4 | hexchar_to_int(start[n + 1]);
          }
        }
        else if (l == 6 && *start == '\\' && start[1] == 'u') {
          item->len   = 1;
          item->data  = _malloc(1);
          *item->data = hexchar_to_int(start[4]) << 4 | hexchar_to_int(start[5]);
        }
        else {
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

NONULL int parse_object(json_ctx_t* jp, int parent, uint32_t key) {
  int res, p_index = jp->len;

  if (jp->depth > DATA_DEPTH_MAX)
    return -3;

  switch (next_char(jp)) {
    case 0:
      return -2;
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
            return -2; // invalid character or end
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
            return -2; // unexpected character, throw.
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
            return -2; // unexpected character, throw.
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
      }
      else
        return -2;
    case 'f':
      if (strncmp(jp->c, "alse", 4) == 0) {
        parsed_next_item(jp, T_BOOLEAN, key, parent);
        jp->c += 4;
        return 0;
      }
      else
        return -2;
    case 'n':
      if (strncmp(jp->c, "ull", 3) == 0) {
        parsed_next_item(jp, T_NULL, key, parent);
        jp->c += 3;
        return 0;
      }
      else
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
  if (jp->keys) _free(jp->keys);
  _free(jp->result);
  _free(jp);
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
        sb_t* sb = sb_new(d_type(item) == T_ARRAY ? "[" : "{");
        for (d_iterator_t it = d_iter(item); it.left; d_iter_next(&it)) {
          char* p = d_create_json(ctx, it.token);
          if (sb->len > 1) sb_add_char(sb, ',');
          if (d_type(item) == T_OBJECT) {
            char* kn = d_get_keystr(ctx, it.token->key);
            if (kn) {
              sb_add_char(sb, '"');
              sb_add_chars(sb, kn);
              sb_add_chars(sb, "\":");
            }
            else {
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
  n->key  = 0;
  n->data = NULL;
  n->len  = type << 28 | len;
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
    memcpy(r->data, p, l);
    return r;
  }

  return next_item(jp, T_INTEGER, value);
}
d_token_t* json_create_string(json_ctx_t* jp, char* value, int len) {
  if (len == -1) len = strlen(value);
  d_token_t* r = next_item(jp, T_STRING, len);
  r->data      = _malloc(len + 1);
  memcpy(r->data, value, len);
  r->data[len] = 0;
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
static const char _hex[] = "0123456789abcdef";
static char       _tmp[7];

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

bytes_t* d_get_byteskl(d_token_t* r, d_key_t k, uint32_t minl) {
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
