
#include "../../core/util/bitset.h"
#include "../../core/util/bytes.h"
#include "../../core/util/crypto.h"
#include "../../core/util/data.h"
#include "../../core/util/mem.h"
#include "../../core/util/stringbuilder.h"
#include "../../core/util/utils.h"
#include "abi.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static void abi_coder_free(abi_coder_t* c) {
  switch (c->type) {
    case ABI_TUPLE: {
      for (int i = 0; i < c->data.tuple.len; i++)
        abi_coder_free(c->data.tuple.components[i]);
      if (c->data.tuple.components) _free(c->data.tuple.components);
      break;
    }
    case ABI_ARRAY: {
      abi_coder_free(c->data.array.component);
      break;
    }
    default:
      break;
  }
  _free(c);
}
static inline abi_coder_t* abi_error(char** error, char* msg, abi_coder_t* coder) {
  if (coder) abi_coder_free(coder);
  *error = msg;
  return NULL;
}
static abi_coder_t* create_coder(char* token, char** error) {
  abi_coder_t* coder        = _calloc(1, sizeof(abi_coder_t));
  char*        start_number = NULL;
  if (strcmp(token, "address") == 0)
    coder->type = ABI_ADDRESS;
  else if (strcmp(token, "bool") == 0)
    coder->type = ABI_BOOL;
  else if (strcmp(token, "string") == 0)
    coder->type = ABI_STRING;
  else if (strncmp(token, "uint", 4) == 0) {
    coder->type             = ABI_NUMBER;
    coder->data.number.sign = false;
    start_number            = token + 4;
  }
  else if (strncmp(token, "int", 3) == 0) {
    coder->type             = ABI_NUMBER;
    coder->data.number.sign = true;
    start_number            = token + 3;
  }
  else if (strncmp(token, "fixed", 5) == 0 || strncmp(token, "ufixed", 6) == 0) {
    char tmp[4];
    coder->type             = ABI_NUMBER;
    coder->data.number.sign = *token == 'f';
    start_number            = token + (coder->data.number.sign ? 5 : 6);
    char* x                 = strchr(token, 'x');
#ifdef __clang_analyzer__
    *start_number = 0; // the analyser is able to understand, that start_number can not point to a garbage-value.
#endif
    if (!*start_number) {
      coder->data.number.n    = 18;
      coder->data.number.size = 128;
      start_number            = NULL;
    }
    else {
      if (!x || !(x - start_number) || (x - start_number) > 3) return abi_error(error, "invalid fixed type, must be (u)fixed<M<x<N>", coder);
      coder->data.number.size = strtol(strncpy(tmp, start_number, x - start_number), NULL, 10);
      start_number            = x + 1;
      if (coder->data.number.size % 8) return abi_error(error, "invalid number length", coder);
    }
  }
  else if (strncmp(token, "bytes", 5) == 0) {
    coder->type  = strlen(token) > 5 ? ABI_FIXED_BYTES : ABI_BYTES;
    start_number = token + 5;
  }
  else
    return abi_error(error, "invalid type", coder);

  if (start_number) {
    int i = strlen(start_number) ? atoi(start_number) : 256;
    if (coder->type == ABI_FIXED_BYTES)
      coder->data.fixed.len = i;
    else if (coder->type == ABI_NUMBER) {
      if (coder->data.number.size)
        coder->data.number.n = i;
      else if (i % 8)
        return abi_error(error, "invalid number length", coder);
      else
        coder->data.number.size = i;
    }
  }

  return coder;
}
static abi_coder_t* create_array(char* val, abi_coder_t* el, char** error, char** next) {
  if (*val == '[')
    val++;
  else
    return abi_error(error, "array must start with [", NULL);
  for (int i = 0; i < 10 && val[i]; i++) {
    if (val[i] <= '9' && val[i] >= '0') continue;
    if (val[i] == ']') {
      abi_coder_t* array          = _calloc(1, sizeof(abi_coder_t));
      array->data.array.component = el;
      array->type                 = ABI_ARRAY;
      if (i) {
        char* tmp = alloca(i + 1);
        memcpy(tmp, val, i);
        tmp[i]                = 0;
        array->data.array.len = atoi(tmp);
      }
      *next = val + i;
      return (*next)[1] == '[' ? create_array(*next + 1, array, error, next) : array;
    }
    return abi_error(error, "invalid character in array braces", NULL);
  }
  return abi_error(error, "missing end braces in array", NULL);
}

int abi_chars_len(char* s) {
  int c = 0;
  while ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') || (*s >= '0' && *s <= '9') || *s == '_') {
    s++;
    c++;
  }
  return c;
}

static abi_coder_t* create_tuple(char* val, char** error, char** next) {
  bool braces = *val == '(';
  if (braces) val++;
  char         token[100];
  int          tl    = 0;
  abi_coder_t* tuple = _calloc(1, sizeof(abi_coder_t));
  tuple->type        = ABI_TUPLE;
  bool indexed       = false;

  for (char c = *val; !*error; c = *(++val)) {
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
      if (tl == 100) return abi_error(error, "token too long", tuple);
      token[tl++] = c;
      continue;
    }
    if (c == ' ' && !tl) continue;
    if (c == ' ' && tl == 7 && strncmp(token, "indexed", 7) == 0) {
      tl      = 0;
      indexed = true;
      continue;
    }

    abi_coder_t* coder = NULL;
    if (tl) {
      token[tl] = 0;
      tl        = 0;
      coder     = create_coder(token, error);
    }
    else if (c == '(') {
      coder = create_tuple(val, error, &val);
      c     = *(++val);
    }
    else if (c != ',' && c != ' ' && c && c != ')')
      return abi_error(error, "invalid character", tuple);

    if (c == '[' && !*error) {
      if (!coder)
        *error = "invalid bracket without type";
      else {
        abi_coder_t* array = create_array(val, coder, error, &val);
        if (!*error)
          coder = array;
      }
    }
    if (*error) {
      if (coder) abi_coder_free(coder);
      return abi_error(error, *error, tuple);
    }

    if (coder) {
      while (*val == ' ') val++;
      int namelen = abi_chars_len(val);
      if (namelen) {
        coder->name = val;
        val += namelen;
        c = *val;
      }
      coder->indexed                                        = indexed;
      indexed                                               = false;
      tuple->data.tuple.components                          = tuple->data.tuple.len
                                                                  ? _realloc(tuple->data.tuple.components, (tuple->data.tuple.len + 1) * sizeof(abi_coder_t*), tuple->data.tuple.len * sizeof(abi_coder_t*))
                                                                  : _malloc(sizeof(abi_coder_t*));
      tuple->data.tuple.components[tuple->data.tuple.len++] = coder;
    }

    if (c == ')' || !c) {
      if (c == ')' && !braces) return abi_error(error, "closing braces without openinng ones", tuple);
      if (!c && braces) return abi_error(error, "missing closing braces", tuple);
      if (next) *next = val;
      return tuple;
    }
  }

  return tuple;
}

static sb_t* add_fn_sig(sb_t* sb, abi_coder_t* coder) {
  switch (coder->type) {
    case ABI_TUPLE: {
      sb_add_char(sb, '(');
      for (int i = 0; i < coder->data.tuple.len; i++) {
        if (i) sb_add_char(sb, ',');
        add_fn_sig(sb, coder->data.tuple.components[i]);
      }
      return sb_add_char(sb, ')');
    }
    case ABI_ADDRESS: return sb_add_chars(sb, "address");
    case ABI_BOOL: return sb_add_chars(sb, "bool");
    case ABI_STRING: return sb_add_chars(sb, "string");
    case ABI_BYTES: return sb_add_chars(sb, "bytes");
    case ABI_NUMBER: {
      if (coder->data.number.n)
        return sb_print(sb, "%s%ix%i", coder->data.number.sign ? "fixed" : "ufixed", coder->data.number.size, coder->data.number.n);
      else
        return sb_print(sb, "%s%i", coder->data.number.sign ? "int" : "uint", coder->data.number.size);
    }
    case ABI_FIXED_BYTES: {
      sb_add_chars(sb, "bytes");
      return sb_add_int(sb, coder->data.fixed.len);
    }
    case ABI_ARRAY: {
      add_fn_sig(sb, coder->data.array.component);
      sb_add_char(sb, '[');
      if (coder->data.array.len) sb_add_int(sb, coder->data.array.len);
      return sb_add_char(sb, ']');
    }
    default:
      break;
  }
  return sb;
}

static void create_fn_hash(char* fn_name, int fn_len, abi_coder_t* arguments, uint8_t* dst) {
  sb_t      sb = {0};
  bytes32_t hash;
  sb_add_range(&sb, fn_name, 0, fn_len);
  sb_add_char(&sb, '(');
  for (int i = 0; i < arguments->data.tuple.len; i++) {
    if (i) sb_add_char(&sb, ',');
    add_fn_sig(&sb, arguments->data.tuple.components[i]);
  }
  sb_add_char(&sb, ')');
  keccak(bytes((uint8_t*) sb.data, sb.len), hash);
  memcpy(dst, hash, 4);
  _free(sb.data);
}

void abi_sig_free(abi_sig_t* c) {
  if (c->input) abi_coder_free(c->input);
  if (c->output) abi_coder_free(c->output);
  _free(c);
}

char* find_end(char* str) {
  char* src = str;
  for (int level = 0; *str; str++) {
    if (*str == '(')
      level++;
    else if (*str == ')' && level == 1)
      return str;
    else if (*str == ')')
      level--;
  }
  return src;
}

abi_sig_t* abi_sig_create(char* signature, char** error) {
  *error            = NULL;
  char* input_start = strchr(signature, '(');
  if (!input_start) input_start = signature;
  char* input_end    = *input_start == '(' ? find_end(input_start) : signature;
  char* output_start = strchr(input_end, ':');
  output_start       = output_start && output_start[1] ? output_start + 1 : NULL;

  abi_sig_t* sig    = _calloc(1, sizeof(abi_sig_t));
  sig->input        = create_tuple(input_start, error, NULL);
  sig->output       = (output_start && !*error) ? create_tuple(output_start, error, NULL) : NULL;
  sig->return_tuple = (output_start && output_start[0] == '(') || (!sig->output && input_start && input_start[0] == '(');

  if (!*error && input_start != signature) create_fn_hash(signature, input_start - signature, sig->input, sig->fn_hash);

  if (*error) {
    abi_sig_free(sig);
    sig = NULL;
  }

  return sig;
}