#include "btc_address.h"
#include "../../core/util/log.h"
#include "../../third-party/bech32/segwit_addr.h"
#include "btc_script.h"
#include "btc_types.h"
#include <string.h>

static const char* btc_mainnet_hrp = "bc";
static const char* btc_testnet_hrp = "tb";

// Converts a OP_N operation to an N integer.
// example1: op_n_to_n(OP_0) => 0
// example2: op_n_to_n(OP_3) => 3
static uint8_t op_n_to_n(uint8_t op) {
  return (op == OP_0) ? 0 : op - 0x50;
}

btc_address_prefix_t btc_script_type_to_prefix(btc_stype_t script_type, bool is_testnet) {
  switch (script_type) {
    case BTC_P2PK:
    case BTC_P2PKH:
      return is_testnet ? BTC_P2PKH_PREFIX_TESTNET : BTC_P2PKH_PREFIX;
    case BTC_P2SH:
      return is_testnet ? BTC_P2SH_PREFIX_TESTNET : BTC_P2SH_PREFIX;
    default:
      return BTC_INVALID_PREFIX;
  }
}

btc_address_t btc_addr(bytes_t as_bytes, char* encoded) {
  return (btc_address_t){.as_bytes = as_bytes, .encoded = encoded};
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

  memcpy(dst->as_bytes.data, tmp, 21);
  memcpy(dst->as_bytes.data + (size_t) 21, checksum, 4);
  dst->as_bytes.len = BTC_PK_ADDR_SIZE_BYTES;

  // calculate base58 address encoding
  return encode(ENC_BASE58, dst->as_bytes, dst->encoded);
}

int btc_addr_from_pub_key(bytes_t pub_key, btc_address_prefix_t prefix, btc_address_t* dst) {
  ripemd160_t pub_key_hash;
  uint8_t     hash256_result[32];
  btc_hash256(pub_key, hash256_result);
  btc_hash160(bytes(hash256_result, 32), pub_key_hash);
  return btc_addr_from_pub_key_hash(pub_key_hash, prefix, dst);
}

btc_stype_t btc_get_addr_type(const char* address, bool is_testnet) {
  if ((!is_testnet && address[0] == '1') || 
      (is_testnet && (address[0] == 'm' || address[0] == 'n'))) {
    return BTC_P2PKH;
  }

  if ((!is_testnet && address[0] == '3') || 
       (is_testnet && address[0] == '2')) {
    return BTC_P2SH;
  }

  if ((!is_testnet && (address[0] == 'b' && address[1] == 'c' && address[2] == '1' && address[3] == 'q')) || 
     (is_testnet && (address[0] == 't' && address[1] == 'b' && address[2] == '1' && address[3] == 'q'))) {
    size_t addr_len = strlen(address);
    if (addr_len == 42) return BTC_V0_P2WPKH;
    if (addr_len == 62) return BTC_P2WSH;
  }

  return BTC_UNSUPPORTED;
}

int btc_decode_address(bytes_t* dst, const char* src, bool is_testnet) {
  UNUSED_VAR(dst);
  btc_stype_t addr_type = btc_get_addr_type(src, is_testnet);
  switch (addr_type) {
    case BTC_P2PKH:
    case BTC_P2SH:
      return decode(ENC_BASE58, src, strlen(src), dst->data);
    case BTC_V0_P2WPKH:
    case BTC_P2WSH: {
      int         ver;
      const char* hrp = is_testnet ? btc_testnet_hrp : btc_mainnet_hrp;
      int         ret = segwit_addr_decode(&ver, dst->data, (size_t*) &dst->len, hrp, src);
      return (ret - 1);
    }
    default:
      return -1;
  }
}

int btc_segwit_addr_from_pub_key_hash(ripemd160_t pub_key_hash, btc_address_t* dst, bool is_testnet) {
  if (!dst) return -1;
  memzero(dst->as_bytes.data, BTC_MAX_ADDR_SIZE_BYTES);
  dst->encoded    = _malloc(BTC_MAX_ADDR_STRING_SIZE + 1);
  const char* hrp = is_testnet ? btc_testnet_hrp : btc_mainnet_hrp;
  return segwit_addr_encode(dst->encoded, hrp, 0, pub_key_hash, 20);
}

int btc_segwit_addr_from_pub_key(bytes_t pub_key, btc_address_t* dst, bool is_testnet) {
  if (!pub_key_is_valid(&pub_key)) return -1;
  ripemd160_t pub_key_hash;
  uint8_t     hash256_result[32];
  btc_hash256(pub_key, hash256_result);
  btc_hash160(bytes(hash256_result, 32), pub_key_hash);
  return btc_segwit_addr_from_pub_key_hash(pub_key_hash, dst, is_testnet);
}

int btc_segwit_addr_from_witness_program(bytes_t witness_program, btc_address_t* dst, bool is_testnet) {
  if (!is_witness_program(&witness_program)) return -1;
  uint8_t  witver    = op_n_to_n(witness_program.data[0]);
  uint8_t* hash_data = witness_program.data + 2; // witness programs have format: VERSION(1) | HASH_LEN(1) | HASH(20 or 32 bytes)
  uint8_t  hash_len  = witness_program.data[1];
  dst->encoded       = _malloc(BTC_MAX_ADDR_STRING_SIZE + 1);
  const char* hrp    = is_testnet ? btc_testnet_hrp : btc_mainnet_hrp;
  int         ret    = segwit_addr_encode(dst->encoded, hrp, witver, hash_data, hash_len);
  memzero(dst->as_bytes.data, BTC_MAX_ADDR_SIZE_BYTES);
  return ret;
}
