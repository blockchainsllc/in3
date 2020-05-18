#include "btc_types.h"
#include "../../core/util/mem.h"
#include "btc_serialize.h"

uint8_t* btc_parse_tx_in(uint8_t* data, btc_tx_in_t* dst) {
  uint64_t len;
  dst->prev_tx_hash  = data;
  dst->prev_tx_index = le_to_int(data + 32);
  dst->script.data   = data + 36 + decode_var_int(data + 36, &len);
  dst->script.len    = (uint32_t) len;
  dst->sequence      = le_to_int(dst->script.data + dst->script.len);
  return dst->script.data + dst->script.len + 4;
}

uint8_t* btc_parse_tx_out(uint8_t* data, btc_tx_out_t* dst) {
  uint64_t len;
  dst->value       = le_to_long(data);
  dst->script.data = data + 8 + decode_var_int(data + 8, &len);
  dst->script.len  = (uint32_t) len;
  return dst->script.data + dst->script.len;
}

in3_ret_t btc_parse_tx(bytes_t tx, btc_tx_t* dst) {
  uint64_t     val;
  btc_tx_in_t  tx_in;
  btc_tx_out_t tx_out;
  dst->all     = tx;
  dst->version = le_to_int(tx.data);
  dst->flag    = btc_is_witness(tx) ? 1 : 0;

  uint8_t* p = tx.data + (dst->flag ? 6 : 4);
  p += decode_var_int(p, &val);
  dst->input_count = (uint32_t) val;
  dst->input.data  = p;
  for (uint32_t i = 0; i < dst->input_count; i++) p = btc_parse_tx_in(p, &tx_in);
  dst->input.len = p - dst->input.data;

  p += decode_var_int(p, &val);
  dst->output_count = (uint32_t) val;
  dst->output.data  = p;
  for (uint32_t i = 0; i < dst->output_count; i++) p = btc_parse_tx_out(p, &tx_out);
  dst->output.len = p - dst->output.data;
  dst->witnesses  = bytes(p, tx.data + tx.len - 4 - p);
  dst->lock_time  = le_to_int(tx.data + tx.len - 4);

  return IN3_OK;
}

uint32_t btc_vsize(btc_tx_t* tx) {
  return tx->output.len + tx->output.data - (tx->all.data + (tx->flag ? 6 : 4)) + 8;
}

uint32_t btc_weight(btc_tx_t* tx) {
  uint32_t w = btc_vsize(tx) * 4;
  if (tx->flag) w += 2 + tx->witnesses.len;
  return w;
}

in3_ret_t btc_tx_id(btc_tx_t* tx, bytes32_t dst) {
  bytes_t  data;
  uint8_t* start = tx->all.data + (tx->flag ? 6 : 4);
  data.len       = tx->output.len + tx->output.data - start + 8;
  data.data      = alloca(data.len);
  memcpy(data.data, tx->all.data, 4);                                  // nVersion
  memcpy(data.data + 4, start, data.len - 8);                          // txins/txouts
  memcpy(data.data + data.len - 4, tx->all.data + tx->all.len - 4, 4); //lockTime

  btc_hash(data, dst);
  return IN3_OK;
}
