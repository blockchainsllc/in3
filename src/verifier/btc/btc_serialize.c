#include "btc_serialize.h"
#include "../../core/util/data.h"
#include "../../core/util/utils.h"
#include "../../third-party/crypto/sha2.h"
#include "../../third-party/tommath/tommath.h"
#include <string.h>

static void rev_hex(char* hex, uint8_t* dst, int l) {
  int len = hex ? strlen(hex) : 0, i, j, out_len = (len + 1) >> 1;
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
  }
}
void btc_hash(bytes_t data, bytes32_t dst) {
  bytes32_t  tmp;
  SHA256_CTX ctx;
  sha256_Init(&ctx);
  sha256_Update(&ctx, data.data, data.len);
  sha256_Final(&ctx, tmp);
  sha256_Init(&ctx);
  sha256_Update(&ctx, tmp, 32);
  sha256_Final(&ctx, tmp);
  rev_copy(dst, tmp);
}

// copy 32 bytes in revers order
void rev_copy(uint8_t* dst, uint8_t* src) {
  for (int i = 0; i < 32; i++) dst[31 - i] = src[i];
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

void btc_target(bytes_t block, bytes32_t target) {
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
  for (unsigned int i = 0; i < count; i++) p += (dst[i] = btc_get_transaction(p)).len;
  return count;
}

bytes_t btc_get_transaction(uint8_t* data) {
  uint64_t len;
  uint8_t* p = data + 4 + decode_var_int(data + 4, &len);
  for (unsigned int i = 0; i < len; i++) p += btc_get_txinput(p).len;
  p += decode_var_int(p, &len);
  for (unsigned int i = 0; i < len; i++) p += btc_get_txoutput(p).len;
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
  rev_hex(d_get_string(data, "versionHex"), block_header, 4);
  rev_hex(d_get_string(data, "previousblockhash"), block_header + 4, 32);
  rev_hex(d_get_string(data, "merkleroot"), block_header + 36, 32);
  rev_copyl(block_header + 68, d_to_bytes(d_get(data, key("time"))), 4);
  rev_hex(d_get_string(data, "bits"), block_header + 72, 4);
  rev_copyl(block_header + 76, d_to_bytes(d_get(data, key("nonce"))), 4);
  return IN3_OK;
}
