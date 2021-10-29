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

typedef struct btc_utxo {
  uint8_t*     tx_hash;
  uint32_t     tx_index;
  btc_tx_out_t tx_out;
} btc_utxo_t;

void btc_init_tx(btc_tx_t* tx);
void btc_init_tx_in(btc_tx_in_t* tx_in);
void btc_init_tx_out(btc_tx_out_t* tx_out);

in3_ret_t btc_parse_tx(bytes_t tx, btc_tx_t* dst);
in3_ret_t btc_serialize_tx(btc_tx_t* tx, bytes_t* dst);
in3_ret_t btc_tx_id(btc_tx_t* tx, bytes32_t dst);

uint8_t* btc_parse_tx_in(uint8_t* data, btc_tx_in_t* dst, uint8_t* limit);
void     btc_serialize_tx_in(btc_tx_in_t* tx_in, bytes_t* dst);

uint8_t* btc_parse_tx_out(uint8_t* data, btc_tx_out_t* dst);
void     btc_serialize_tx_out(btc_tx_out_t* tx_out, bytes_t* dst);

uint32_t btc_vsize(btc_tx_t* tx);
uint32_t btc_weight(btc_tx_t* tx);

void add_input_to_tx(btc_tx_t* tx, btc_tx_in_t* tx_in);
void add_output_to_tx(btc_tx_t* tx, btc_tx_out_t* tx_out);

static inline bool btc_is_witness(bytes_t tx) {
  return tx.data[4] == 0 && tx.data[5] == 1;
}

void     add_outputs_to_tx(d_token_t* outputs, btc_tx_t* tx);
uint32_t btc_prepare_utxo(d_token_t* utxo_inputs, btc_utxo_t** utxos);

// void create_raw_tx(btc_tx_in_t* tx_in, uint32_t tx_in_len, btc_tx_out_t* tx_out, uint32_t tx_out_len, uint32_t lock_time, bytes_t* dst_raw_tx);

#endif