#include "btc_serialize.h"
#include "../../core/util/crypto.h"
#include "../../core/util/data.h"
#include "../../core/util/utils.h"
#include "../../third-party/tommath/tommath.h"
#include "btc_types.h"
#include <string.h>

void btc_hash(bytes_t data, bytes32_t dst) {
  bytes32_t    tmp;
  in3_digest_t ctx = crypto_create_hash(DIGEST_SHA256_BTC);
  crypto_update_hash(ctx, data);
  crypto_finalize_hash(ctx, tmp);
  rev_copy(dst, tmp);
}

void btc_hash256(bytes_t data, bytes32_t dst) {
  bytes32_t    tmp;
  in3_digest_t ctx = crypto_create_hash(DIGEST_SHA256);
  crypto_update_hash(ctx, data);
  crypto_finalize_hash(ctx, tmp);
  memcpy(dst, tmp, 32);
}

void btc_hash160(bytes_t data, address_t dst) {
  address_t    tmp;
  in3_digest_t ctx = crypto_create_hash(DIGEST_RIPEMD_160);
  crypto_update_hash(ctx, data);
  crypto_finalize_hash(ctx, tmp);
  memcpy(dst, tmp, 20);
}

static void rev_hex(char* hex, uint8_t* dst, int l) {
  if (!hex) return;
  int len     = strlen(hex); // NOSONAR - this function expects null-terminated string which was checked prior to calling it
  int out_len = (len + 1) >> 1;
  int i, j;
  if (out_len > l)
    out_len = l;
  else if (out_len < l)
    memset(dst + out_len, 0, l - out_len);

  if (len % 1) {
    dst[out_len - 1] = hexchar_to_int(*hex);
    out_len--;
    hex++;
  }

  for (i = 0, j = out_len - 1; j >= 0; j--, i += 2)
    dst[j] = (hexchar_to_int(hex[i]) << 4) | hexchar_to_int(hex[i + 1]);
}

bytes_t btc_block_get(bytes_t block, btc_block_field field) {
  switch (field) {
    case BTC_B_VERSION: return bytes(block.data, 4);
    case BTC_B_PARENT_HASH: return bytes(block.data + 4, 32);
    case BTC_B_MERKLE_ROOT: return bytes(block.data + 36, 32);
    case BTC_B_TIMESTAMP: return bytes(block.data + 68, 4);
    case BTC_B_BITS: return bytes(block.data + 72, 4);
    case BTC_B_NONCE: return bytes(block.data + 76, 4);
    case BTC_B_HEADER: return bytes(block.data, 80);
    default: return NULL_BYTES;
  }
}

// copy 32 bytes in revers order
void rev_copy(uint8_t* dst, uint8_t* src) {
  rev_copyl(dst, bytes(src, 32), 32);
}

void rev_copyl(uint8_t* dst, bytes_t src, int l) {
  if (src.len < (uint32_t) l) {
    memset(dst + src.len, 0, l - src.len);
    l = src.len;
  }
  for (int i = 0; i < l; i++) dst[l - 1 - i] = src.data[i];
}

uint32_t le_to_int(uint8_t* data) {
  return (((uint32_t) data[3]) << 24) | (((uint32_t) data[2]) << 16) | (((uint32_t) data[1]) << 8) | data[0];
}

uint64_t le_to_long(uint8_t* data) {
  return (((uint64_t) data[7]) << 24) | (((uint64_t) data[6]) << 24) | (((uint64_t) data[5]) << 24) | (((uint64_t) data[4]) << 24) |
         (((uint64_t) data[3]) << 24) | (((uint64_t) data[2]) << 16) | (((uint64_t) data[1]) << 8) | data[0];
}

// TODO: Receive 'buf' as type 'uint8_t*' instead of 'bytes_t*'
void uint_to_le(bytes_t* buf, uint32_t index, uint32_t value) {
  buf->data[index]     = value & 0xff;
  buf->data[index + 1] = (value >> 8) & 0xff;
  buf->data[index + 2] = (value >> 16) & 0xff;
  buf->data[index + 3] = (value >> 24) & 0xff;
}

// TODO: Receive 'buf' as type 'uint8_t*' instead of 'bytes_t*'
void long_to_le(bytes_t* buf, uint32_t index, uint64_t value) {
  buf->data[index]     = value & 0xff;
  buf->data[index + 1] = (value >> 8) & 0xff;
  buf->data[index + 2] = (value >> 16) & 0xff;
  buf->data[index + 3] = (value >> 24) & 0xff;
  buf->data[index + 4] = (value >> 32) & 0xff;
  buf->data[index + 5] = (value >> 40) & 0xff;
  buf->data[index + 6] = (value >> 48) & 0xff;
  buf->data[index + 7] = (value >> 56) & 0xff;
}

size_t get_compact_uint_size(uint64_t cmpt_uint) {
  if (cmpt_uint > 0xffffffff) {
    return 9;
  }
  if (cmpt_uint > 0xffff) {
    return 5;
  }
  if (cmpt_uint > 0xfc) {
    return 3;
  }
  return 1;
}

// TODO: Receive 'buf' as type 'uint8_t*' instead of 'bytes_t*'
void long_to_compact_uint(bytes_t* buf, uint32_t index, uint64_t value) {
  int len;
  if (value > 0xffffffff) {
    len              = 9;
    buf->data[index] = 0xff;
  }
  else if (value > 0xffff) {
    len              = 5;
    buf->data[index] = 0xfe;
  }
  else if (value > 0xfc) {
    len              = 3;
    buf->data[index] = 0xfd;
  }
  else {
    len              = 1;
    buf->data[index] = (uint8_t) (value & 0xff);
  }

  // fill buffer with value
  if (len > 1) {
    for (int i = 0; i < (len - 1); i++) {
      buf->data[i + index] = (uint8_t) ((value >> (i << 1)) & 0xff);
    }
  }
}

void btc_target_from_block(bytes_t block, bytes32_t target) {
  uint8_t *bits = btc_block_get(block, BTC_B_BITS).data, tmp[32];
  memset(tmp, 0, 32);
  memcpy(tmp + bits[3] - 3, bits, 3);
  rev_copy(target, tmp);
}

uint32_t decode_var_int(uint8_t* p, uint64_t* val) {
  if (*p == 0xfd) {
    *val = (((uint32_t) p[2]) << 8) | p[1];
    return 3;
  }

  if (*p == 0xfe) {
    *val = le_to_int(p + 1);
    return 5;
  }

  if (*p == 0xff) {
    *val = le_to_long(p + 1);
    return 9;
  }
  *val = *p;
  return 1;
}

int btc_get_transaction_count(bytes_t block) {
  uint64_t count;
  decode_var_int(block.data + 80, &count);
  return (int) count;
}

int btc_get_transactions(bytes_t block, bytes_t* dst) {
  uint64_t count;
  uint8_t* p = block.data + 80 + decode_var_int(block.data + 80, &count);
  for (unsigned int i = 0; i < count; i++) p += (dst[i] = btc_get_transaction_end(p)).len;
  return count;
}

bytes_t btc_get_transaction_end(uint8_t* data) {
  uint64_t len;
  bool     witness = data[4] == 0 && data[5] == 1;
  uint8_t* p       = data + (witness ? 6 : 4);
  p += decode_var_int(p, &len);
  for (unsigned int i = 0; i < len; i++) p += btc_get_txinput(p).len;
  int txin = witness ? (int) len : 0;

  p += decode_var_int(p, &len);
  for (unsigned int i = 0; i < len; i++) p += btc_get_txoutput(p).len;

  // now read witnesses
  for (int i = 0; i < txin; i++) {
    p += decode_var_int(p, &len);
    int n = (int) len;
    for (int j = 0; j < n; j++) {
      p += decode_var_int(p, &len);
      p += len;
    }
  }
  return bytes(data, (p - data) + 4);
}

bytes_t btc_get_txinput(uint8_t* data) {
  uint64_t len;
  uint32_t l = decode_var_int(data + 36, &len);
  return bytes(data, 40 + l + len);
}

bytes_t btc_get_txoutput(uint8_t* data) {
  uint64_t len;
  uint32_t l = decode_var_int(data + 8, &len);
  return bytes(data, 8 + l + len);
}

in3_ret_t btc_serialize_block_header(d_token_t* data, uint8_t* block_header) {
  rev_hex(d_get_string(data, key("versionHex")), block_header, 4);
  rev_hex(d_get_string(data, key("previousblockhash")), block_header + 4, 32);
  rev_hex(d_get_string(data, key("merkleroot")), block_header + 36, 32);
  rev_copyl(block_header + 68, d_bytes(d_get(data, key("time"))), 4);
  rev_hex(d_get_string(data, key("bits")), block_header + 72, 4);
  rev_copyl(block_header + 76, d_bytes(d_get(data, key("nonce"))), 4);
  return IN3_OK;
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