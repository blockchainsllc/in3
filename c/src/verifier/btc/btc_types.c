#include "btc_types.h"
#include "../../core/util/mem.h"
#include "btc_serialize.h"

#define BTC_TX_VERSION_SIZE_BYTES  4
#define BTC_TX_LOCKTIME_SIZE_BYTES 4

uint8_t* btc_parse_tx_in(uint8_t* data, btc_tx_in_t* dst, uint8_t* limit) {
  uint64_t len;
  dst->prev_tx_hash  = data;
  dst->prev_tx_index = le_to_int(data + 32);
  dst->script.data   = data + 36 + decode_var_int(data + 36, &len);
  dst->script.len    = (uint32_t) len;
  if (dst->script.data + dst->script.len + 4 > limit) return NULL; // check limit
  dst->sequence = le_to_int(dst->script.data + dst->script.len);
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
  uint8_t* end = tx.data + tx.len;
  uint8_t* p   = tx.data + (dst->flag ? 6 : 4);

  p += decode_var_int(p, &val);
  if (p >= end) return IN3_EINVAL;
  dst->input_count = (uint32_t) val;
  dst->input.data  = p;
  for (uint32_t i = 0; i < dst->input_count; i++) {
    p = btc_parse_tx_in(p, &tx_in, end);
    if (!p || p >= end) return IN3_EINVAL;
  }
  dst->input.len = p - dst->input.data;

  p += decode_var_int(p, &val);
  dst->output_count = (uint32_t) val;
  dst->output.data  = p;
  for (uint32_t i = 0; i < dst->output_count; i++) {
    p = btc_parse_tx_out(p, &tx_out);
    if (p > end) return IN3_EINVAL;
  }
  dst->output.len = p - dst->output.data;
  dst->witnesses  = bytes(p, tx.data + tx.len - 4 - p);
  dst->lock_time  = le_to_int(tx.data + tx.len - 4);

  return IN3_OK;
}

// Converts a btc transaction into a serialized transaction
// TODO: Implement serialization for when tx_in and tx_out are not NULL
// WARNING: You need to free dst pointer after using this function!
in3_ret_t btc_serialize_transaction(btc_tx_t* tx, btc_tx_in_t* tx_in, btc_tx_out_t* tx_out, bytes_t* dst) {
  UNUSED_VAR(tx_in);
  UNUSED_VAR(tx_out);

  // Clean exit buffer
  dst->len = 0;

  // calculate transaction size in bytes
  uint32_t tx_size;
  tx_size = (BTC_TX_VERSION_SIZE_BYTES +
             (2 * tx->flag) +
             get_compact_uint_size((uint64_t) tx->input_count) +
             tx->input.len +
             get_compact_uint_size((uint64_t) tx->output_count) +
             tx->output.len +
             tx->witnesses.len +
             BTC_TX_LOCKTIME_SIZE_BYTES);

  dst->data = malloc(tx_size * sizeof(*dst->data));
  dst->len  = tx_size;

  // Serialize transaction data
  uint32_t index = 0;
  // version
  uint_to_le(dst, index, tx->version);
  index += 4;
  // Check if transaction uses SegWit
  if (tx->flag) {
    dst->data[index++] = 0;
    dst->data[index++] = 1;
  }
  // input_count
  long_to_compact_uint(dst, index, tx->input_count);
  index += get_compact_uint_size(tx->input_count);
  // inputs
  // TODO: serialize struct if tx_in is not null
  for (uint32_t i = 0; i < tx->input.len; i++) {
    dst->data[index++] = tx->input.data[i];
  }
  // output_count
  long_to_compact_uint(dst, index, tx->output_count);
  index += get_compact_uint_size(tx->output_count);
  // outputs
  // TODO: serialize struct if tx_out is not null
  for (uint32_t i = 0; i < tx->output.len; i++) {
    dst->data[index++] = tx->output.data[i];
  }
  // Include witness
  if (tx->flag) {
    for (uint32_t i = 0; i < tx->witnesses.len; i++) {
      dst->data[index++] = tx->witnesses.data[i];
    }
  }
  // locktime
  //uint_to_le(dst, index, tx->lock_time);
  dst->data[index + 3] = ((tx->lock_time >> 24) & 0xff);
  dst->data[index + 2] = ((tx->lock_time >> 16) & 0xff);
  dst->data[index + 1] = ((tx->lock_time >> 8) & 0xff);
  dst->data[index]     = ((tx->lock_time) & 0xff);

  return IN3_OK;
}

uint32_t btc_vsize(btc_tx_t* tx) {
  uint32_t w = btc_weight(tx);
  return w % 4 ? (w + 4) / 4 : w / 4;
}

uint32_t btc_weight(btc_tx_t* tx) {
  const uint32_t w = tx->witnesses.len
                         ? (tx->all.len - tx->witnesses.len - 2) * 3 + tx->all.len
                         : tx->all.len * 4;
  return w;
}

in3_ret_t btc_tx_id(btc_tx_t* tx, bytes32_t dst) {
  bytes_t  data;
  uint8_t* start = tx->all.data + (tx->flag ? 6 : 4);
  data.len       = tx->output.len + tx->output.data - start + 8;
  data.data      = data.len > 1000 ? _malloc(data.len) : alloca(data.len);
  memcpy(data.data, tx->all.data, 4);                                  // nVersion
  memcpy(data.data + 4, start, data.len - 8);                          // txins/txouts
  memcpy(data.data + data.len - 4, tx->all.data + tx->all.len - 4, 4); // lockTime

  btc_hash(data, dst);
  if (data.len > 1000) _free(data.data);
  return IN3_OK;
}
