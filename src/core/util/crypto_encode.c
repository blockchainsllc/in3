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
#include "../../third-party/crypto/bignum.h"

#ifdef WASM
#include <emscripten.h>
#endif

#ifdef BASE64
#include "../../third-party/libb64/cdecode.h"
#include "../../third-party/libb64/cencode.h"
#endif

in3_encoding_type_t d_encoding_from_string(char* enc, in3_encoding_type_t def) {
  if (!enc) return def;
  if (strcmp(enc, "hex") == 0) return ENC_HEX;
  if (strcmp(enc, "base58") == 0) return ENC_BASE58;
  if (strcmp(enc, "base64") == 0) return ENC_BASE64;
  if (strcmp(enc, "decimal") == 0) return ENC_DECIMAL;
  if (strcmp(enc, "utf8") == 0) return ENC_UTF8;
  return def;
}

int encode(in3_encoding_type_t type, bytes_t src, char* dst, size_t dst_len) {
  switch (type) {
    case ENC_UTF8:
      memcpy(dst, src.data, min(src.len, dst_len));
      dst[src.len] = 0;
      return src.len;
    case ENC_HEX: return bytes_to_hex(src.data, src.len, dst);
    case ENC_BASE58: {
#ifdef CRYPTO_TREZOR
      size_t size = src.len * 2 + 1;
      if (!b58enc(dst, &size, src.data, src.len) || size > src.len * 2) return -1;
      return (int) size;
#else
      return -1;
#endif
    }
    case ENC_BASE64: {
#ifdef BASE64
      char*  r = base64_encode(src.data, src.len);
      size_t s = _strnlen(r, src.len * 3);
      if (s > dst_len) r[dst_len] = 0; // make sure the string is not bigger than dst_len
      strcpy(dst, r);                  // NOSONAR - size is checked
      _free(r);
      return s;
#else
      return -1;
#endif
    }
    case ENC_DECIMAL: {
#ifdef CRYPTO_TREZOR
      bytes32_t val = {0};
      memcpy(val + 32 - src.len, src.data, src.len);
      bignum256 bn;
      bn_read_be(val, &bn);
      char   tmp[301];
      size_t l = bn_format(&bn, "", "", 0, 0, false, 0, tmp, 300);
      memcpy(dst, tmp, l);
      dst[l] = 0;
      return l;
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
    case ENC_DECIMAL: return src_len * 3;
    case ENC_UTF8: return src_len + 1;
    default: return src_len;
  }
}
int decode(in3_encoding_type_t type, const char* src, int src_len, uint8_t* dst) {
  if (src_len < 0) src_len = strlen(src);
  switch (type) {
    case ENC_UTF8: memcpy(dst, src, src_len); return src_len;
    case ENC_HEX: return hex_to_bytes(src, src_len, dst, src_len * 2);
    case ENC_BASE58: {
#ifdef CRYPTO_TREZOR
      size_t size = src_len;
      if (src[size]) {
        char* t = alloca(size + 1);
        memcpy(t, src, size);
        t[size] = 0;
        src     = t;
      }
      if (b58tobin(dst, &size, src))
        memmove(dst, dst + src_len - size, size);
      else
        return -1;
      return (int) size;
#else
      return -1;
#endif
    }
    case ENC_BASE64: {
#ifdef BASE64
      size_t len     = 0;
      char*  to_free = NULL;
      if (src && src[src_len] != 0) {
        if (src_len > 200) to_free = _malloc(src_len + 1);
        char* cpy = to_free ? to_free : alloca(src_len + 1);
        memcpy(cpy, src, src_len);
        cpy[src_len] = 0;
        src          = cpy;
      }
      uint8_t* r = base64_decode(src, &len);
      memcpy(dst, r, len);
      _free(r);
      _free(to_free);
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
    case ENC_UTF8: return src_len;
    default: return src_len;
  }
}

#ifdef WASM
EM_JS(void, wasm_random_buffer, (uint8_t * dst, size_t len), {
  // unload len
  function randomBytes() {
    var vals = new Uint8Array(len);
    try {
      return crypto.getRandomValues(vals);
    } catch (x) {
      try {
        return require('crypto').randomBytes(len);
      } catch (y) {
        const seed = Date.now();
        for (let i = 0; i < len; i++) vals[i] = seed[i % 32] ^ Math.floor(Math.random() * 256);
        return vals;
      }
    }
  }

  var res = randomBytes(len);
  for (var i = 0; i < len; i++) {
    HEAPU8[dst + i] = res[i];
  }
})
#endif
void random_buffer(uint8_t* dst, size_t len) {
#ifdef WASM
  wasm_random_buffer(dst, len);
  return;
#else
  FILE* r = fopen("/dev/urandom", "r");
  if (r) {
    for (size_t i = 0; i < len; i++) dst[i] = (uint8_t) fgetc(r);
    fclose(r);
    return;
  }
#endif
  srand(current_ms() % 0xFFFFFFFF);
#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__)
  unsigned int number;
  for (size_t i = 0; i < len; i++) dst[i] = (rand_s(&number) ? rand() : (int) number) % 256;
#else
  for (size_t i = 0; i < len; i++) dst[i] = rand() % 256;
#endif
}
