#include "btc_types.h"
#include "../../core/client/request.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "btc_script.h"
#include "btc_serialize.h"

typedef enum btc_tx_field {
  BTC_INPUT,
  BTC_OUTPUT,
  BTC_WITNESS
} btc_tx_field_t;

bool script_is_standard(btc_stype_t script_type) {
  return script_type != BTC_NON_STANDARD && script_type != BTC_UNSUPPORTED && script_type != BTC_UNKNOWN;
}

bool pub_key_is_valid(const bytes_t* pub_key) {
  return (pub_key->len == BTC_UNCOMP_PUB_KEY_SIZE_BYTES && pub_key->data[0] == 0x4) || (pub_key->len == BTC_COMP_PUB_KEY_SIZE_BYTES && (pub_key->data[0] == 0x2 || pub_key->data[0] == 0x3));
}

void btc_init_tx(btc_tx_t* tx) {
  if (tx) {
    memset(tx, 0, sizeof(btc_tx_t));
    tx->version = 1;
  }
}

void btc_init_tx_ctx(btc_tx_ctx_t* tx_ctx) {
  if (tx_ctx) {
    memset(tx_ctx, 0, sizeof(btc_tx_ctx_t));
    btc_init_tx(&tx_ctx->tx);
  }
}

void btc_init_tx_in(btc_tx_in_t* tx_in) {
  if (tx_in) {
    memset(tx_in, 0, sizeof(btc_tx_in_t));
    tx_in->sequence = 0xffffffff;
  }
}

void btc_init_tx_out(btc_tx_out_t* tx_out) {
  if (tx_out) {
    memset(tx_out, 0, sizeof(btc_tx_out_t));
  }
}

void btc_init_utxo(btc_utxo_t* utxo) {
  if (utxo) {
    memset(utxo, 0, sizeof(btc_utxo_t));
  }
}

void btc_free_tx(btc_tx_t* tx) {
  if (tx) {
    if (tx->all.data) _free(tx->all.data);
    if (tx->input.data) _free(tx->input.data);
    if (tx->output.data) _free(tx->output.data);
    if (tx->witnesses.data) _free(tx->witnesses.data);
  }
}

void btc_free_tx_in(btc_tx_in_t* tx_in) {
  if (tx_in) {
    if (tx_in->prev_tx_hash) _free(tx_in->prev_tx_hash);
    if (tx_in->script.data.data) _free(tx_in->script.data.data);
  }
}

void btc_free_tx_out(btc_tx_out_t* tx_out) {
  if (tx_out && tx_out->script.data.data) _free(tx_out->script.data.data);
}

void btc_free_utxo(btc_utxo_t* utxo) {
  if (utxo) {
    if (utxo->tx_hash) _free(utxo->tx_hash);
    if (utxo->raw_script.data.data) _free(utxo->raw_script.data.data);

    btc_free_tx_out(&utxo->tx_out);

    if (utxo->signatures) {
      for (uint32_t i = 0; i < utxo->sig_count; i++) {
        _free(utxo->signatures[i].data);
      }
      _free(utxo->signatures);
    }

    if (utxo->accounts) {
      for (uint32_t i = 0; i < utxo->accounts_count; i++) {
        _free(utxo->accounts[i].pub_key.data);
        _free(utxo->accounts[i].account.data);
      }
      _free(utxo->accounts);
    }
  }
}

void btc_free_tx_ctx(btc_tx_ctx_t* tx_ctx) {
  if (tx_ctx) {
    btc_free_tx(&tx_ctx->tx);
    if (tx_ctx->utxos) {
      for (uint32_t i = 0; i < tx_ctx->utxo_count; i++) {
        btc_free_utxo(&tx_ctx->utxos[i]);
      }
      _free(tx_ctx->utxos);
    }
    if (tx_ctx->inputs) {
      for (uint32_t i = 0; i < tx_ctx->input_count; i++) {
        btc_free_tx_in(&tx_ctx->inputs[i]);
      }
      _free(tx_ctx->inputs);
    }
    if (tx_ctx->outputs) {
      for (uint32_t i = 0; i < tx_ctx->output_count; i++) {
        btc_free_tx_out(&tx_ctx->outputs[i]);
      }
      _free(tx_ctx->outputs);
    }
  }
}

uint8_t* btc_parse_tx_in(uint8_t* data, btc_tx_in_t* dst, uint8_t* limit) {
  uint64_t len;
  dst->prev_tx_hash     = data;
  dst->prev_tx_index    = le_to_int(data + BTC_TX_HASH_SIZE_BYTES);
  dst->script.data.data = data + BTC_TX_IN_PREV_OUPUT_SIZE_BYTES + decode_var_int(data + BTC_TX_IN_PREV_OUPUT_SIZE_BYTES, &len);
  dst->script.data.len  = (uint32_t) len;
  if (dst->script.data.data + dst->script.data.len + 4 > limit) return NULL; // check limit
  dst->sequence = le_to_int(dst->script.data.data + dst->script.data.len);
  return dst->script.data.data + dst->script.data.len + 4;
}

// WARNING: You need to free dst.data after calling this function
// TODO: Implement support for "Coinbase" inputs
// TODO: Handle null arguments
// TODO: Handle max script len = 10000 bytes
in3_ret_t btc_serialize_tx_in(in3_req_t* req, btc_tx_in_t* tx_in, bytes_t* dst) {
  if (!tx_in || !dst) return req_set_error(req, "ERROR: in btc_serialize_tx_in: Arguments cannot be null", IN3_EINVAL);
  if (!tx_in->prev_tx_hash) return req_set_error(req, "ERROR: in btc_serialize_tx_in: missing previous transaction hash", IN3_ERPC);

  // calculate serialized tx input size in bytes
  uint32_t tx_in_size = (BTC_TX_IN_PREV_OUPUT_SIZE_BYTES +
                         get_compact_uint_size((uint64_t) tx_in->script.data.len) +
                         tx_in->script.data.len +
                         BTC_TX_IN_SEQUENCE_SIZE_BYTES);

  // serialize tx_in
  dst->data = (dst->data) ? _realloc(&dst->data, tx_in_size, dst->len) : _malloc(tx_in_size);
  dst->len  = tx_in_size;

  uint32_t index = 0;
  rev_copyl(dst->data + index, bytes(tx_in->prev_tx_hash, BTC_TX_HASH_SIZE_BYTES), BTC_TX_HASH_SIZE_BYTES);
  index += BTC_TX_HASH_SIZE_BYTES;
  uint_to_le(dst, index, tx_in->prev_tx_index);
  index += BTX_TX_INDEX_SIZE_BYTES;

  // -- script
  long_to_compact_uint(dst, index, tx_in->script.data.len);
  index += get_compact_uint_size(tx_in->script.data.len);
  memcpy(dst->data + index, tx_in->script.data.data, tx_in->script.data.len);

  // -- sequence
  uint_to_le(dst, index, tx_in->sequence);
  return IN3_OK;
}

uint8_t* btc_parse_tx_out(uint8_t* data, btc_tx_out_t* dst) {
  uint64_t len;
  dst->value            = le_to_long(data);
  dst->script.data.data = data + BTC_TX_OUT_VALUE_SIZE_BYTES + decode_var_int(data + BTC_TX_OUT_VALUE_SIZE_BYTES, &len);
  dst->script.data.len  = (uint32_t) len;
  return dst->script.data.data + dst->script.data.len;
}

// WARNING: You need to free 'dst' pointer after calling this function
// TODO: Handle null arguments
in3_ret_t btc_serialize_tx_out(in3_req_t* req, btc_tx_out_t* tx_out, bytes_t* dst) {
  if (!tx_out || !dst) return req_set_error(req, "ERROR: in btc_serialize_tx_out: Arguments cannot be null", IN3_EINVAL);
  if (tx_out->script.data.len > MAX_SCRIPT_SIZE_BYTES) return req_set_error(req, "ERROR: in btc_serialize_tx_out: Script is bigger than the maximum allowed size", IN3_ENOTSUP);

  // calculate serialized tx output size in bytes
  uint32_t tx_out_size = (BTC_TX_OUT_VALUE_SIZE_BYTES +
                          get_compact_uint_size((uint64_t) tx_out->script.data.len) +
                          tx_out->script.data.len);

  // alloc memory in dst
  dst->data = (dst->data) ? _realloc(&dst->data, tx_out_size, dst->len) : _malloc(tx_out_size);
  dst->len  = tx_out_size;

  // serialize tx_out
  uint32_t index = 0;

  // -- value
  long_to_le(dst, index, tx_out->value);
  index += BTC_TX_OUT_VALUE_SIZE_BYTES;

  // -- lock-script size
  long_to_compact_uint(dst, index, tx_out->script.data.len);
  index += get_compact_uint_size((uint64_t) tx_out->script.data.len);

  // -- lock-script
  memcpy(dst->data + index, tx_out->script.data.data, tx_out->script.data.len);
  return IN3_OK;
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

uint32_t btc_get_raw_tx_size(const btc_tx_t* tx) {
  return (BTC_TX_VERSION_SIZE_BYTES +
          (2 * tx->flag) +
          get_compact_uint_size((uint64_t) tx->input_count) +
          tx->input.len +
          get_compact_uint_size((uint64_t) tx->output_count) +
          tx->output.len +
          (tx->flag ? tx->witnesses.len : 0) +
          BTC_TX_LOCKTIME_SIZE_BYTES);
}

// Converts a btc transaction into a serialized transaction
// WARNING: You need to free dst pointer after using this function!
in3_ret_t btc_serialize_tx(in3_req_t* req, const btc_tx_t* tx, bytes_t* dst) {
  if (!tx || !dst) return req_set_error(req, "ERROR: in btc_serialize_tx: Arguments cannot be null", IN3_EINVAL);
  if (tx->input.len == 0 || tx->output.len == 0) return req_set_error(req, "ERROR: in btc_serialize_tx: Transaction inputs or outputs missing", IN3_EINVAL);
  if (tx->flag && tx->witnesses.len == 0) return req_set_error(req, "ERROR: in btc_serialize_tx: Missing witness data in a segwit transaction", IN3_EINVAL);

  // calculate transaction size in bytes
  uint32_t tx_size = btc_get_raw_tx_size(tx);

  dst->data = dst->data ? _realloc(dst->data, tx_size, dst->len) : _malloc(tx_size);
  dst->len  = tx_size;

  // Serialize transaction data
  uint32_t index = 0;

  // version
  uint_to_le(dst, index, tx->version);
  index += BTC_TX_VERSION_SIZE_BYTES;

  // Check if transaction uses SegWit
  if (tx->flag) {
    dst->data[index++] = 0;
    dst->data[index++] = 1;
  }

  // input_count
  long_to_compact_uint(dst, index, tx->input_count);
  index += get_compact_uint_size(tx->input_count);

  // inputs
  memcpy(dst->data + index, tx->input.data, tx->input.len);
  index += tx->input.len;

  // output_count
  long_to_compact_uint(dst, index, tx->output_count);
  index += get_compact_uint_size(tx->output_count);

  // outputs
  memcpy(dst->data + index, tx->output.data, tx->output.len);
  index += tx->output.len;

  // witnesses
  if (tx->flag) {
    memcpy(dst->data + index, tx->witnesses.data, tx->witnesses.len);
    index += tx->output.len;
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

btc_stype_t btc_get_script_type(const bytes_t* script) {
  if ((!script->data) || (script->len > MAX_SCRIPT_SIZE_BYTES)) {
    return BTC_UNSUPPORTED;
  }

  btc_stype_t script_type = BTC_NON_STANDARD;
  uint32_t    len         = script->len;
  uint8_t*    p           = script->data;

  if ((len == (uint32_t) p[0] + 2) && (p[0] == BTC_COMP_PUB_KEY_SIZE_BYTES || p[0] == BTC_UNCOMP_PUB_KEY_SIZE_BYTES) && (p[len - 1] == OP_CHECKSIG)) {
    // locking script has format: PUB_KEY_LEN(1) PUB_KEY(33 or 65 bytes) OP_CHECKSIG(1)
    script_type = BTC_P2PK;
  }
  else if ((len == 25) && (p[0] == OP_DUP) && (p[1] == OP_HASH160) && (p[2] == 0x14) && (p[len - 2] == OP_EQUALVERIFY) && (p[len - 1] == OP_CHECKSIG)) {
    // locking script has format: OP_DUP(1) OP_HASH160(1) PUB_KEY_HASH_LEN(1) PUB_KEY_HASH(20) OP_EQUALVERIFY(1) OP_CHECKSIG(1)
    script_type = BTC_P2PKH;
  }
  else if ((len == 23) && (p[0] == OP_HASH160) && (p[1] == 0x14) && (p[len - 1] == OP_EQUAL)) {
    // locking script has format: OP_HASH160(1) SCRIPT_HASH_LEN(1) SCRIPT_HASH(20) OP_EQUAL(1)
    script_type = BTC_P2SH;
  }
  else if ((len == 22) && (p[0] == 0) && (p[1] == 0x14)) {
    // locking script has format: OP_0(1) PUB_KEY_HASH_LEN(1) PUB_KEY_HASH(20)
    script_type = BTC_V0_P2WPKH;
  }
  else if ((len == 34) && (p[0] < OP_PUSHDATA1) && (p[1] == 0x20)) {
    // locking script has format: OP_0(1) WITNESS_SCRIPT_HASH_LEN(1) WITNESS_SCRIPT_HASH(32)
    script_type = BTC_P2WSH;
  }
  else if ((p[len - 1] == OP_CHECKMULTISIG) && (p[0] <= p[len - 2])) {
    // locking script has format: M(1) LEN_PK_1(1) PK_1(33 or 65 bytes) ... LEN_PK_N(1) PK_N(33 or 65 bytes) N(1) OP_CHECKMULTISIG
    script_type = (p[len - 2] <= 20) ? BTC_P2MS : BTC_UNSUPPORTED;
  }
  return script_type;
}

in3_ret_t btc_verify_public_key(in3_req_t* req, const bytes_t* public_key) {
  if (((public_key->len == 33) && (public_key->data[0] == 0x2 || public_key->data[0] == 0x3)) ||
      (public_key->len == 65 && public_key->data[0] == 0x4)) {
    return IN3_OK;
  }
  return req_set_error(req, "ERROR: in btc_verif_public_key: Provided public key has invalid data format", IN3_EINVAL);
}

static in3_ret_t add_to_tx(in3_req_t* req, btc_tx_ctx_t* tx_ctx, void* src, btc_tx_field_t field_type) {
  if (!tx_ctx || !src) {
    return req_set_error(req, "ERROR: in add_to_tx: Function arguments cannot be null!", IN3_EINVAL);
  }

  bytes_t  raw_src = NULL_BYTES, *dst;
  uint32_t old_len;
  bool     must_free = false;

  switch (field_type) {
    case BTC_INPUT:
      TRY(btc_serialize_tx_in(req, (btc_tx_in_t*) src, &raw_src))
      old_len             = tx_ctx->tx.input.len;
      dst                 = &tx_ctx->tx.input;
      tx_ctx->input_count = tx_ctx->tx.input_count;
      tx_ctx->tx.input_count++;
      tx_ctx->inputs = tx_ctx->inputs ? _realloc(tx_ctx->inputs, tx_ctx->input_count * sizeof(btc_tx_in_t), (tx_ctx->input_count + 1) * sizeof(btc_tx_in_t)) : _malloc(sizeof(btc_tx_in_t));
      tx_ctx->input_count++;
      must_free = true;
      break;
    case BTC_OUTPUT:
      TRY(btc_serialize_tx_out(req, (btc_tx_out_t*) src, &raw_src))
      old_len = tx_ctx->tx.output.len;
      dst     = &tx_ctx->tx.output;
      tx_ctx->tx.output_count++;
      tx_ctx->outputs = tx_ctx->outputs ? _realloc(tx_ctx->outputs, tx_ctx->output_count * sizeof(btc_tx_out_t), (tx_ctx->output_count + 1) * sizeof(btc_tx_out_t)) : _malloc(sizeof(btc_tx_out_t));
      tx_ctx->output_count++;
      must_free = true;
      break;
    case BTC_WITNESS:
      old_len      = tx_ctx->tx.witnesses.len;
      dst          = &tx_ctx->tx.witnesses;
      raw_src.len  = ((bytes_t*) src)->len;
      raw_src.data = ((bytes_t*) src)->data;
      break;
    default:
      return req_set_error(req, "ERROR: in add_to_tx: Unrecognized transaction field code.", IN3_EINVAL);
  }

  dst->len += raw_src.len;
  if (raw_src.data) {
    dst->data = (dst->data) ? _realloc(dst->data, dst->len, old_len) : _malloc(dst->len);
    memcpy(dst->data + old_len, raw_src.data, raw_src.len);
  }
  else {
    dst->data = NULL;
  }

  if (must_free) {
    _free(raw_src.data);
  }
  return IN3_OK;
}

in3_ret_t btc_add_input_to_tx(in3_req_t* req, btc_tx_ctx_t* tx_ctx, btc_tx_in_t* tx_in) {
  return add_to_tx(req, tx_ctx, tx_in, BTC_INPUT);
}

in3_ret_t btc_add_output_to_tx(in3_req_t* req, btc_tx_ctx_t* tx_ctx, btc_tx_out_t* tx_out) {
  return add_to_tx(req, tx_ctx, tx_out, BTC_OUTPUT);
}

in3_ret_t btc_add_witness_to_tx(in3_req_t* req, btc_tx_ctx_t* tx_ctx, bytes_t* witness) {
  return add_to_tx(req, tx_ctx, witness, BTC_WITNESS);
}

in3_ret_t add_outputs_to_tx(in3_req_t* req, d_token_t* outputs, btc_tx_ctx_t* tx_ctx) {
  uint32_t len = d_len(outputs);
  for (uint32_t i = 0; i < len; i++) {
    d_token_t* output = d_get_at(outputs, i);
    if (!output) return req_set_error(req, "ERROR: Transaction output data is missing", IN3_EINVAL);
    const char* script_string = d_string(d_get(output, key("script")));
    if (!script_string) return req_set_error(req, "ERROR: Transaction output script is missing", IN3_EINVAL);
    uint64_t value = d_get_long(output, key("value"));

    btc_tx_out_t tx_out;
    uint32_t     script_len = strlen(script_string) / 2;
    bytes_t      script     = bytes(_malloc(script_len), script_len);
    hex_to_bytes(script_string, strlen(script_string), script.data, script.len);

    tx_out.script.data = script;
    tx_out.script.type = btc_get_script_type(&tx_out.script.data);
    tx_out.value       = value;

    TRY_FINAL(btc_add_output_to_tx(req, tx_ctx, &tx_out), _free(script.data);)
  }
  return IN3_OK;
}

static void add_account_pub_key_to_utxo(btc_utxo_t* utxo, btc_account_pub_key_t* acc_pk) {
  size_t current_size                  = utxo->accounts_count * sizeof(btc_account_pub_key_t);
  size_t new_size                      = current_size + sizeof(btc_account_pub_key_t);
  utxo->accounts                       = utxo->accounts ? _realloc(utxo->accounts, new_size, current_size) : _malloc(new_size);
  utxo->accounts[utxo->accounts_count] = *acc_pk;
  utxo->accounts_count++;
}

static in3_ret_t btc_fill_utxo(btc_utxo_t* utxo, d_token_t* utxo_input) {
  if (!utxo || !utxo_input) return IN3_EINVAL;

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

  // Write the values we have
  btc_init_utxo(utxo);
  utxo->tx_hash            = tx_hash.data;
  utxo->tx_index           = tx_index;
  utxo->tx_out.value       = value;
  utxo->tx_out.script.data = tx_script;
  utxo->tx_out.script.type = btc_get_script_type(&tx_script);
  utxo->raw_script.type    = utxo->tx_out.script.type;

  return IN3_OK;
}

static in3_ret_t handle_utxo_arg(btc_utxo_t* utxo, d_token_t* arg) {
  if (!utxo || !arg) return IN3_EINVAL;
  btc_stype_t type = utxo->tx_out.script.type;

  if (type == BTC_P2SH || type == BTC_P2WSH) {
    // is the argument defining an unlocking script?
    const char* script_str = d_string(d_get(arg, key("script")));
    if (script_str) {
      uint32_t script_len   = (strlen(script_str) >> 1);
      utxo->raw_script.data = bytes(_malloc(script_len), script_len);
      hex_to_bytes(script_str, -1, utxo->raw_script.data.data, utxo->raw_script.data.len);
      utxo->raw_script.type = btc_get_script_type(&utxo->raw_script.data);
    }
    else {
      return IN3_EINVAL;
    }
  }

  if (type == BTC_P2MS || type == BTC_P2SH || type == BTC_P2WSH) {
    // is the argument defining a new "account<->pub_key" pair?
    d_token_t*  acc         = d_get(arg, key("account"));
    const char* pub_key_str = d_string(d_get(arg, key("pub_key")));
    if (acc && pub_key_str) {
      bytes_t               b = d_bytes(acc);
      btc_account_pub_key_t acc_pk;
      acc_pk.account.len  = b.len;
      acc_pk.account.data = _malloc(b.len);
      memcpy(acc_pk.account.data, b.data, b.len);

      acc_pk.pub_key.len  = (strlen(pub_key_str) >> 1);
      acc_pk.pub_key.data = _malloc(acc_pk.pub_key.len);
      hex_to_bytes(pub_key_str, -1, acc_pk.pub_key.data, acc_pk.pub_key.len);

      add_account_pub_key_to_utxo(utxo, &acc_pk);
    }
    else {
      return IN3_EINVAL;
    }
  }

  return IN3_OK;
}

// WARNING: You must free selected_utxos pointer after calling this function, as well as the pointed utxos tx_hash and tx_out.data fields
// TODO: Currently we are adding all utxo_inputs to the list of selected_utxos. Implement an algorithm to select only the necessary utxos for the transaction, given the outputs.
in3_ret_t btc_prepare_utxos(in3_req_t* req, btc_tx_ctx_t* tx_ctx, btc_account_pub_key_t* default_acc_pk, d_token_t* utxo_inputs, d_token_t* args) {
  if (!tx_ctx) return req_set_error(req, "ERROR: in btc_prepare_utxos: transaction context cannot be null", IN3_EINVAL);

  tx_ctx->utxo_count = d_len(utxo_inputs);
  tx_ctx->utxos      = _malloc(tx_ctx->utxo_count * sizeof(btc_utxo_t));

  // Read and initialize each utxo we need for the transaction
  // TODO: Only add the necessary utxos to selected_utxos
  for (uint32_t i = 0; i < tx_ctx->utxo_count; i++) {
    btc_utxo_t utxo;
    d_token_t* utxo_input = d_get_at(utxo_inputs, i);
    TRY(btc_fill_utxo(&utxo, utxo_input))
    if (utxo.tx_out.script.type == BTC_UNSUPPORTED) {
      return req_set_error(req, "ERROR: in btc_prepare_utxos: utxo script type is not supported", IN3_ENOTSUP);
    }
    // finally, add utxo to context
    tx_ctx->utxos[i] = utxo;
  }

  // Handle optional arguments
  if (args) {
    uint32_t args_len = d_len(args);
    for (uint32_t i = 0; i < args_len; i++) {
      d_token_t*  arg        = d_get_at(args, i);
      uint32_t    utxo_index = d_get_long(arg, key("utxo_index"));
      btc_utxo_t* utxo       = &tx_ctx->utxos[utxo_index];

      TRY(handle_utxo_arg(utxo, arg))
    }
  }

  // Now that all optional arguments were parsed, we fill the last remaining
  // fields into our utxo data
  for (uint32_t i = 0; i < tx_ctx->utxo_count; i++) {
    btc_utxo_t* utxo = &tx_ctx->utxos[i];
    btc_stype_t type = utxo->tx_out.script.type;

    if (type == BTC_P2SH || type == BTC_P2WSH) {
      // argument containing unhashed script should have been provided
      // otherwise it is impossible to obtain a signature
      if (!utxo->raw_script.data.len) {
        return req_set_error(req, "ERROR: in btc_prepare_utxos: utxo unhashed script not provided in P2SH or P2WSH transaction", IN3_ENOTSUP);
      }
      type = utxo->raw_script.type; // get the type of the unhashed script instead
    }
    else {
      utxo->raw_script = utxo->tx_out.script;
    }

    // how many signatures do we need to unlock the utxo?
    if (type == BTC_P2MS) {
      utxo->req_sigs = utxo->raw_script.data.data[1];
    }
    else {
      utxo->req_sigs = 1;
    }

    // Guarantee every utxo has at least one account<->pub_key pair assigned to it
    if (!utxo->accounts) {
      add_account_pub_key_to_utxo(utxo, default_acc_pk);
    }
  }

  return IN3_OK;
}

in3_ret_t btc_set_segwit(btc_tx_ctx_t* tx_ctx) {
  tx_ctx->tx.flag = 0;
  for (uint32_t i = 0; i < tx_ctx->utxo_count; i++) {
    if (tx_ctx->utxos[i].tx_out.script.data.data[0] < OP_PUSHDATA1) {
      tx_ctx->tx.flag = 1;
      break;
    }
  }
  return IN3_OK;
}
