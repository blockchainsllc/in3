#ifndef _BTC_ADDRESS_H
#define _BTC_ADDRESS_H

#include "btc_script.h"
#include "btc_serialize.h"

#define BTC_PK_ADDR_SIZE_BYTES   25
#define BTC_MAX_ADDR_SIZE_BYTES  40
#define BTC_MAX_ADDR_STRING_SIZE 91

/* btc address prefixes */
typedef enum {
  BTC_P2PKH_PREFIX         = 0x00,
  BTC_P2SH_PREFIX          = 0x05,
  BTC_P2PKH_PREFIX_TESTNET = 0x6f,
  BTC_P2SH_PREFIX_TESTNET  = 0xc4,
  BTC_INVALID_PREFIX       = 0xff
} btc_address_prefix_t;

typedef struct btc_address {
  bytes_t as_bytes; // raw byte representation of btc address
  char*   encoded;  // Encoding of btc address (usually base58 or bech32)
} btc_address_t;

btc_address_t btc_addr(bytes_t as_bytes, char* encoded);

int btc_addr_from_pub_key_hash(ripemd160_t pub_key_hash160, btc_address_prefix_t prefix, btc_address_t* dst);
int btc_addr_from_pub_key(bytes_t pub_key, btc_address_prefix_t prefix, btc_address_t* dst);
int btc_segwit_addr_from_pub_key_hash(ripemd160_t pub_key_hash, btc_address_t* dst, bool is_testnet);
int btc_segwit_addr_from_pub_key(bytes_t pub_key, btc_address_t* dst, bool is_testnet);
int btc_segwit_addr_from_witness_program(bytes_t witness_program, btc_address_t* dst, bool is_testnet);

btc_stype_t btc_get_addr_type(const char* address);
int         btc_decode_address(bytes_t* dst, const char* src, bool is_testnet);

btc_address_prefix_t btc_script_type_to_prefix(btc_stype_t script_type, bool is_testnet);

#endif