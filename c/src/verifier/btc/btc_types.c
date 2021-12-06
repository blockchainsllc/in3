#include "btc_types.h"
#include "../../core/client/request.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "btc_script.h"
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
// TODO: Error handling for null tx and dst pointers
in3_ret_t btc_serialize_tx(const btc_tx_t* tx, bytes_t* dst) {
  // calculate transaction size in bytes
  uint32_t tx_size;
  tx_size = btc_get_raw_tx_size(tx);

  if (!dst->data) {
    dst->data = _calloc(tx_size, 1);
    dst->len  = tx_size;
  }
  else if (dst->len < tx_size) {
    dst->data = _realloc(dst->data, tx_size, dst->len);
    dst->len  = tx_size;
  }

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
  memcpy(dst->data + index, tx->input.data, tx->input.len);
  index += tx->input.len;
  // output_count
  long_to_compact_uint(dst, index, tx->output_count);
  index += get_compact_uint_size(tx->output_count);
  // outputs
  // TODO: serialize struct if tx_out is not null
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

alg_t btc_get_script_type(const bytes_t* script) {
  if ((!script->data) || (script->len < 21) || (script->len > MAX_SCRIPT_SIZE_BYTES)) {
    return UNSUPPORTED;
  }

  alg_t    script_type = NON_STANDARD;
  uint32_t len         = script->len;
  uint8_t* p           = script->data;

  if ((len == (uint32_t) p[0] + 2) && (p[0] == 33 || p[0] == 65) && (p[len - 1] == OP_CHECKSIG)) {
    // locking script has format: PUB_KEY_LEN(1) PUB_KEY(33 or 65 bytes) OP_CHECKSIG(1)
    script_type = P2PK;
  }
  else if ((len == 25) && (p[0] == OP_DUP) && (p[1] == OP_HASH160) && (p[2] == 0x14) && (p[len - 2] == OP_EQUALVERIFY) && (p[len - 1] == OP_CHECKSIG)) {
    // locking script has format: OP_DUP(1) OP_HASH160(1) PUB_KEY_HASH_LEN(1) PUB_KEY_HASH(20) OP_EQUALVERIFY(1) OP_CHECKSIG(1)
    script_type = P2PKH;
  }
  else if ((len == 23) && (p[0] == OP_HASH160) && (p[1] == 0x14) && (p[len - 1] == OP_EQUAL)) {
    // locking script has format: OP_HASH160(1) SCRIPT_HASH_LEN(1) SCRIPT_HASH(20) OP_EQUAL(1)
    script_type = P2SH;
  }
  else if ((len == 22) && (p[0] == 0) && (p[1] == 0x14)) {
    // locking script has format: OP_0(1) PUB_KEY_HASH_LEN(1) PUB_KEY_HASH(20)
    script_type = V0_P2WPKH;
  }
  else if ((len == 34) && (p[0] < OP_PUSHDATA1) && (p[1] == 0x20)) {
    // locking script has format: OP_0(1) WITNESS_SCRIPT_HASH_LEN(1) WITNESS_SCRIPT_HASH(32)
    script_type = P2WSH;
  }
  else if ((p[len - 1] == OP_CHECKMULTISIG) && (p[0] <= p[len - 2])) {
    // locking script has format: M(1) LEN_PK_1(1) PK_1(33 or 65 bytes) ... LEN_PK_N(1) PK_N(33 or 65 bytes) N(1) OP_CHECKMULTISIG
    script_type = (p[len - 2] <= 20) ? BARE_MULTISIG : UNSUPPORTED;
  }
  return script_type;
}

static in3_ret_t add_to_tx(in3_req_t* req, btc_tx_t* tx, void* src, btc_tx_field_t field_type) {
  if (!tx || !src) {
    return req_set_error(req, "ERROR: in add_to_tx: Function arguments cannot be null!", IN3_EINVAL);
  }

  bytes_t  raw_src = NULL_BYTES, *dst;
  uint32_t old_len;
  bool     must_free = false;

  switch (field_type) {
    case BTC_INPUT:
      btc_serialize_tx_in(req, (btc_tx_in_t*) src, &raw_src);
      old_len = tx->input.len;
      dst     = &tx->input;
      tx->input_count++;
      must_free = true;
      break;
    case BTC_OUTPUT:
      btc_serialize_tx_out((btc_tx_out_t*) src, &raw_src);
      old_len = tx->output.len;
      dst     = &tx->output;
      tx->output_count++;
      must_free = true;
      break;
    case BTC_WITNESS:
      old_len      = tx->witnesses.len;
      dst          = &tx->witnesses;
      raw_src.len  = ((bytes_t*) src)->len;
      raw_src.data = ((bytes_t*) src)->data;
      break;
    default:
      // TODO: Implement better error handling
      return req_set_error(req, "Unrecognized transaction field code. No action was performed", IN3_EINVAL);
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
    if (!output) return req_set_error(req, "ERROR: Transaction output data is missing", IN3_EINVAL);
    const char* script_string = d_string(d_get(output, key("script")));
    if (!script_string) return req_set_error(req, "ERROR: Transaction output script is missing", IN3_EINVAL);
    uint64_t value = d_get_long(output, key("value"));

    btc_tx_out_t tx_out;
    uint32_t     script_len = strlen(script_string) / 2;
    bytes_t      script     = bytes(_malloc(script_len), script_len);
    hex_to_bytes(script_string, strlen(script_string), script.data, script.len);

    tx_out.script = script;
    tx_out.value  = value;

    TRY_FINAL(add_output_to_tx(req, tx, &tx_out), _free(script.data);)
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

// WARNING: You must free selected_utxos pointer after calling this function, as well as the pointed utxos tx_hash and tx_out.data fields
// TODO: Currently we are adding all utxo_inputs to the list of selected_utxos. Implement an algorithm to select only the necessary utxos for the transaction, given the outputs.
in3_ret_t btc_prepare_utxos(in3_req_t* req, const btc_tx_t* tx, btc_account_pub_key_t* default_acc_pk, d_token_t* utxo_inputs, d_token_t* args, btc_utxo_t** selected_utxos, uint32_t* len) {
  UNUSED_VAR(tx);
  UNUSED_VAR(req);

  *len            = d_len(utxo_inputs);
  *selected_utxos = _malloc(*len * sizeof(btc_utxo_t));

  // Read and initialize each utxo we need for the transaction
  // TODO: Only add the necessary utxos to selected_utxos
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

    // Write the values we already have
    utxo.tx_hash       = tx_hash.data;
    utxo.tx_index      = tx_index;
    utxo.tx_out.value  = value;
    utxo.tx_out.script = tx_script;
    utxo.script_type   = btc_get_script_type(&utxo.tx_out.script);

    // write default values for the other fields. These are not final
    utxo.unlocking_script = NULL_BYTES;
    utxo.accounts_count   = 0;
    utxo.accounts         = NULL;
    utxo.signatures       = NULL;
    utxo.sig_count        = 0;

    // Finally, add initialized utxo to our list
    *selected_utxos[i] = utxo;
  }

  // Handle optional arguments
  if (args) {
    uint32_t args_len = d_len(args);
    for (uint32_t i = 0; i < args_len; i++) {
      d_token_t* arg        = d_get_at(args, i);
      uint32_t   utxo_index = d_get_long(arg, key("utxo_index"));

      btc_utxo_t* utxo = selected_utxos[utxo_index];
      alg_t       type = utxo->script_type;

      if (type == P2SH || type == P2WSH) {
        // is the argument defining an unlocking script?
        const char* script_str = d_string(d_get(arg, key("script")));
        if (script_str) {
          utxo->unlocking_script.len  = (strlen(script_str) >> 1);
          utxo->unlocking_script.data = _malloc(utxo->unlocking_script.len);
          hex_to_bytes(script_str, -1, utxo->unlocking_script.data, utxo->unlocking_script.len);
        }
      }

      if (type == BARE_MULTISIG || type == P2SH || type == P2WSH) {
        // is the argument defining a new "account<->pub_key" pair?
        d_token_t*  acc         = d_get(arg, key("account"));
        const char* pub_key_str = d_string(d_get(arg, key("pub_key")));
        if (acc && pub_key_str) {
          btc_account_pub_key_t acc_pk;
          acc_pk.account.len  = acc->len;
          acc_pk.account.data = _malloc(acc->len);
          memcpy(acc_pk.account.data, acc->data, acc->len);

          acc_pk.pub_key.len  = (strlen(pub_key_str) >> 1);
          acc_pk.pub_key.data = _malloc(acc_pk.pub_key.len);
          hex_to_bytes(pub_key_str, -1, acc_pk.pub_key.data, acc_pk.pub_key.len);

          add_account_pub_key_to_utxo(utxo, &acc_pk);
        }
      }
    }
  }

  // Now that all optional arguments were parsed, we fill the last remaining
  // fields into our utxo data
  for (uint32_t i = 0; i < *len; i++) {
    btc_utxo_t* utxo    = selected_utxos[i];
    alg_t       subtype = (utxo->script_type == P2SH || utxo->script_type == P2WSH) ? btc_get_script_type(&utxo->unlocking_script) : utxo->script_type;

    // how many signatures do we need to unlock th utxo?
    if (subtype == BARE_MULTISIG) {
      utxo->req_sigs = (utxo->unlocking_script.len > 0) ? utxo->unlocking_script.data[1] : utxo->tx_out.script.data[1];
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

in3_ret_t btc_set_segwit(btc_tx_t* tx, const btc_utxo_t* selected_utxo_list, const uint32_t utxo_list_len) {
  tx->flag = 0;
  for (uint32_t i = 0; i < utxo_list_len; i++) {
    if (selected_utxo_list[i].tx_out.script.data[0] < OP_PUSHDATA1) {
      tx->flag = 1;
      break;
    }
  }
  return IN3_OK;
}
