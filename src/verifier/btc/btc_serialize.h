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

bytes_t   btc_block_get(bytes_t block, btc_block_field field);
void      btc_hash(bytes_t data, bytes32_t dst);
in3_ret_t btc_serialize_block_header(d_token_t* data, uint8_t* block_header);

// copy 32 bytes in revers order
void     rev_copy(uint8_t* dst, uint8_t* src);
uint32_t le_to_int(uint8_t* data);
uint64_t le_to_long(uint8_t* data);
void     btc_target(bytes_t block, bytes32_t target);
uint32_t decode_var_int(uint8_t* p, uint64_t* val);
int      btc_get_transaction_count(bytes_t block);
int      btc_get_transactions(bytes_t block, bytes_t* dst);
bytes_t  btc_get_transaction(uint8_t* data);
bytes_t  btc_get_txinput(uint8_t* data);
bytes_t  btc_get_txoutput(uint8_t* data);

#endif // _BTC_SERIALIZE_H