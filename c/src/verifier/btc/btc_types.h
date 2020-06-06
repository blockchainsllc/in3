#ifndef _BTC_TYPES_H
#define _BTC_TYPES_H

#include "../../core/util/bytes.h"
#include "../../core/util/data.h"
#include "../../core/util/error.h"
#include <stdint.h>

typedef struct btc_tx {
  bytes_t  all;
  uint32_t version;
  uint16_t flag;
  uint32_t input_count;
  bytes_t  input;
  uint32_t output_count;
  bytes_t  output;
  bytes_t  witnesses;
  uint32_t lock_time;
} btc_tx_t;

typedef struct btc_tx_in {
  uint8_t* prev_tx_hash;
  uint32_t prev_tx_index;
  bytes_t  script;
  uint32_t sequence;
} btc_tx_in_t;

typedef struct btc_tx_out {
  uint64_t value;
  bytes_t  script;
} btc_tx_out_t;

in3_ret_t btc_parse_tx(bytes_t tx, btc_tx_t* dst);
in3_ret_t btc_tx_id(btc_tx_t* tx, bytes32_t dst);
uint8_t*  btc_parse_tx_in(uint8_t* data, btc_tx_in_t* dst, uint8_t* limit);
uint8_t*  btc_parse_tx_out(uint8_t* data, btc_tx_out_t* dst);
uint32_t  btc_vsize(btc_tx_t* tx);
uint32_t  btc_weight(btc_tx_t* tx);

static inline bool btc_is_witness(bytes_t tx) {
  return tx.data[4] == 0 && tx.data[5] == 1;
}

#endif