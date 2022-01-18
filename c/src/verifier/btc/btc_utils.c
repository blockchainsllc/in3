#include "btc_utils.h"

// copy a byte array in reverse order
in3_ret_t rev_memcpy(uint8_t* dst, uint8_t* src, uint32_t len) {
  if (src && dst) {
    for (uint32_t i = 0; i < len; i++) {
      dst[(len - 1) - i] = src[i];
    }
    return IN3_OK;
  }
  else {
    return IN3_EINVAL;
  }
}

in3_ret_t append_bytes(bytes_t* dst, const bytes_t* src) {
  if (dst && src && src->len > 0 && src->data) {
    dst->data = (dst->data) ? _realloc(dst->data, dst->len + src->len, dst->len) : _malloc(dst->len + src->len);
    memcpy(dst->data + dst->len, src->data, src->len);
    dst->len += src->len;
    return IN3_OK;
  }
  else {
    return IN3_EINVAL;
  }
}