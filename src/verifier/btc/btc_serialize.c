#include "btc_serialize.h"
#include "../../third-party/crypto/sha2.h"
#include "../../third-party/tommath/tommath.h"
#include <string.h>

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

uint32_t le_to_int(uint8_t* data) {
  return (((uint32_t) data[3]) << 24) | (((uint32_t) data[2]) << 16) | (((uint32_t) data[1]) << 8) | data[0];
}

uint64_t le_to_long(uint8_t* data) {
  return (((uint64_t) data[7]) << 24) | (((uint64_t) data[6]) << 24) | (((uint64_t) data[5]) << 24) | (((uint64_t) data[4]) << 24) |
         (((uint64_t) data[3]) << 24) | (((uint64_t) data[2]) << 16) | (((uint64_t) data[1]) << 8) | data[0];
}

void btc_target_le(bytes_t block, bytes32_t target) {
  uint8_t* bits = btc_block_get(block, BTC_B_BITS).data;
  memset(target, 0, 32);
  memcpy(target + bits[3] - 3, bits, 3);
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
