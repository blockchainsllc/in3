#include "abi.h"
#include "../core/util/bytes.h"
#include "../core/util/data.h"
#include "../core/util/utils.h"
#include <string.h>

//balanceOf(address):(uint256)
//balanceOf(address):()
//
// in3 call 0x123455345 balanceOf(address,(uint,bool)):uint256 0x2342342341
// in3 call 0x123455345 (uint256):balanceOf address:0x2342342341 uint256:123 bytes32:0x23432532452345
// in3 call 0x123455345 balanceOf:uint256 0x2342342341:address 123:uint256 0x23432532452345:bytes32 "testest"

static int add_error(call_request_t* req, char* error) {
  req->error = error;
  return -1;
}

static inline var_t* token(bytes_builder_t* bb, int i) {
  return (var_t*) bb->b.data + i;
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
    type_len = atoi(name + 4) / 8;
  } else if (strncmp(name, "int", 3) == 0) {
    atype    = A_INT;
    type_len = atoi(name + 3) / 8;
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

call_request_t* parseSignature(char* sig) {
  call_request_t* req  = _malloc(sizeof(call_request_t));
  int             l    = strlen(sig);
  char *          ends = memchr(sig, ':', l), *startb = memchr(sig, '(', l);
  if (!ends || !startb || ends < startb) {
    add_error(req, "Invalid call-signature");
    return req;
  }

  bytes_t          signature = bytes((uint8_t*) sig, ends - sig);
  bytes32_t        hash;
  bytes_builder_t *tokens = bb_new(), *data = bb_new();
  if (!parse_tuple(tokens, startb + 1)) {
    req->error = "invalid arguments in signature";
    return req;
  }
  int out_start = tokens->b.len;
  if (!parse_tuple(tokens, ends + (ends[1] == '(' ? 2 : 1))) {
    req->error = "invalid return types in signature";
    return req;
  }
  req->in_data  = token(tokens, 0);
  req->out_data = token(tokens, out_start);
  req->current  = req->in_data;
  _free(tokens);

  // create input data
  sha3_to(&signature, hash);
  bb_write_raw_bytes(data, hash, 4); // write functionhash

  req->data_offset = 4;

  return req;
}

static int t_size(var_t* t) {
  if (t->type == A_TUPLE) {
    int    n = 0, i = 0, tmp;
    var_t* s;
    for (s = t + 1; i < t->type_len; i++) {
      tmp = t_size(s);
      n += tmp;
      s += tmp;
    }
    return n;
  }
  return 1;
}

static var_t* t_next(var_t* t) {
  return t + t_size(t);
}
static int word_size(int b) {
  return (b + 31) / 32;
}

static int get_fixed_size(var_t* t) {
  int    i, all;
  var_t* s;
  switch (t->type) {
    case A_TUPLE:
      if (t->array_len < 0) return 32;
      for (i = 0, s = t + 1, all = 0; i < t->type_len; i++, s = t_next(s))
        all += get_fixed_size(s);
      return all * (t->array_len > 0 ? t->array_len : 1);

    case A_STRING:
      return 32;

    case A_BYTES:
      if (t->type_len == 0)
        return 32;
      return word_size(t->type_len) * (t->array_len > 0 ? t->array_len : 1);

    default:
      return word_size(t->type_len) * (t->array_len > 0 ? t->array_len : 1);
  }
}

int set_data(call_request_t* req, d_token_t* data, var_t* tuple, int pos) {

  switch (tuple->type) {
    case A_TUPLE:
      if (tuple->array_len < 0) {
        // this is a dynamic tuple
        return add_error(req, "dynamic tuples are not supported yet!");
      } else {
        int expected_size = tuple->array_len ? tuple->array_len : 1;
        if (tuple->array_len && d_len(data) != expected_size) return add_error(req, "wront tuple_size!");
        // [[1,2],[1,2]]
        d_token_t* t = tuple->array_len ? data + 1 : data;
        for (int n = 0; n < expected_size; n++, t = d_next(t)) {
          d_token_t* tt       = t + 1;
          var_t*     subTuple = tuple + 1;
          int        offset = pos, static_len = get_fixed_size(tuple);
          bb_check_size(req->call_data, static_len + offset - req->call_data->b.len); /// reserver enough for static
          memset(req->call_data->b.data + offset, 0, static_len);
          req->call_data->b.len += static_len;
          for (int i = 0; i < tuple->type_len; i++, subTuple = t_next(subTuple), tt = d_next(tt)) {
            if (set_data(req, tt, subTuple, pos) < 0) return -1;
            pos += get_fixed_size(subTuple);
          }
        }
      }
      return 0;
    case A_ADDRESS:
    case A_BOOL:
    case A_INT:
    case A_UINT: {
      int p = pos, len = 1;
      if (tuple->array_len < 0) {
        bytes32_t word;
        memset(word, 0, 32);
        len = d_len(data);
        int_to_bytes(len, word + 32 - 4);
        bb_check_size(req->call_data, len * 32 + 32);
        bb_write_raw_bytes(req->call_data, word, 32);                                                  // add the length to the tail
        int_to_bytes(req->call_data->b.len - req->data_offset, req->call_data->b.data + pos + 32 - 4); // write the offset
        p = req->call_data->b.len;
      } else if (tuple->array_len > 0)
        len = tuple->array_len;

      d_token_t* t = tuple->array_len ? data + 1 : data;
      for (int i = 0; i < len; i++, t = d_next(t), p += 32)
        d_bytes_to(t, req->call_data->b.data + p, 32);
    }
    case A_STRING:
    case A_BYTES: {
      int p = pos, len = 1;
      if (tuple->array_len < 0) {
        bytes32_t word;
        memset(word, 0, 32);
        len = d_len(data);
        int_to_bytes(len, word + 32 - 4);
        bb_check_size(req->call_data, len * 32 + 32);
        bb_write_raw_bytes(req->call_data, word, 32);                                                  // add the length to the tail
        int_to_bytes(req->call_data->b.len - req->data_offset, req->call_data->b.data + pos + 32 - 4); // write the offset
        p = req->call_data->b.len;
      } else if (tuple->array_len > 0)
        len = tuple->array_len;

      d_token_t* t = tuple->array_len ? data + 1 : data;
      for (int i = 0; i < len; i++, t = d_next(t)) {
        bytes_t b = d_to_bytes(t);

        if (tuple->type_len) {
          // static length
          if (b.len > tuple->type_len) b.len = tuple->type_len;
          memcpy(req->call_data->b.data + p, b.data, b.len);
          int wl = word_size(tuple->type_len);
          if (b.len < (uint32_t) wl)
            memset(req->call_data->b.data + p + b.len, 0, wl - b.len);
          p += wl;
        } else {
          // dynamic length
          memset(req->call_data->b.data + p, 0, 32);
          int_to_bytes(req->call_data->b.len - req->data_offset, req->call_data->b.data + p + 32 - 4);
          p += 32;

          // now add the dynamic data
          bytes32_t word;
          memset(word, 0, 32);
          int_to_bytes(b.len, word + 32 - 4);
          bb_check_size(req->call_data, word_size(b.len) + 32);
          bb_write_raw_bytes(req->call_data, word, 32);      // add the length to the tail
          bb_write_raw_bytes(req->call_data, b.data, b.len); // add the data

          int wl = word_size(b.len);
          memset(word, 0, 32);
          if (b.len < (uint32_t) wl)
            bb_write_raw_bytes(req->call_data, word, wl - b.len); // padd right
        }
      }
    }
  }
  return 0;
}
