
#include "../../core/util/bitset.h"
#include "../../core/util/bytes.h"
#include "../../core/util/data.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "abi.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static in3_ret_t decode_tuple(abi_coder_t* tuple, bytes_t data, json_ctx_t* res, int* data_read, bool as_array, char** error);

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
      break;
    }
    case ABI_NUMBER: {
      TRY(next_word(&pos, &data, &word, error))
      int b = c->data.number.size / 8;
      if (b <= 8) {
        if (c->data.number.sign && (word[32 - b] & 0x80)) {            // we have a negative number, which we need to convert to a string
          memset(word, 0xff, 32 - b);                                  // fill all bytes with ff so we can use uint64_t
          int64_t val = (int64_t) bytes_to_long(word + 24, 8);         // take the value and convert to a signed
          char    tmp[24];                                             //
          json_create_string(res, tmp, sprintf(tmp, "%" PRId64, val)); //format the signed as string
        }
        else
          json_create_int(res, bytes_to_long(word + 24, 8));
      }
      else {
        bytes_t r = bytes(word + 32 - b, b);
        b_optimize_len(&r);
        json_create_bytes(res, r);
      }
      break;
    }
    case ABI_TUPLE:
      return decode_tuple(c, data, res, data_read, true, error);
    case ABI_ARRAY: {
      int len = c->data.array.len;
      if (!len) {
        TRY(next_word(&pos, &data, &word, error)) // is is a dynamic array, so we need to
        len = bytes_to_int(word + 28, 4);         // read the length
      }
      bool is_dynamic = abi_is_dynamic(c->data.array.component);
      int  offset     = pos;
      json_create_array(res)->len |= len;
      for (int i = 0; i < len; i++) {
        int r = 0, start = pos;
        if (is_dynamic) {
          TRY(next_word(&pos, &data, &word, error))
          start = offset + bytes_to_int(word + 28, 4);
        }
        TRY(decode_value(c->data.array.component, bytes(data.data + start, data.len - start), res, &r, error))
        if (!is_dynamic) pos += r;
      }
    }
  }

  if (data_read) *data_read = pos;
  return IN3_OK;
}

static in3_ret_t decode_tuple(abi_coder_t* tuple, bytes_t data, json_ctx_t* res, int* data_read, bool add_array, char** error) {
  if (add_array) json_create_array(res)->len |= tuple->data.tuple.len;
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
  json_ctx_t*  res    = json_create();
  abi_coder_t* c      = s->output ? s->output : s->input;
  in3_ret_t    failed = decode_tuple(c, data, res, NULL, s->return_tuple || c->data.tuple.len != 1, error);
  if (failed && res) json_free(res);
  return *error ? NULL : res;
}
