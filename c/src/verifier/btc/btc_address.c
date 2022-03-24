#include "btc_address.h"
#include "../../core/util/log.h"
#include "../../third-party/bech32/segwit_addr.h"
#include "btc_script.h"
#include "btc_types.h"

static const char* btc_mainnet_hrp = "bc";

// Converts a OP_N operation to an N integer.
// example1: op_n_to_n(OP_0) => 0
// example2: op_n_to_n(OP_3) => 3
static uint8_t op_n_to_n(uint8_t op) {
  return (op == OP_0) ? 0 : op - 0x50;
}

btc_address_prefix_t btc_script_type_to_prefix(btc_stype_t script_type) {
  switch (script_type) {
    case BTC_P2PK:
      return BTC_P2PK_PREFIX;
    case BTC_P2PKH:
      return BTC_P2PKH_PREFIX;
    case BTC_P2SH:
      return BTC_P2SH_PREFIX;
    default:
      return BTC_INVALID_PREFIX;
  }
}

int btc_addr_from_pub_key_hash(ripemd160_t pub_key_hash160, btc_address_prefix_t prefix, btc_address_t* dst) {
  if (!dst) return -1;

  uint8_t tmp[21], hash256_result[32], checksum[4];

  // First build prefix+hash160(pub_key)
  tmp[0] = prefix;
  memcpy(tmp + 1, pub_key_hash160, 20);

  // Calculate hash256(prefiix-hash160). Fist 4 bytes will be used as address checksum
  btc_hash(bytes(tmp, 21), hash256_result);
  rev_copyl(checksum, bytes(hash256_result + 28, 4), 4);

  memcpy(dst->as_bytes, tmp, 21);
  memcpy(dst->as_bytes + (size_t) 21, checksum, 4);

  // calculate base58 address encoding
  dst->encoded = _malloc(encode_size(ENC_BASE58, BTC_ADDRESS_SIZE_BYTES));
  return encode(ENC_BASE58, bytes(dst->as_bytes, BTC_ADDRESS_SIZE_BYTES), dst->encoded);
}

int btc_addr_from_pub_key(bytes_t pub_key, btc_address_prefix_t prefix, btc_address_t* dst) {
  ripemd160_t pub_key_hash;
  uint8_t     hash256_result[32];
  btc_hash256(pub_key, hash256_result);
  btc_hash160(bytes(hash256_result, 32), pub_key_hash);
  return btc_addr_from_pub_key_hash(pub_key_hash, prefix, dst);
}

int btc_segwit_addr_from_pub_key_hash(ripemd160_t pub_key_hash, btc_address_t* dst) {
  if (!dst) return -1;
  memzero(dst->as_bytes, BTC_ADDRESS_SIZE_BYTES);
  dst->encoded = _malloc(MAX_BECH32_STRING_LEN + 1);
  return segwit_addr_encode(dst->encoded, btc_mainnet_hrp, 0, pub_key_hash, 20);
}

int btc_segwit_addr_from_pub_key(bytes_t pub_key, btc_address_t* dst) {
  if (!pub_key_is_valid(&pub_key)) return -1;
  ripemd160_t pub_key_hash;
  uint8_t     hash256_result[32];
  btc_hash256(pub_key, hash256_result);
  btc_hash160(bytes(hash256_result, 32), pub_key_hash);
  return btc_segwit_addr_from_pub_key_hash(pub_key_hash, dst);
}

int btc_segwit_addr_from_witness_program(bytes_t witness_program, btc_address_t* dst) {
  if (!is_witness_program(&witness_program)) return -1;
  uint8_t  witver    = op_n_to_n(witness_program.data[0]);
  uint8_t* hash_data = witness_program.data + 2; // witness programs have format: VERSION(1) | HASH_LEN(1) | HASH(20 or 32 bytes)
  uint8_t  hash_len  = witness_program.data[1];
  dst->encoded       = _malloc(MAX_BECH32_STRING_LEN + 1);
  int ret            = segwit_addr_encode(dst->encoded, btc_mainnet_hrp, witver, hash_data, hash_len);
  memzero(dst->as_bytes, BTC_ADDRESS_SIZE_BYTES);
  return ret;
}
