
#include "../../core/util/bitset.h"
#include "../../core/util/bytes.h"
#include "../../core/util/data.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "abi.h"
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static in3_ret_t encode_tuple(abi_coder_t* tuple, d_token_t* src, bytes_builder_t* bb, char** error);

bool abi_is_dynamic(abi_coder_t* coder) {
  switch (coder->type) {
    case ABI_ADDRESS:
    case ABI_FIXED_BYTES:
    case ABI_NUMBER:
    case ABI_BOOL:
      return false;
    case ABI_STRING:
    case ABI_BYTES:
      return true;
    case ABI_ARRAY:
      return coder->data.array.len == 0 || abi_is_dynamic(coder->data.array.component);
    case ABI_TUPLE: {
      for (int i = 0; i < coder->data.tuple.len; i++) {
        if (abi_is_dynamic(coder->data.tuple.components[i])) return true;
      }
      return false;
    }
  }
  return false;
}

static in3_ret_t encode_error(char* msg, char** error) {
  *error = msg;
  return IN3_EINVAL;
}
static in3_ret_t encode_value(abi_coder_t* coder, d_token_t* src, bytes_builder_t* bb, char** error) {
  bytes32_t b = {0};
  switch (coder->type) {
    case ABI_ADDRESS: {
      bytes_t data = d_to_bytes(src);
      if (data.len != 20) return encode_error("Invalid address-length", error);
      memcpy(b + 12, data.data, 20);
      break;
    }
    case ABI_BOOL: {
      if (d_type(src) != T_BOOLEAN && d_type(src) != T_INTEGER) return encode_error("invalid boolean-value", error);
      b[31] = d_int(src);
      break;
    }
    case ABI_FIXED_BYTES: {
      bytes_t data = d_to_bytes(src);
      if (data.len != (unsigned int) coder->data.fixed.len) return encode_error("Invalid bytes-length", error);
      memcpy(b, data.data, data.len);
      break;
    }
    case ABI_NUMBER: {
      uint8_t filler = 0;
      bytes_t data;
      if (d_type(src) == T_STRING) {
        uint8_t*     tmp = alloca(32);
        char*        val = d_string(src);
        unsigned int bl  = coder->data.number.size / 8;

        if (strlen(val) > 18) return encode_error("invalid number string (too big)", error);
        errno = 0;
        if (coder->data.number.sign) {
          int64_t n = strtoll(val, NULL, 10);
          if (n < 0) filler = 0xff;
          memset(tmp, filler, 32);
          long_to_bytes((uint64_t) n, tmp + 24);
        }
        else {
          memset(tmp, 0, 32);
          uint64_t n = strtoull(val, NULL, 10);
          long_to_bytes(n, tmp + 24);
        }
        if (errno) return encode_error("invalid number value", error);
        data = bytes(tmp + 32 - bl, bl);
      }
      else
        data = d_to_bytes(src);
      if (data.len > (uint32_t) coder->data.number.size / 8) {
        if (coder->data.number.sign && data.data[data.len - coder->data.number.size / 8 - 1] == 0xff)
          data = bytes(data.data + data.len - coder->data.number.size / 8, coder->data.number.size / 8);
        else
          return encode_error("number too big", error);
      }
      memcpy(b + 32 - data.len, data.data, data.len);
      if (coder->data.number.sign && data.len < 32) memset(b, filler, 32 - data.len);
      break;
    }
    case ABI_TUPLE:
      return encode_tuple(coder, src, bb, error);
    case ABI_STRING:
    case ABI_BYTES: {
      if (d_type(src) != T_STRING && d_type(src) != T_BYTES) return encode_error("invalid bytes or string value", error);
      bytes_t data = d_to_bytes(src);
      int_to_bytes(data.len, b + 28);
      bb_write_raw_bytes(bb, b, 32);
      for (int i = 0; i < (int) data.len; i += 32) {
        memset(b, 0, 32);
        memcpy(b, data.data + i, min(32, data.len - i));
        bb_write_raw_bytes(bb, b, 32);
      }
      return IN3_OK;
    }
    case ABI_ARRAY: {
      if (d_type(src) != T_ARRAY) return encode_error("must be an array", error);
      int len = coder->data.array.len ? coder->data.array.len : d_len(src);
      if (d_len(src) != len) return encode_error("invalid array length", error);
      if (!coder->data.array.len) {
        int_to_bytes(len, b + 28);
        bb_write_raw_bytes(bb, b, 32);
      }
      bool is_dynamic = abi_is_dynamic(coder->data.array.component);
      int  static_pos = bb->b.len;
      if (is_dynamic) {
        bb_check_size(bb, 32 * len);
        bb->b.len += 32 * len;
      }
      for (int i = 0; i < len; i++) {
        if (is_dynamic) {
          memset(bb->b.data + static_pos + i * 32, 0, 32);
          int_to_bytes(bb->b.len - static_pos, bb->b.data + static_pos + i * 32 + 28);
        }
        TRY(encode_value(coder->data.array.component, d_get_at(src, i), bb, error))
      }
      return IN3_OK;
    }
  }

  bb_write_raw_bytes(bb, b, 32);

  return IN3_OK;
}

static in3_ret_t encode_tuple(abi_coder_t* tuple, d_token_t* src, bytes_builder_t* bb, char** error) {
  int len = d_type(src) == T_ARRAY ? d_len(src) : 1;
  if (len != tuple->data.tuple.len) {
    *error = "Invalid tuple length";
    return IN3_EINVAL;
  }
  in3_ret_t       res;
  bytes_builder_t b_dynamic = {0};
  bytes_builder_t b_static  = {0};
  int*            updates   = alloca(sizeof(int) * len);
  for (int i = 0; i < len; i++) {
    updates[i]      = -1;
    d_token_t*   el = d_type(src) == T_ARRAY ? d_get_at(src, i) : src;
    abi_coder_t* c  = tuple->data.tuple.components[i];
    if (abi_is_dynamic(c)) {
      int offset = b_dynamic.b.len;
      TRY_GOTO(encode_value(c, el, &b_dynamic, error))
      bytes32_t tmp = {0};
      int_to_bytes(offset, tmp + 28);
      updates[i] = b_static.b.len;
      bb_write_raw_bytes(&b_static, tmp, 32);
    }
    else {
      TRY_GOTO(encode_value(c, el, &b_static, error))
    }
  }

  for (int i = 0; i < len; i++) {
    if (updates[i] != -1) int_to_bytes(bytes_to_int(b_static.b.data + 28 + updates[i], 4) + b_static.b.len, b_static.b.data + 28 + updates[i]);
  }

  bb_write_fixed_bytes(bb, &b_static.b);
  bb_write_fixed_bytes(bb, &b_dynamic.b);

clean:
  if (b_dynamic.b.data) _free(b_dynamic.b.data);
  if (b_static.b.data) _free(b_static.b.data);
  return *error ? IN3_EINVAL : IN3_OK;
}

bytes_t abi_encode(abi_sig_t* s, d_token_t* src, char** error) {
  bytes_builder_t bb = {0};
  if (!memiszero(s->fn_hash, 4)) bb_write_raw_bytes(&bb, s->fn_hash, 4);
  if (encode_tuple(s->input, src, &bb, error) && bb.b.data)
    _free(bb.b.data);
  return *error ? bytes(NULL, 0) : bb.b;
}
