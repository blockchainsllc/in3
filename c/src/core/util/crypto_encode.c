#include "bytes.h"
#include "crypto.h"
#include "debug.h"
#include "mem.h"
#include "utils.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../third-party/crypto/base58.h"
#ifdef BASE64
#include "../../third-party/libb64/cdecode.h"
#include "../../third-party/libb64/cencode.h"
#endif

int encode(in3_encoding_type_t type, bytes_t src, char* dst) {
  switch (type) {
    case ENC_HEX: return bytes_to_hex(src.data, src.len, dst);
    case ENC_BASE58: {
      size_t size = src.len * 2 + 1;
      if (!b58enc(dst, &size, src.data, src.len) || size > src.len * 2) return -1;
      return (int) size;
    }
    case ENC_BASE64: {
#ifdef BASE64
      char* r = base64_encode(src.data, src.len);
      int   s = sprintf(dst, "%s", r);
      _free(r);
      return s;
#else
      return -1;
#endif
    }
    default: return IN3_ENOTSUP;
  }
}
int encode_size(in3_encoding_type_t type, int src_len) {
  switch (type) {
    case ENC_HEX: return src_len * 2;
    case ENC_BASE58: return src_len * 2;
    case ENC_BASE64: return src_len * 2;
    default: return src_len;
  }
}
int decode(in3_encoding_type_t type, const char* src, int src_len, uint8_t* dst) {
  if (src_len < 0) src_len = strlen(src);
  switch (type) {
    case ENC_HEX: return hex_to_bytes(src, src_len, dst, src_len * 2);
    case ENC_BASE58: {
      size_t size = src_len;
      if (b58tobin(dst, &size, src))
        memmove(dst, dst + src_len - size, size);
      else
        return -1;
      return (int) size;
    }
    case ENC_BASE64: {
#ifdef BASE64
      size_t   len = 0;
      uint8_t* r   = base64_decode(src, &len);
      memcpy(dst, r, len);
      _free(r);
      return (int) len;
#else
      return -1;
#endif
    }
    default: return (int) IN3_ENOTSUP;
  }
}
int decode_size(in3_encoding_type_t type, int src_len) {
  switch (type) {
    case ENC_HEX: return (src_len + 1) / 2;
    case ENC_BASE58: return src_len;
    case ENC_BASE64: return src_len;
    default: return src_len;
  }
}
