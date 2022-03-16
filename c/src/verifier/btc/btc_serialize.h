#ifndef _BTC_SERIALIZE_H
#define _BTC_SERIALIZE_H

#include "../../core/util/bytes.h"
#include "../../core/util/data.h"
#include "../../core/util/error.h"
#include <stdint.h>

/** type of a token. */
typedef enum {
  BTC_B_VERSION     = 0,
  BTC_B_PARENT_HASH = 1,
  BTC_B_MERKLE_ROOT = 2,
  BTC_B_TIMESTAMP   = 3,
  BTC_B_BITS        = 4,
  BTC_B_NONCE       = 5,
  BTC_B_HEADER      = 15

} btc_block_field;

typedef uint8_t ripemd160_t[20];

bytes_t   btc_block_get(bytes_t block, btc_block_field field);
void      btc_hash(bytes_t data, bytes32_t dst);
void      btc_hash256(bytes_t data, bytes32_t dst);
void      btc_hash160(bytes_t data, address_t dst);
in3_ret_t btc_serialize_block_header(d_token_t* data, uint8_t* block_header);

void      rev_copy(uint8_t* dst, uint8_t* src);        // copy 32 bytes in reverse order
void      rev_copyl(uint8_t* dst, bytes_t src, int l); // copy bytes in reverse order
uint32_t  le_to_int(uint8_t* data);
uint64_t  le_to_long(uint8_t* data);
void      btc_target_from_block(bytes_t block, bytes32_t target);
uint32_t  decode_var_int(uint8_t* p, uint64_t* val);
int       btc_get_transaction_count(bytes_t block);
int       btc_get_transactions(bytes_t block, bytes_t* dst);
bytes_t   btc_get_transaction_end(uint8_t* data);
bytes_t   btc_get_txinput(uint8_t* data);
bytes_t   btc_get_txoutput(uint8_t* data);
void      uint_to_le(bytes_t* buf, uint32_t index, uint32_t value);
void      long_to_le(bytes_t* buf, uint32_t index, uint64_t value);
size_t    get_compact_uint_size(uint64_t cmpt_uint);
void      long_to_compact_uint(bytes_t* buf, uint32_t index, uint64_t value);
in3_ret_t append_bytes(bytes_t* dst, const bytes_t* src);

#endif // _BTC_SERIALIZE_H