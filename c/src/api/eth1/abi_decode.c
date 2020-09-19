
#include "../../core/util/bitset.h"
#include "../../core/util/bytes.h"
#include "../../core/util/data.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "abi2.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static in3_ret_t decode_tuple(abi_coder_t* tuple, bytes_t data, json_ctx_t* res, int* data_read, char** error);

static in3_ret_t next_word(int* offset, bytes_t* data, uint8_t** dst, char** error) {
  if (*offset + 32 > (int) data->len) {
    *error = "reached end of data";
    return IN3_EINVAL;
  }
  *dst = data->data + *offset;
  *offset += 32;
  return IN3_OK;
}

static in3_ret_t decode_value(abi_coder_t* c, bytes_t data, json_ctx_t* res, int* data_read, char** error) {
  uint8_t* word;
  int      pos = 0;
  switch (c->type) {
    case ABI_ADDRESS: {
      TRY(next_word(&pos, &data, &word, error))
      json_create_bytes(res, bytes(word + 12, 20));
      break;
    }
    case ABI_FIXED_BYTES: {
      TRY(next_word(&pos, &data, &word, error))
      json_create_bytes(res, bytes(word, c->data.fixed.len));
      break;
    }
    case ABI_BOOL: {
      TRY(next_word(&pos, &data, &word, error))
      json_create_bool(res, word[31]);
      break;
    }
    case ABI_STRING:
    case ABI_BYTES: {
      TRY(next_word(&pos, &data, &word, error))
      int len = bytes_to_int(word + 28, 4);
      int wl  = ((len + 31) / 32) * 32;
      if ((int) data.len < wl + pos) {
        *error = "out of data when reading bytes";
        return IN3_EINVAL;
      }
      c->type == ABI_STRING
          ? json_create_string(res, (char*) word + 32, len)
          : json_create_bytes(res, bytes(word + 32, len));
      pos += wl;
      break;
    }
    case ABI_NUMBER: {
      TRY(next_word(&pos, &data, &word, error))
      if (c->data.number.size <= 64)
        json_create_int(res, bytes_to_long(word + 24, 8));
      else
        json_create_bytes(res, bytes(word + 32 - c->data.number.size / 8, c->data.number.size / 8));
      break;
    }
    case ABI_TUPLE:
      return decode_tuple(c, data, res, data_read, error);
    case ABI_ARRAY: {
      int len = c->data.array.len;
      if (!len) {
        TRY(next_word(&pos, &data, &word, error))
        len = bytes_to_int(word + 28, 4);
      }
      json_create_array(res)->len |= len;
      for (int i = 0; i < len; i++) {
        int r = 0;
        if (data.len < pos + 32) {
          *error = "out of data when reading array";
          return IN3_EINVAL;
        }
        TRY(decode_value(c->data.array.component, bytes(data.data + pos, data.len - pos), res, &r, error))
        pos += r;
      }
    }
  }

  if (data_read) *data_read = pos;
  return IN3_OK;
}

static in3_ret_t decode_tuple(abi_coder_t* tuple, bytes_t data, json_ctx_t* res, int* data_read, char** error) {
  d_token_t* array = json_create_array(res);
  array->len |= tuple->data.tuple.len;
  uint8_t* word = NULL;
  int      pos  = 0;
  for (int i = 0; i < tuple->data.tuple.len; i++) {
    abi_coder_t* c = tuple->data.tuple.components[i];
    if (abi_is_dynamic(c)) {
      TRY(next_word(&pos, &data, &word, error))
      int offset = bytes_to_int(word + 28, 4);
      if (offset + 32 > (int) data.len) {
        *error = "invalid offset";
        return IN3_EINVAL;
      }
      TRY(decode_value(c, bytes(data.data + offset, data.len - offset), res, NULL, error))
    }
    else {
      int len = 0;
      if (pos + 32 > (int) data.len) {
        *error = "end of data";
        return IN3_EINVAL;
      }
      TRY(decode_value(c, bytes(data.data + pos, data.len - pos), res, &len, error))
      pos += len;
    }
  }
  if (data_read) *data_read = pos;

  return *error ? IN3_EINVAL : IN3_OK;
}

json_ctx_t* abi_decode(abi_sig_t* s, bytes_t data, char** error) {
  json_ctx_t* res = json_create();
  if (decode_tuple(s->output ? s->output : s->input, data, res, NULL, error))
    json_free(res);
  return *error ? NULL : res;
}
