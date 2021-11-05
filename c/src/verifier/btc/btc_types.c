#include "btc_types.h"
#include "../../core/client/request.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "btc_serialize.h"

// Transaction fixed size values
#define BTC_TX_VERSION_SIZE_BYTES  4
#define BTC_TX_LOCKTIME_SIZE_BYTES 4

// Input fixed size values
#define BTC_TX_IN_PREV_OUPUT_SIZE_BYTES 36 // Outpoint = prev txid (32 bytes) + output index (4 bytes)
#define BTC_TX_IN_SEQUENCE_SIZE_BYTES   4

// Output fixed size values
#define BTC_TX_OUT_VALUE_SIZE_BYTES 8

typedef enum btc_tx_field {
  BTC_INPUT,
  BTC_OUTPUT,
  BTC_WITNESS
} btc_tx_field_t;

void btc_init_tx(btc_tx_t* tx) {
  memset(tx, 0, sizeof(btc_tx_t));
  tx->version = 1;
}

void btc_init_tx_in(btc_tx_in_t* tx_in) {
  tx_in->prev_tx_hash  = NULL;
  tx_in->prev_tx_index = 0;
  tx_in->script.len    = 0;
  tx_in->script.data   = NULL;
  tx_in->sequence      = 0xffffffff;
}

void btc_init_tx_out(btc_tx_out_t* tx_out) {
  tx_out->script.len  = 0;
  tx_out->script.data = NULL;
  tx_out->value       = 0;
}

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

// WARNING: You need to free dst.data after calling this function
// TODO: Implement support for "Coinbase" inputs
// TODO: Handle null arguments
// TODO: Handle max script len = 10000 bytes
in3_ret_t btc_serialize_tx_in(in3_req_t* req, btc_tx_in_t* tx_in, bytes_t* dst) {
  if (!tx_in || !dst) return IN3_EINVAL;
  // calculate serialized tx input size in bytes
  uint32_t tx_in_size = (BTC_TX_IN_PREV_OUPUT_SIZE_BYTES +
                         get_compact_uint_size((uint64_t) tx_in->script.len) +
                         tx_in->script.len +
                         BTC_TX_IN_SEQUENCE_SIZE_BYTES);

  // serialize tx_in
  // -- Previous outpoint
  if (!tx_in->prev_tx_hash) return req_set_error(req, "missing prevtash_hash", IN3_ERPC);

  // alloc memory in dst
  dst->data = _malloc(tx_in_size);
  dst->len  = tx_in_size;

  uint32_t index = 0;
  for (uint32_t i = 0; i < 32; i++) {
    dst->data[index++] = tx_in->prev_tx_hash[31 - i];
  }
  uint_to_le(dst, index, tx_in->prev_tx_index);
  index += 4;

  // -- script
  long_to_compact_uint(dst, index, tx_in->script.len);
  index += get_compact_uint_size(tx_in->script.len);

  for (uint32_t i = 0; i < tx_in->script.len; i++) {
    dst->data[index++] = tx_in->script.data[i];
  }

  // -- sequence
  uint_to_le(dst, index, tx_in->sequence);
  return IN3_OK;
}

uint8_t* btc_parse_tx_out(uint8_t* data, btc_tx_out_t* dst) {
  uint64_t len;
  dst->value       = le_to_long(data);
  dst->script.data = data + 8 + decode_var_int(data + 8, &len);
  dst->script.len  = (uint32_t) len;
  return dst->script.data + dst->script.len;
}

// WARNING: You need to free 'dst' pointer after calling this function
// TODO: Handle null arguments
// TODO: Handle max script len = 10000 bytes
void btc_serialize_tx_out(btc_tx_out_t* tx_out, bytes_t* dst) {
  // calculate serialized tx output size in bytes
  uint32_t tx_out_size = (BTC_TX_OUT_VALUE_SIZE_BYTES +
                          get_compact_uint_size((uint64_t) tx_out->script.len) +
                          tx_out->script.len);

  // alloc memory in dst
  dst->data = _malloc(tx_out_size);
  dst->len  = tx_out_size;

  // serialize tx_out
  uint32_t index = 0;

  // -- value
  long_to_le(dst, index, tx_out->value);
  index += 8;

  // -- lock-script size
  long_to_compact_uint(dst, index, tx_out->script.len);
  index += get_compact_uint_size((uint64_t) tx_out->script.len);

  // -- lock-script
  memcpy(dst->data + index, tx_out->script.data, tx_out->script.len); 
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
// WARNING: You need to free dst pointer after using this function!
// TODO: Error handling for null tx and dst pointers
in3_ret_t btc_serialize_tx(btc_tx_t* tx, bytes_t* dst) {
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
             (tx->flag ? tx->witnesses.len : 0) +
             BTC_TX_LOCKTIME_SIZE_BYTES);

  dst->data = _malloc(tx_size);
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


static in3_ret_t add_to_tx(in3_req_t* req, btc_tx_t* tx, void* src, btc_tx_field_t field_type) {
  if (!tx || !src) {
    return req_set_error(req, "ERROR: in add_to_tx: Function arguments cannot be null!", IN3_EINVAL);
  }

  bytes_t  raw_src, *dst;
  uint32_t old_len;

  switch (field_type) {
    case BTC_INPUT:
      btc_serialize_tx_in(req, (btc_tx_in_t*) src, &raw_src);
      old_len = tx->input.len;
      dst     = &tx->input;
      tx->input_count++;
      break;
    case BTC_OUTPUT:
      btc_serialize_tx_out((btc_tx_out_t*) src, &raw_src);
      old_len = tx->output.len;
      dst     = &tx->output;
      tx->output_count++;
      break;
    case BTC_WITNESS:
      old_len = tx->witnesses.len;
      dst     = &tx->witnesses;
      raw_src.len = ((bytes_t*) src)->len;
      raw_src.data = ((bytes_t*) src)->data;
      break;
    default:
      // TODO: Implement better error handling
      return req_set_error(req, "Unrecognized transaction field code. No action was performed", IN3_EINVAL);
  }

  size_t mem_size = raw_src.len;
  dst->data       = (!dst->data) ? _malloc(mem_size) : _realloc(dst->data, mem_size, dst->len);
  dst->len += raw_src.len;
  // Add bytes to tx field
  // for (uint32_t i = 0; i < raw_src.len; i++) {
  //   dst->data[old_len + i] = raw_src.data[i];
  // }
  return IN3_OK;
}

in3_ret_t add_input_to_tx(in3_req_t* req, btc_tx_t* tx, btc_tx_in_t* tx_in) {
  return add_to_tx(req, tx, tx_in, BTC_INPUT);
}

in3_ret_t add_output_to_tx(in3_req_t* req, btc_tx_t* tx, btc_tx_out_t* tx_out) {
  return add_to_tx(req, tx, tx_out, BTC_OUTPUT);
}

in3_ret_t add_witness_to_tx(in3_req_t* req, btc_tx_t* tx, bytes_t* witness) {
  return add_to_tx(req, tx, witness, BTC_WITNESS);
}

in3_ret_t add_outputs_to_tx(in3_req_t* req, d_token_t* outputs, btc_tx_t* tx) {
  uint32_t len = d_len(outputs);
  for (uint32_t i = 0; i < len; i++) {
    d_token_t* output = d_get_at(outputs, i);

    const char* script_string = d_string(d_get(output, key("script")));
    uint64_t    value         = d_get_long(output, key("value"));

    btc_tx_out_t tx_out;
    uint32_t     script_len = strlen(script_string) / 2;
    bytes_t      script     = bytes(_malloc(script_len), script_len);
    hex_to_bytes(script_string, strlen(script_string), script.data, script.len);

    tx_out.script = script;
    tx_out.value  = value;

    TRY_CATCH(add_output_to_tx(req, tx, &tx_out), _free(script.data);)
  }
  return IN3_OK;
}

// utxos must be freed
in3_ret_t btc_prepare_utxo(d_token_t* utxo_inputs, btc_utxo_t** utxos, uint32_t* len) {
  *len   = d_len(utxo_inputs);
  *utxos = _malloc(*len * sizeof(btc_utxo_t));

  for (uint32_t i = 0; i < *len; i++) {
    btc_utxo_t  utxo;
    d_token_t*  utxo_input       = d_get_at(utxo_inputs, i);
    uint32_t    tx_index         = d_get_long(d_get(utxo_input, key("tx_index")), 0L);
    uint64_t    value            = d_get_long(d_get(utxo_input, key("value")), 0L);
    const char* tx_hash_string   = d_string(d_get(utxo_input, key("tx_hash")));
    const char* tx_script_string = d_string(d_get(utxo_input, key("script")));

    uint32_t tx_hash_len = strlen(tx_hash_string) / 2;
    bytes_t  tx_hash     = bytes(_malloc(tx_hash_len), tx_hash_len);
    hex_to_bytes(tx_hash_string, strlen(tx_hash_string), tx_hash.data, tx_hash.len);

    uint32_t tx_script_len = strlen(tx_script_string) / 2;
    bytes_t  tx_script     = bytes(_malloc(tx_script_len), tx_script_len);
    hex_to_bytes(tx_script_string, strlen(tx_script_string), tx_script.data, tx_script.len);

    utxo.tx_hash       = tx_hash.data;
    utxo.tx_index      = tx_index;
    utxo.tx_out.value  = value;
    utxo.tx_out.script = tx_script;

    *utxos[i] = utxo;
  }

  return IN3_OK;
}
