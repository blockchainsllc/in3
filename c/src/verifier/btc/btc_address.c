#include "btc_address.h"

void btc_addr_from_pub_key_hash(ripemd160_t pub_key_hash160, btc_address_prefix_t prefix, btc_address_t* dst) {
  uint8_t tmp[21], hash256_result[32], checksum[4];

  // First build prefix+hash160(pub_key)
  tmp[0] = prefix;
  memcpy(tmp + 1, pub_key_hash160, 20);

  // Calculate hash256(prefiix-hash160). Fist 4 bytes will be used as address checksum
  btc_hash256(bytes(tmp, 21), hash256_result);
  memcpy(checksum, hash256_result, 4);

  memcpy(dst->as_bytes, tmp, 21);
  memcpy(dst->as_bytes + (size_t) 21, checksum, 4);

  // calculate base58 address encoding
  encode(ENC_BASE58, bytes(dst->as_bytes, BTC_ADDRESS_SIZE_BYTES), dst->encoded);
}

void btc_addr_from_pub_key(bytes_t pub_key, btc_address_prefix_t prefix, btc_address_t* dst) {
  ripemd160_t pub_key_hash;
  btc_hash160(pub_key, pub_key_hash);
  btc_addr_from_pub_key_hash(pub_key_hash, prefix, dst);
}