#include "bytes.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "data.h"
#include "mem.h"
#include "stringbuilder.h"

#include "debug.h" // DEBUG !!!

// Here we check the pointer-size, because pointers smaller than 32bit may result in a undefined behavior, when calling d_to_bytes() for a T_INTEGER
#if UINTPTR_MAX == 0xFFFF
#error since we store a uint32_t in a pointer, pointers need to be at least 32bit!
#endif

typedef struct keyname {
  char*           name;
  d_key_t         key;
  struct keyname* next;
} keyname_t;
static uint8_t    __track_keys = 0;
static keyname_t* __keynames   = NULL;

d_key_t keyn(const char* c, const int len) {
  uint16_t val = 0;
  int      i   = 0;
  for (; i < len; i++) {
    if (*c == 0) return val;
    val ^= *c | val << 7;
    c += 1;
  }
  return val;
}

static d_key_t add_key(char* c, int len) {
  d_key_t k = keyn(c, len);
  if (!__track_keys) return k;
  keyname_t* kn = __keynames;
  while (kn) {
    if (kn->key == k) return k;
    kn = kn->next;
  }

  kn         = malloc(sizeof(keyname_t));
  kn->next   = __keynames;
  __keynames = kn;
  kn->key    = k;
  kn->name   = malloc(len + 1);
  memcpy(kn->name, c, len);
  kn->name[len] = 0;
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

int d_bytes_to(d_token_t* item, uint8_t* dst, const int max) {
  if (item) {
    int l = d_len(item), i, val;
    if (l > max) l = max;
    switch (d_type(item)) {
      case T_BYTES:
        if (max > l) {
          memset(dst, 0, max - l);
          dst += max - l;
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
uint32_t d_int(const d_token_t* item) {
  return d_intd(item, 0);
}
uint32_t d_intd(const d_token_t* item, const uint32_t def_val) {
  if (item == NULL) return def_val;
  switch (d_type(item)) {
    case T_INTEGER:
    case T_BOOLEAN:
      return item->len & 0xFFFFFFF;
    case T_BYTES: {
      if (item->len == 0) return 0;
      return bytes_to_int(item->data, min(4, item->len));
    }
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
    return item->len & 0xFFFFFFF;
  else if (d_type(item) == T_BYTES)
    return bytes_to_long(item->data, item->len);
  return def_val;
}

bool d_eq(const d_token_t* a, const d_token_t* b) {
  if (a == NULL || b == NULL) return false;
  if (a->len != b->len) return false;
  if (a->data && b->data)
    return b_cmp(d_bytes(a), d_bytes(b));
  return a->data == NULL && b->data == NULL;
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
  char* start = jp->c;
  int   r;
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
  char temp[20];
  int  i = 0;
  jp->c--;
  for (; i < 20; i++) {
    if (jp->c[i] >= '0' && jp->c[i] <= '9')
      temp[i] = jp->c[i];
    else {
      temp[i] = 0;
      jp->c += i;

      int res = atoi(temp); // modified for integers that doesn't fit in 28 bits
      if (res & 0XF0000000) {
        item->data = _malloc(4);
        item->len  = 4;
        int_to_bytes(res, item->data);
      } else
        item->len |= res;

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
      case '"':
        l = jp->c - start - 1;
        if (l > 1 && *start == '0' && start[1] == 'x') {
          // this is a hex-value
          if (l == 2) {
            // empty byte array
            item->len  = 0;
            item->data = NULL;
          } else if (l < 10 && !(l > 3 && start[2] == '0' && start[3] == '0')) { // we can accept up to 3,4 bytes as integer
            item->len = T_INTEGER << 28;
            for (i = 2; i < l; i++)
              item->len |= strtohex(start[i]) << ((l - i - 1) << 2);
          } else {
            // we need to allocate bytes for it. and so set the type to bytes
            item->len  = ((l & 1) ? l - 1 : l - 2) >> 1;
            item->data = _malloc(item->len);
            if (l & 1) item->data[0] = strtohex(start[2]);
            l = (l & 1) + 2;
            for (i = l - 2, n = l; i < item->len; i++, n += 2)
              item->data[i] = strtohex(start[n]) << 4 | strtohex(start[n + 1]);
          }
        } else if (l == 6 && *start == '\\' && start[1] == 'u') {
          item->len   = 1;
          item->data  = _malloc(1);
          *item->data = strtohex(start[4]) << 4 | strtohex(start[5]);
        } else {
          item->len  = l | T_STRING << 28;
          item->data = (uint8_t*) start;
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

  switch (next_char(jp)) {
    case 0: return -2;
    case '{':
      parsed_next_item(jp, T_OBJECT, key, parent)->data = (uint8_t*) jp->c - 1;
      while (true) {
        switch (next_char(jp)) {
          case '"':
            res = parse_key(jp);
            if (res < 0) return res;
            break;
          case '}': return 0;
          default: return -2; // invalid character or end
        }
        res = parse_object(jp, p_index, res); // parse the value
        if (res < 0) return res;
        switch (next_char(jp)) {
          case ',': break;    // we continue reading the next property
          case '}': return 0; // this was the last property, so we return successfully.
          default: return -2; // unexpected character, throw.
        }
      }
    case '[':
      parsed_next_item(jp, T_ARRAY, key, parent)->data = (uint8_t*) jp->c - 1;
      if (next_char(jp) == ']') return 0;
      jp->c--;

      while (true) {
        res = parse_object(jp, p_index, jp->result[p_index].len & 0xFFFFFF); // parse the value
        if (res < 0) return res;
        switch (next_char(jp)) {
          case ',': break;    // we continue reading the next property
          case ']': return 0; // this was the last element, so we return successfully.
          default: return -2; // unexpected character, throw.
        }
      }
    case '"':
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
      return parse_number(jp, parsed_next_item(jp, T_INTEGER, key, parent));
    default:
      return -2;
  }
}

void free_json(json_ctx_t* jp) {
  if (!jp || jp->result == NULL) return;
  if (jp->allocated) {
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
  json_ctx_t* parser = _malloc(sizeof(json_ctx_t));
  parser->len        = 0;
  parser->result     = _malloc(sizeof(d_token_t) * 10);
  parser->c          = js;
  parser->allocated  = 10;
  int res            = parse_object(parser, -1, 0);
  if (res < 0) {
    free_json(parser);
    return NULL;
  }
  parser->c = js;
  return parser;
}

static int find_end(char* str) {
  int   l = 0;
  char* c = str;
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
      sprintf(dst, "0x%x", d_int(item));
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

d_token_t* d_clone(d_token_t* item) {
  d_token_t* t = _malloc(sizeof(*t));
  if (t == NULL) return NULL;
  size_t len = find_end(item->data);
  t->data    = _malloc(len);
  if (t->data == NULL) {
    _free(t);
    return NULL;
  }
  memcpy(t->data, item->data, len);
  t->key = item->key;
  t->len = item->len;
  return t;
}

str_range_t d_to_json(d_token_t* item) {
  str_range_t s;
  s.data = (char*) item->data;
  s.len  = find_end(s.data);
  return s;
}

// util fast parse
int json_get_int_value(char* js, char* prop) {
  json_ctx_t* ctx = parse_json(js);
  if (ctx) {
    int res = d_get_int(ctx->result, prop);
    free_json(ctx);
    return res;
  }
  return -1;
}

void json_get_str_value(char* js, char* prop, char* dst) {
  *dst          = 0; // preset returned string as empty string
  d_token_t*  t = NULL;
  str_range_t s;

  json_ctx_t* ctx = parse_json(js);
  if (ctx) {
    t = d_get(ctx->result, key(prop));
    switch (d_type(t)) {
      case T_STRING:
        strcpy(dst, d_string(t));
        break;
      case T_BYTES:
        dst[0] = '0';
        dst[1] = 'x';
        bytes_to_hex(t->data, t->len, dst + 2);
        dst[t->len * 2 + 2] = 0;
        break;
      case T_ARRAY:
      case T_OBJECT:
        s = d_to_json(t);
        memcpy(dst, s.data, s.len);
        dst[s.len] = 0;
        break;
      case T_BOOLEAN:
        strcpy(dst, d_int(t) ? "true" : "false");
        break;
      case T_INTEGER:
        sprintf(dst, "0x%x", d_int(t));
        break;
      case T_NULL:
        strcpy(dst, "null");
    }
    free_json(ctx);
  }
}

char* json_get_json_value(char* js, char* prop) {
  json_ctx_t* ctx = parse_json(js);
  if (ctx) {
    char* c = d_create_json(d_get(ctx->result, key(prop)));
    free_json(ctx);
    return c;
  }
  return NULL;
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

static int read_token(json_ctx_t* jp, uint8_t* d, size_t* p) {
  uint16_t key;
  d_type_t type = d[*p] >> 5; // first 3 bits define the type

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
    uint32_t idx = len - 1;
    if (jp->len < idx) return -1;
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
      t->data = d + ((*p)++);
      if (t->data[len] != 0) return 1;
      *p += len;
      break;
    case T_BYTES:
      t->data = d + (*p);
      *p += len;
      break;
    default:
      break;
  }
  return 0;
}

json_ctx_t* parse_binary_str(char* data, int len) {
  bytes_t b = {.data = (uint8_t*) data, .len = len};
  return parse_binary(&b);
}

json_ctx_t* parse_binary(bytes_t* data) {
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
  __track_keys = v;
}
void d_clear_keynames() {
  keyname_t* kn = NULL;
  while (__keynames) {
    kn = __keynames;
    free(kn->name);
    __keynames = kn->next;
    free(kn);
  }
}
