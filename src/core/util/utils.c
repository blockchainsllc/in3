#include "utils.h"
#include "../crypto/sha3.h"
#include "bytes.h"
#include "debug.h"
#include "mem.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int hash_cmp(uint8_t* a, uint8_t* b) {
  int len = 31;

  while (a[len] == b[len] && len--)
    ;

  return ++len;
}

int size_of_bytes(int str_len) {
  int out_len = (str_len & 1) ? (str_len + 1) / 2 : str_len / 2;
  return out_len;
}
void long_to_bytes(uint64_t val, uint8_t* dst) {
  *dst       = val >> 56 & 0xFF;
  *(dst + 1) = val >> 48 & 0xFF;
  *(dst + 2) = val >> 40 & 0xFF;
  *(dst + 3) = val >> 32 & 0xFF;
  *(dst + 4) = val >> 24 & 0xFF;
  *(dst + 5) = val >> 16 & 0xFF;
  *(dst + 6) = val >> 8 & 0xFF;
  *(dst + 7) = val & 0xFF;
}
void int_to_bytes(uint32_t val, uint8_t* dst) {
  *dst       = val >> 24 & 0xFF;
  *(dst + 1) = val >> 16 & 0xFF;
  *(dst + 2) = val >> 8 & 0xFF;
  *(dst + 3) = val & 0xFF;
}

uint8_t strtohex(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return 255;
}

int hex2byte_arr(char* buf, int len, uint8_t* out, int outbuf_size) {
  if (len == -1) {
    len = strlen(buf);
    if (*buf == '0' && buf[1] == 'x') {
      buf += 2;
      len -= 2;
    }
  }
  int i       = len - 1;
  int out_len = (len & 1) ? (len + 1) / 2 : len / 2;
  int j       = out_len - 1;

  if (j > outbuf_size)
    return -1; /* Output buffer is smaller than need */

  while (i >= 0) {
    out[j] = strtohex(buf[i--]);
    if (i >= 0) {
      out[j--] |= strtohex(buf[i--]) << 4;
    }
  }

  return out_len;
}
bytes_t* hex2byte_new_bytes(char* buf, int len) {
  int bytes_len = (len & 1) ? (len + 1) / 2 : len / 2;

  uint8_t* b     = _malloc(bytes_len);
  bytes_t* bytes = _malloc(sizeof(bytes_t));
  hex2byte_arr(buf, len, b, bytes_len);
  bytes->data = b;
  bytes->len  = bytes_len;
  return bytes;
}

void int8_to_char(uint8_t* buffer, int len, char* out) {
  const char hex[] = "0123456789abcdef";
  int        i = 0, j = 0;
  while (j < len) {
    out[i++] = hex[(buffer[j] >> 4) & 0xF];
    out[i++] = hex[buffer[j] & 0xF];
    j++;
  }
  out[i] = '\0';
}

int sha3_to(bytes_t* data, void* dst) {
  if (data == NULL) return -1;
  struct SHA3_CTX ctx;
  sha3_256_Init(&ctx);
  sha3_Update(&ctx, data->data, data->len);
  keccak_Final(&ctx, dst);
  return 0;
}

bytes_t* sha3(bytes_t* data) {
  bytes_t*        out;
  struct SHA3_CTX ctx;

  out = _calloc(1, sizeof(bytes_t));

  sha3_256_Init(&ctx);
  sha3_Update(&ctx, data->data, data->len);

  out->data = _calloc(1, 32 * sizeof(uint8_t));
  out->len  = 32;

  keccak_Final(&ctx, out->data);
  return out;
}

uint64_t bytes_to_long(uint8_t* data, int len) {
  uint64_t res = 0;
  int      i;
  for (i = 0; i < len; i++) {
    if (data[i])
      res |= ((uint64_t) data[i]) << (len - i - 1) * 8;
  }
  return res;
}
uint64_t c_to_long(char* a, int l) {
  if (a[0] == '0' && a[1] == 'x') {
    long val = 0;
    for (int i = l - 1; i > 1; i--)
      val |= ((uint64_t) strtohex(a[i])) << (4 * (l - 1 - i));
    return val;
  } else if (l < 12) {
    char temp[12];
    strncpy(temp, a, l);
    temp[l] = 0;
    return atoi(temp);
  }
  return -1;
}

char* _strdup(char* src, int len) {
  if (len < 0) len = strlen(src);
  char* dst = _malloc(len + 1);
  strncpy(dst, src, len);
  dst[len] = 0;
  return dst;
}
int min_bytes_len(uint64_t val) {
  int i;
  for (i = 0; i < 8; i++, val >>= 8) {
    if (val == 0) return i;
  }
  return 8;
}
