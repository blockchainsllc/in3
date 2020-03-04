/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3
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

#include "abi.h"
#include "../../core/util/bitset.h"
#include "../../core/util/bytes.h"
#include "../../core/util/data.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include <string.h>

static int add_error(call_request_t* req, char* error) {
  req->error = error;
  return -1;
}

static inline var_t* token(bytes_builder_t* bb, int i) {
  return (var_t*) (bb->b.data + i);
}
void req_free(call_request_t* req) {
  if (req->call_data) bb_free(req->call_data);
  if (req->in_data) _free(req->in_data);
  _free(req);
}

static int next_token(bytes_builder_t* bb, atype_t type) {
  if (bb_check_size(bb, bb->b.len + sizeof(var_t)) < 0) return -1;
  int r = bb->b.len;
  bb->b.len += sizeof(var_t);
  *token(bb, r) = (var_t){.type = type, .data = bytes(NULL, 0), .type_len = 0, .array_len = 0};
  return r;
}

int add_token(bytes_builder_t* bb, char* start, unsigned int len, int tuple) {
  if (len == 0) return 0;
  char name[50];
  memcpy(name, start, len);
  name[len]        = 0;
  int     alen     = 0;
  int     type_len = 0;
  atype_t atype    = A_UINT;
  // handle arrays
  if (name[len - 1] == ']') {
    char *p = name + len - 2, *bs = NULL;
    while (p > name) {
      if (*p == '[') {
        bs = p + 1;
        break;
      }
      p--;
    }
    if (!bs) return -1;
    if (bs == name + len - 1)
      alen = -1;
    else {
      name[len - 1] = 0;
      alen          = atoi(bs);
    }
    name[len = bs - name - 1] = 0;
  }

  if (strcmp(name, "address") == 0) {
    atype    = A_ADDRESS;
    type_len = 20;
  } else if (strncmp(name, "uint", 4) == 0) {
    atype    = A_UINT;
    type_len = strlen(name) == 4 ? 32 : (atoi(name + 4) / 8);
  } else if (strncmp(name, "int", 3) == 0) {
    atype    = A_INT;
    type_len = strlen(name) == 3 ? 32 : (atoi(name + 3) / 8);
  } else if (strcmp(name, "bool") == 0) {
    atype    = A_BOOL;
    type_len = 1;
  } else if (strncmp(name, "bytes", 5) == 0) {
    atype    = A_BYTES;
    type_len = len > 5 ? atoi(name + 5) : 0;
  } else if (strcmp(name, "string") == 0) {
    atype    = A_STRING;
    type_len = 0;
  } else
    return -1;

  var_t* t     = token(bb, next_token(bb, atype));
  t->array_len = alen;
  t->type_len  = type_len;
  token(bb, tuple)->type_len++;
  return 0;
}

char* parse_tuple(bytes_builder_t* bb, char* c) {
  int tuple = next_token(bb, A_TUPLE);

  char* start = c;
  while (*c) {
    if (*c == '(') {
      c = parse_tuple(bb, c + 1);
      if (!c || *c != ')') return NULL;
    } else if (*c == ')') {
      if (add_token(bb, start, c - start, tuple) < 0) return NULL;
      if (c[1] == '[') {
        char* end = strchr(c, ']');
        if (!end) return NULL;
        if (end == c + 2)
          token(bb, tuple)->array_len = -1;
        else {
          *end                        = 0;
          token(bb, tuple)->array_len = atoi(c + 2);
          *end                        = ']';
        }
      }
      return c;
    } else if (*c == ',') {
      if (add_token(bb, start, c - start, tuple) < 0) return NULL;
      start = c + 1;
    }
    c++;
  }
  return add_token(bb, start, c - start, tuple) < 0 ? NULL : c;
}

static void remove_parens(char* s, size_t l) {
  for (size_t i = l; i != 0; --i)
    if (s[i - 1] == '(' || s[i - 1] == ')')
      for (size_t j = i - 1; j <= l; ++j)
        s[j] = s[j + 1];
}

char* escape_tuples(char* sig, size_t l, char** startb, char** ends) {
  if (*sig) {
    char* sig_ = _strdupn(sig, l);
    *startb    = memchr(sig_, '(', l);
    *ends      = memchr(sig_, ':', l);
    remove_parens(sig_ + (*startb - sig_) + 1, (*ends ? (unsigned) (*ends - *startb) : (l - (*startb - sig_))) - 2);
    if (*ends) {
      *startb = memchr(*ends, '(', strlen(*ends));
      if (*startb) remove_parens(sig_ + (*startb - sig_) + 1, strlen(sig_ + (*startb - sig_) + 2));
    }
    *ends   = memchr(sig_, ':', l);
    *startb = memchr(sig_, '(', l);
    return sig_;
  }
  return NULL;
}

call_request_t* parseSignature(char* sig) {
  call_request_t* req = _malloc(sizeof(call_request_t));
  req->error          = NULL;
  int   l             = strlen(sig);
  char *ends = memchr(sig, ':', l), *startb = memchr(sig, '(', l);
  if (!startb) {
    add_error(req, "Invalid call-signature");
    return req;
  }

  char*            s         = escape_tuples(sig, l, &startb, &ends);
  bytes_t          signature = bytes((uint8_t*) s, ends ? (unsigned) (ends - s) : strlen(s));
  bytes32_t        hash;
  bytes_builder_t* tokens = bb_new();
  if (!parse_tuple(tokens, startb + 1)) {
    req->error = "invalid arguments in signature";
    return req;
  }
  int out_start = tokens->b.len;
  if (ends && !parse_tuple(tokens, ends + (ends[1] == '(' ? 2 : 1))) {
    req->error = "invalid return types in signature";
    return req;
  }
  req->in_data     = token(tokens, 0);
  req->out_data    = ends ? token(tokens, out_start) : NULL;
  req->current     = req->in_data;
  req->call_data   = bb_new();
  req->data_offset = 4;
  _free(tokens);

  // create input data
  sha3_to(&signature, hash);
  bb_write_raw_bytes(req->call_data, hash, 4); // write functionhash
  _free(s);
  return req;
}

static int t_size(var_t* t) {
  if (t->type == A_TUPLE) {
    int    n = 0, i = 0, tmp;
    var_t* s = NULL;
    for (s = t + 1; i < t->type_len; i++) {
      tmp = t_size(s);
      n += tmp;
      s += tmp;
    }
    return n;
  }
  return 1;
}

var_t* t_next(var_t* t) {
  return t + t_size(t);
}
int word_size(int b) {
  return (b + 31) / 32;
}

static bool is_dynamic(var_t* t) {
  if (t->array_len < 0) return true;
  if (t->type_len == 0 && (t->type == A_STRING || t->type == A_BYTES)) return true;
  if (t->type == A_TUPLE) {
    int    i;
    var_t* s = NULL;
    for (i = 0, s = t + 1; i < t->type_len; i++, s = t_next(s)) {
      if (is_dynamic(s)) return true;
    }
  }
  return false;
}

static int head_size(var_t* t, bool single) {
  if (is_dynamic(t) && !single) return 32;
  int f = t->array_len > 0 ? t->array_len : 1, a = 32;
  if (t->type == A_TUPLE) {
    int i;
    a        = 0;
    var_t* s = NULL;
    for (i = 0, s = t + 1; i < t->type_len; i++, s = t_next(s)) a += head_size(s, false);
  } else if (t->type == A_BYTES || t->type == A_STRING)
    a = word_size(t->type_len) * 32;
  return single ? a : (a * f);
}

static int check_buffer(call_request_t* req, int pos) {
  if ((uint32_t) pos > req->call_data->b.len) {
    if (bb_check_size(req->call_data, pos - req->call_data->b.len) < 0)
      return -1;
    req->call_data->b.len = pos;
  }
  return 0;
}

static int write_uint256(call_request_t* req, int p, uint32_t val) {
  if (check_buffer(req, p + 32) < 0) return -1;
  uint8_t* pos = req->call_data->b.data + p;
  memset(pos, 0, 28);
  int_to_bytes(val, pos + 28);
  return 32;
}
static int write_right(call_request_t* req, int p, bytes_t data) {
  int l = word_size(data.len) * 32;
  if (l == 0) l = 32;
  if (check_buffer(req, p + l) < 0) return -1;
  uint8_t* pos = req->call_data->b.data + p;
  if ((uint32_t) l > data.len) {
    memset(pos, 0, l - data.len);
    pos += l - data.len;
  }
  memcpy(pos, data.data, data.len);
  return l;
}
static int write_left(call_request_t* req, int p, bytes_t data) {
  int l = word_size(data.len) * 32;
  if (check_buffer(req, p + l) < 0) return -1;
  uint8_t* pos = req->call_data->b.data + p;
  memcpy(pos, data.data, data.len);
  if ((uint32_t) l > data.len)
    memset(pos + data.len, 0, l - data.len);
  return l;
}

static void twos_complement(bitset_t* bs) {
  // 2's complement
  unsigned k = 0;
  for (; k < 256; k++)
    if (bs_isset(bs, k))
      break;
  k++;
  for (; k < 256; k++)
    bs_toggle(bs, k);

  // Convert to big-endian
  uint8_t tmp;
  for (int i = 0; i < 16; ++i) {
    tmp                = bs->bits.p[i];
    bs->bits.p[i]      = bs->bits.p[31 - i];
    bs->bits.p[31 - i] = tmp;
  }
}

static int encode(call_request_t* req, d_token_t* data, var_t* tuple, int head_pos, int tail_pos) {
  bytes_builder_t* buffer    = req->call_data;
  int              array_len = tuple->array_len;
  d_token_t*       d         = data;

  if (is_dynamic(tuple) && tail_pos > head_pos) {
    write_uint256(req, head_pos, tail_pos - 4 /*head_pos*/);
    if (encode(req, data, tuple, tail_pos, -1) < 0) return -1;
    return head_pos + 32;
  }

  if (array_len < 0) {
    array_len = d_len(data);
    head_pos += write_uint256(req, head_pos, array_len);
    if (!array_len) return head_pos;
  }

  if (array_len) {
    if (array_len != d_len(data) || d_type(data) != T_ARRAY) return add_error(req, "wrong array_size!");
    d = data + 1;
  } else
    array_len = 1;

  for (int i = 0; i < array_len; i++, d = d_next(d)) {
    switch (tuple->type) {
      case A_TUPLE: {
        int n          = 0;
        int tail_start = head_pos + head_size(tuple, true);
        if (check_buffer(req, tail_start) < 0) return add_error(req, "wrong array_size!");
        var_t*     t  = tuple + 1;
        d_token_t* dd = d + 1;
        if (tuple->type_len != d_len(d) || d_type(d) != T_ARRAY) return add_error(req, "wrong tuple size!");
        for (n = 0; n < tuple->type_len; n++, t = t_next(t), dd = d_next(dd))
          head_pos = encode(req, dd, t, head_pos, max((int) buffer->b.len, tail_start));
        break;
      }
      case A_INT: {
        switch (d_type(d)) {
          case T_STRING: {
            char*              tmp = d_string(d);
            unsigned long long n   = _strtoull(tmp + 1, NULL, 10);
            bitset_t*          bs  = bs_from_ull(n, 256);
            twos_complement(bs);
            bytes_t b = {.data = bs->bits.p, .len = 32};
            head_pos += write_right(req, head_pos, (*tmp == '-') ? b : d_to_bytes(d));
            bs_free(bs);
            break;
          }
          case T_INTEGER:
          case T_BYTES: {
            head_pos += write_right(req, head_pos, d_to_bytes(d));
            break;
          }
          default:
            return add_error(req, "big negative numbers are not supported (yet)!");
        }
        break;
      }
      case A_ADDRESS:
      case A_UINT:
      case A_BOOL:
        head_pos += write_right(req, head_pos, d_to_bytes(d));
        break;
      case A_BYTES:
      case A_STRING:
        if (!tuple->type_len)
          head_pos += write_uint256(req, head_pos, d_len(d));
        head_pos += write_left(req, head_pos, d_to_bytes(d));

      default:
        break;
    }
  }
  return head_pos;
}

int set_data(call_request_t* req, d_token_t* data, var_t* tuple) {
  return encode(req, data, tuple, req->data_offset, -1);
}

d_token_t* get_data(json_ctx_t* ctx, var_t* t, bytes_t data, int* offset) {
  d_token_t* res = NULL;
  bytes_t    tmp;
  int        len = t->type_len, dst = *offset;

  if (t->array_len) {
    if (t->array_len == -1) { // dynamic
      dst = bytes_to_int(data.data + dst + 28, 4);
      len = bytes_to_int(data.data + dst + 28, 4);
      dst += 32;
    } else
      len = t->array_len;

    int ol       = t->array_len; // we store the old array-indentifier
    t->array_len = 0;            // because we need to temporarly deactivate it so we can fetch the single type.
    res          = json_create_array(ctx);
    for (int n = 0; n < len; n++)
      json_array_add_value(res, get_data(ctx, t, data, &dst));

    t->array_len = ol; // restore the array-identifier
    if (ol == -1)      // move offset
      *offset += 32;
    else
      *offset = dst;
    return res;
  }

  switch (t->type) {
    case A_TUPLE:
      res      = json_create_array(ctx);
      var_t* p = t + 1;
      for (int i = 0; i < len; i++, p = t_next(p))
        json_array_add_value(res, get_data(ctx, p, data, offset));
      break;
    case A_INT: {
      bitset_t bs  = {.len = 256, .bits.p = data.data + dst};
      bool     neg = bs_isset(&bs, 0U);
      twos_complement(&bs);

      unsigned long long n;
      memcpy(&n, bs.bits.p, sizeof(n));

      char buf[32];
      sprintf(buf, "%s%llu", neg ? "-" : "", n + 1);
      res = json_create_string(ctx, buf);
      break;
    }
    case A_UINT:
      tmp = bytes(data.data + dst, 32);
      b_optimize_len(&tmp);
      res = json_create_bytes(ctx, tmp);
      *offset += 32;
      break;
    case A_BOOL:
      tmp = bytes(data.data + dst, 32);
      b_optimize_len(&tmp);
      res = json_create_bool(ctx, tmp.len > 1 || (tmp.len == 1 && *tmp.data));
      *offset += 32;
      break;
    case A_ADDRESS:
      res = json_create_bytes(ctx, bytes(data.data + dst + 12, 20));
      *offset += 32;
      break;
    case A_STRING:
    case A_BYTES:

      if (!t->type_len) {
        dst = bytes_to_int(data.data + dst + 28, 4);
        len = bytes_to_int(data.data + dst + 28, 4);
        dst += 32;
      }

      if (t->type == A_STRING) {
        char* tmp = alloca(len + 1);
        strncpy(tmp, (char*) (data.data + dst), len);
        tmp[len] = '\0';
        res      = json_create_string(ctx, tmp);
      } else
        res = json_create_bytes(ctx, bytes(data.data + dst, len));
      *offset += t->type_len ? (word_size(len) << 5) : 32;

      break;

    default:
      break;
  }
  return res;
}

json_ctx_t* req_parse_result(call_request_t* req, bytes_t data) {
  int         offset = 0;
  json_ctx_t* res    = json_create();
  if (req->out_data->type_len == 0) return res;
  if (req->out_data->type_len == 1) {
    get_data(res, req->out_data + 1, data, &offset);
    return res;
  }
  get_data(res, req->out_data, data, &offset);

  return res;
}
