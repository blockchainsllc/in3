#ifndef _BTC_ADDRESS_H
#define _BTC_ADDRESS_H

#include "btc_serialize.h"

#define BTC_ADDRESS_SIZE_BYTES 25

/* btc address prefixes */
typedef enum {
  BTC_P2PKH_PREFIX         = 0x00,
  BTC_P2SH_PREFIX          = 0x05,
  BTC_P2PK_PREFIX          = 0x80,
  BTC_P2PKH_PREFIX_TESTNET = 0x6f,
  BTC_P2SH_PREFIX_TESTNET  = 0xc4
} btc_address_prefix_t;

typedef uint8_t btc_addr_t[BTC_ADDRESS_SIZE_BYTES];

typedef struct btc_address {
  btc_addr_t as_bytes; // raw byte representation of btc address
  char*      encoded;  // Encoding of btc address (usually base58 or bech32)
} btc_address_t;

void btc_addr_from_pub_key_hash(ripemd160_t pub_key_hash160, btc_address_prefix_t prefix, btc_address_t* dst);
void btc_addr_from_pub_key(bytes_t pub_key, btc_address_prefix_t prefix, btc_address_t* dst);

#endif