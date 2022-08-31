#include "btc_sign.h"
#include "../../core/client/plugin.h"
#include "../../core/client/request.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/crypto.h"
#include "../../core/util/log.h"
#include "btc_script.h"
#include "btc_serialize.h"
#include "btc_types.h"

// WARNING: You need to free hash_message.data after calling this function!
// static in3_ret_t build_tx_in_hash_msg(in3_req_t* req, bytes_t* hash_message, const btc_tx_t* tx, const btc_utxo_t* utxo_list, const uint32_t utxo_list_len, const uint32_t utxo_index, const uint8_t sighash) {
static in3_ret_t build_tx_in_hash_msg(in3_req_t* req, bytes_t* hash_message, const btc_tx_ctx_t* tx_ctx, const uint32_t utxo_index, const uint8_t sighash) {
  if (!hash_message || !tx_ctx || !tx_ctx->utxos) {
    return req_set_error(req, "ERROR: in build_tx_in_hash_msg: Function arguments cannot be null", IN3_EINVAL);
  }

  btc_stype_t script_type = tx_ctx->utxos[utxo_index].tx_out.script.type;
  switch (script_type) {
    case BTC_P2MS:
    case BTC_P2SH:
    case BTC_P2PK:
    case BTC_P2PKH: {
      TRY(btc_serialize_tx(req, &tx_ctx->tx, hash_message)); // write serialized transaction
      if (hash_message->len) {
        hash_message->data = _realloc(hash_message->data, hash_message->len + BTC_TX_IN_SIGHASH_SIZE_BYTES, hash_message->len); // Allocate memory for appending sighash
        hash_message->len += BTC_TX_IN_SIGHASH_SIZE_BYTES;
        uint_to_le(hash_message, hash_message->len - BTC_TX_IN_SIGHASH_SIZE_BYTES, sighash); // write sighash at the end of the input
      }
      else {
        return req_set_error(req, "ERROR: in build_tx_in_hash_msg: Failed to build transaction signing message", IN3_EUNKNOWN);
      }
    } break;
    case BTC_P2WSH:
    case BTC_V0_P2WPKH: {
      bytes_t prev_outputs, sequence;
      uint8_t hash_prev_outputs[BTC_TX_HASH_SIZE_BYTES],
          hash_sequence[BTC_TX_HASH_SIZE_BYTES],
          hash_outputs[BTC_TX_HASH_SIZE_BYTES];

      prev_outputs.len = tx_ctx->utxo_count * BTC_TX_IN_PREV_OUTPUT_SIZE_BYTES;
      sequence.len     = tx_ctx->utxo_count * BTC_TX_IN_SEQUENCE_SIZE_BYTES;

      prev_outputs.data = _malloc(prev_outputs.len);
      sequence.data     = _malloc(sequence.len);

      for (uint32_t i = 0; i < tx_ctx->utxo_count; i++) {
        rev_copy(prev_outputs.data + (BTC_TX_IN_PREV_OUTPUT_SIZE_BYTES * i), tx_ctx->utxos[i].tx_hash);
        rev_copyl(prev_outputs.data + (BTC_TX_HASH_SIZE_BYTES * i), bytes((uint8_t*) &tx_ctx->utxos[i].tx_index, BTC_TX_IN_SEQUENCE_SIZE_BYTES), BTC_TX_IN_SEQUENCE_SIZE_BYTES);
        rev_copyl(sequence.data + (BTC_TX_IN_SEQUENCE_SIZE_BYTES * i), bytes((uint8_t*) &tx_ctx->utxos[i].sequence, BTC_TX_IN_SEQUENCE_SIZE_BYTES), BTC_TX_IN_SEQUENCE_SIZE_BYTES);
      }

      if (!(sighash & BTC_SIGHASH_ANYONECANPAY)) {
        btc_hash(prev_outputs, hash_prev_outputs);
        if ((sighash & 0x1f) != BTC_SIGHASH_ALL) {
          btc_hash(sequence, hash_sequence);
        }
      }

      if ((sighash & 0x1f) == BTC_SIGHASH_ALL) {
        btc_hash(tx_ctx->tx.output, hash_outputs);
      }
      else if (((sighash & 0x1f) != BTC_SIGHASH_SINGLE) && utxo_index < tx_ctx->tx.output_count) {
        // TODO: Implement support for sighashes other than SIGHASH_ALL
        // In pseudo-code, this is what should happen:
        // -- serialized_outputs = little_endian(outputs[utxo_index].value, 8_bytes)
        // -- serialized_outputs.append(outputs[utxo_index].script)
        // -- hash_outputs = dsha256(serialized_outputs)
      }

      // Build message for hashing and signing:
      // version | hash_prev_outputs | hash_sequence | prev_tx | prev_tx_index | utxo_script | utxo_value | input_sequence | hash_outputs | locktime | sighash
      uint32_t index    = 0;
      hash_message->len = (BTC_TX_VERSION_SIZE_BYTES +
                           BTC_TX_HASH_SIZE_BYTES +
                           BTC_TX_HASH_SIZE_BYTES +
                           BTC_TX_IN_PREV_OUTPUT_SIZE_BYTES +
                           get_compact_uint_size((uint64_t) utxo_index) +
                           tx_ctx->utxos[utxo_index].tx_out.script.data.len +
                           BTC_TX_OUT_VALUE_SIZE_BYTES +
                           BTC_TX_IN_SEQUENCE_SIZE_BYTES +
                           BTC_TX_HASH_SIZE_BYTES +
                           BTC_TX_LOCKTIME_SIZE_BYTES +
                           BTC_TX_IN_SIGHASH_SIZE_BYTES);

      hash_message->data = _calloc(hash_message->len, 1);
      uint8_t* d         = hash_message->data;
      uint_to_le(hash_message, index, tx_ctx->tx.version);
      index += BTC_TX_VERSION_SIZE_BYTES;
      memcpy(d + index, hash_prev_outputs, BTC_TX_HASH_SIZE_BYTES);
      index += BTC_TX_HASH_SIZE_BYTES;
      memcpy(d + index, hash_sequence, BTC_TX_HASH_SIZE_BYTES);
      index += BTC_TX_HASH_SIZE_BYTES;
      rev_copyl(d + index, bytes(tx_ctx->utxos[utxo_index].tx_hash, BTC_TX_HASH_SIZE_BYTES), BTC_TX_HASH_SIZE_BYTES);
      index += BTC_TX_HASH_SIZE_BYTES;
      uint_to_le(hash_message, index, tx_ctx->utxos[utxo_index].tx_index);
      index += BTX_TX_INDEX_SIZE_BYTES;
      memcpy(d + index, tx_ctx->utxos[utxo_index].tx_out.script.data.data, tx_ctx->utxos[utxo_index].tx_out.script.data.len);
      index += tx_ctx->utxos[utxo_index].tx_out.script.data.len;
      long_to_le(hash_message, index, tx_ctx->utxos[utxo_index].tx_out.value);
      index += BTC_TX_OUT_VALUE_SIZE_BYTES;
      uint_to_le(hash_message, index, tx_ctx->utxos[utxo_index].sequence);
      index += BTC_TX_IN_SEQUENCE_SIZE_BYTES;
      memcpy(d + index, hash_outputs, BTC_TX_HASH_SIZE_BYTES);
      index += BTC_TX_HASH_SIZE_BYTES;
      memcpy(d + index, (uint8_t*) &tx_ctx->tx.lock_time, BTC_TX_LOCKTIME_SIZE_BYTES);
      index += BTC_TX_LOCKTIME_SIZE_BYTES;
      uint_to_le(hash_message, index, sighash);

      _free(prev_outputs.data);
      _free(sequence.data);
    } break;
    default:
      return req_set_error(req, "ERROR: utxo script type is non-standard or unsupported", IN3_EINVAL);
  }
  return IN3_OK;
}

// WARNING: You need to free tx_in.script.data after calling this function! Also, you may need to free witness.script.data depending on script_type value
static in3_ret_t build_unlocking_script(in3_req_t* req, btc_tx_in_t* tx_in, bytes_t* witness, const btc_utxo_t* utxo) {
  if (!utxo) {
    return req_set_error(req, "ERROR: in build_unlocking_script: utxo cannot be null.", IN3_EINVAL);
  }
  if (!utxo->signatures || !(utxo->signatures) || utxo->sig_count == 0) {
    return req_set_error(req, "ERROR: in build_unlocking_script: must provide at least one signature.", IN3_EINVAL);
  }
  if (!tx_in) {
    return req_set_error(req, "ERROR: in build_unlocking_script: tx_in missing.", IN3_EINVAL);
  }

  if (utxo->tx_out.script.type == BTC_P2SH || utxo->tx_out.script.type == BTC_P2WSH) {
    if (utxo->raw_script.data.len == 0) {
      return req_set_error(req, "ERROR: in build_unlocking_script: trying to redeem a P2SH or P2WSH utxo without providing a valid script.", IN3_EINVAL);
    }
    if (utxo->tx_out.script.type == BTC_P2SH && utxo->raw_script.data.len > MAX_P2SH_SCRIPT_SIZE_BYTES) {
      char message[100];
      sprintf(message, "ERROR: in build_unlocking_script: provided redeem script is bigger than the %d bytes limit.", MAX_P2SH_SCRIPT_SIZE_BYTES);
      return req_set_error(req, message, IN3_EINVAL);
    }
    if (utxo->raw_script.data.len > MAX_SCRIPT_SIZE_BYTES) {
      char message[100];
      sprintf(message, "ERROR: in build_unlocking_script: provided redeem script is bigger than the %d bytes limit.", MAX_SCRIPT_SIZE_BYTES);
      return req_set_error(req, message, IN3_EINVAL);
    }
  }

  if (!witness && (utxo->tx_out.script.type == BTC_V0_P2WPKH || utxo->tx_out.script.type == BTC_P2WSH)) {
    return req_set_error(req, "ERROR: in build_unlocking_script: witness missing.", IN3_EINVAL);
  }

  bytes_t **signatures = (bytes_t * *const) &(utxo->signatures), *pub_key = &(utxo->signers[0].pub_key), *unlocking_script = NULL, num_elements = NULL_BYTES;
  switch (utxo->tx_out.script.type) {
    case BTC_P2PK: {
      // Unlocking script format is: DER_SIG_LEN|DER_SIG
      tx_in->script.data.data    = tx_in->script.data.data ? _realloc(tx_in->script.data.data, signatures[0]->len + 1, tx_in->script.data.len) : _malloc(signatures[0]->len + 1);
      tx_in->script.data.len     = signatures[0]->len + 1;
      tx_in->script.data.data[0] = (uint8_t) signatures[0]->len;
      memcpy(tx_in->script.data.data + 1, signatures[0]->data, signatures[0]->len);
    } break;
    case BTC_P2PKH: {
      // Unlocking script format is: DER_LEN|DER_SIG|PUB_KEY_LEN|PUB_BEY
      uint32_t script_len = 1 + signatures[0]->len + 1 + pub_key->len; // DER_SIG_LEN + DER_SIG + PUBKEY_LEN + PUBKEY

      tx_in->script.data.data = tx_in->script.data.data ? _realloc(tx_in->script.data.data, script_len, tx_in->script.data.len) : _malloc(script_len);
      tx_in->script.data.len  = script_len;

      bytes_t* b     = &tx_in->script.data;
      uint32_t index = 0;

      b->data[index++] = (uint8_t) signatures[0]->len;                  // write DER_SIG_LEN field
      memcpy(b->data + index, signatures[0]->data, signatures[0]->len); // write DER_SIG field
      index += signatures[0]->len;
      b->data[index++] = (uint8_t) pub_key->len;            // write PUB_KEY_LEN field
      memcpy(b->data + index, pub_key->data, pub_key->len); // write PUB_KEY field
    } break;
    case BTC_V0_P2WPKH: {
      // Unlocking script format is: NUM_ELEMENTS | DER_SIG_LEN | DER_SIG | PUB_KEY_LEN | PUB_KEY
      // tx_in script field should be empty. Unlocking script will be written to witness instead

      // clean tx_in script
      tx_in->script.data.len = 0;

      uint32_t script_len = 1 + 1 + signatures[0]->len + 1 + pub_key->len; // NUM_ELEMENTS + DER_SIG_LEN + DER_SIG + PUB_KEY_LEN + PUB_KEY
      witness->data       = witness->data ? _realloc(witness->data, script_len, witness->len) : _malloc(script_len);
      witness->len        = script_len;
      uint32_t index      = 0;

      witness->data[index++] = 2;                                             // write NUM_ELEMENTS
      witness->data[index++] = (uint8_t) signatures[0]->len;                  // write DER_SIG_LEN
      memcpy(witness->data + index, signatures[0]->data, signatures[0]->len); // write DER_SIG
      index += signatures[0]->len;
      witness->data[index++] = pub_key->len;                      // write PUB_KEY_LEN
      memcpy(witness->data + index, pub_key->data, pub_key->len); // write PUB_KEY
    } break;
    case BTC_P2MS: {
      // Unlocking script format is: ZERO_BYTE | SIG_1_LEN | SIG_1 | SIG_2_LEN | SIG_2 | .....
      // Zero byte is present to remedy a bug in OP_CHECKMULTISIG which makes it read one more input
      // than it should.
      uint32_t req_sigs      = (utxo->req_sigs > utxo->sig_count) ? utxo->sig_count : utxo->req_sigs; // get how many signatures are required from the locking script
      tx_in->script.data.len = 0;                                                                     // cleanup tx_in script to receive unlocking script
      bytes_t zero_byte      = {alloca(1), 1};
      zero_byte.data[0]      = 0x0;
      append_bytes(&tx_in->script.data, &zero_byte);
      for (uint32_t i = 0; i < req_sigs; i++) {
        bytes_t sig_field;
        sig_field.len     = utxo->signatures->len + 1;
        sig_field.data    = alloca(sig_field.len);
        sig_field.data[0] = utxo->signatures->len;
        memcpy(sig_field.data + 1, utxo->signatures->data, utxo->signatures->len);
        append_bytes(&tx_in->script.data, &sig_field);
      }
    } break;
    case BTC_P2WSH:
    case BTC_P2SH:
      tx_in->script.data.len = 0;
      if (utxo->tx_out.script.type == BTC_P2WSH) {
        // witness:         NUM_ELEMENTS | ZERO_BYTE | SIG_1_LEN | SIG_1 | ... | SIG_N_LEN | SIG_N | VALID_BTC_SCRIPT
        // tx_in_script:    (empty)
        num_elements.len     = 1;
        num_elements.data    = alloca(1);
        num_elements.data[0] = utxo->req_sigs + 2;
        unlocking_script     = witness;
        append_bytes(unlocking_script, &num_elements);
      }
      else {
        // Unlocking script has format: SIG_1_LEN | SIG_1 | ... | SIG_N_LEN | SIG_N | VALID_BTC_SCRIPT
        unlocking_script = &tx_in->script.data;
      }
      if (utxo->req_sigs > 1) {
        bytes_t zero_byte = {alloca(1), 1};
        zero_byte.data[0] = 0x0;
        append_bytes(unlocking_script, &zero_byte);
      }
      for (uint32_t i = 0; i < utxo->req_sigs; i++) {
        bytes_t sig_field = {alloca(signatures[i]->len + 1), signatures[i]->len + 1};
        sig_field.data[0] = signatures[i]->len;
        memcpy(sig_field.data + 1, signatures[i]->data, signatures[i]->len);
        append_bytes(unlocking_script, &sig_field);
      }
      append_bytes(unlocking_script, &utxo->raw_script.data);
      break;
    default:
      return req_set_error(req, "ERROR: utxo script type is non-standard or unsupported", IN3_EINVAL);
  }
  return IN3_OK;
}

// Fill tx_in fields, preparing the input for signing
// WARNING: You need to free tx_in->prev_tx_hash after calling this function
static in3_ret_t prepare_tx_in(in3_req_t* req, const btc_utxo_t* utxo, btc_tx_in_t* tx_in) {
  if (!utxo || !tx_in || !utxo->tx_out.script.data.data) {
    // TODO: Implement better error treatment
    return req_set_error(req, "ERROR: in prepare_tx_in: function arguments can not be null!", IN3_EINVAL);
  }

  tx_in->prev_tx_index = utxo->tx_index;
  tx_in->prev_tx_hash  = _malloc(32);
  memcpy(tx_in->prev_tx_hash, utxo->tx_hash, 32);

  // Before signing, input script field should temporarilly be equal to the utxo we want to redeem
  tx_in->script.type      = utxo->tx_out.script.type;
  tx_in->sequence         = utxo->sequence;
  tx_in->script.data.len  = utxo->tx_out.script.data.len;
  tx_in->script.data.data = _malloc(tx_in->script.data.len);
  memcpy(tx_in->script.data.data, utxo->tx_out.script.data.data, tx_in->script.data.len);
  return IN3_OK;
}

static void free_tx_ctx_inputs(btc_tx_ctx_t* tx_ctx) {
  for (uint32_t i = 0; i < tx_ctx->input_count; i++) {
    btc_free_tx_in(&tx_ctx->inputs[i]);
  }
  _free(tx_ctx->inputs);
  _free(tx_ctx->tx.input.data);
}

// WARNING: You need to free der_sig.data after calling this function!
in3_ret_t btc_sign_tx_in(in3_req_t* req, bytes_t* der_sig, const btc_tx_ctx_t* tx_ctx, const uint32_t utxo_index, const uint32_t signer_index, const btc_tx_in_t* tx_in, uint8_t sighash, sb_t* sb) {
  if (!tx_ctx || !der_sig || !tx_in) {
    return req_set_error(req, "ERROR: in btc_sign_tx_in: function arguments cannot be NULL.", IN3_ERPC);
  }

  // TODO: Implement support for other sighashes
  if (sighash != BTC_SIGHASH_ALL) {
    return req_set_error(req, "ERROR: in btc_sign_tx_in: Sighash not yet supported.", IN3_ERPC);
  }

  // Generate an unsigned transaction. This will be used to generate the hash provided to
  // the ecdsa signing algorithm

  const btc_tx_t*   tx        = &tx_ctx->tx;
  const bytes_t*    signer_id = &tx_ctx->utxos[utxo_index].signers[signer_index].signer_id;
  const btc_utxo_t* utxos     = tx_ctx->utxos;

  // Create a temporary unsigned transaction with "empty" input data, which will
  // be used to create our signature
  btc_tx_ctx_t tmp_tx;
  btc_init_tx_ctx(&tmp_tx);
  tmp_tx.utxos           = tx_ctx->utxos;
  tmp_tx.tx.flag         = tx->flag;
  tmp_tx.tx.version      = tx->version;
  tmp_tx.tx.output_count = tx->output_count;
  tmp_tx.tx.output.len   = tx->output.len;
  tmp_tx.tx.output.data  = alloca(tmp_tx.tx.output.len);
  memcpy(tmp_tx.tx.output.data, tx->output.data, tmp_tx.tx.output.len);
  tmp_tx.tx.lock_time = tx->lock_time;

  // Include inputs into temporary unsigned tx.
  // The input we want to sign is the only one which should include script data
  btc_tx_in_t tmp_tx_in;
  for (uint32_t i = 0; i < tx_ctx->utxo_count; i++) {
    if (i == utxo_index) {
      tmp_tx_in = *tx_in;
    }
    else {
      tmp_tx_in.prev_tx_hash  = utxos[i].tx_hash;
      tmp_tx_in.prev_tx_index = utxos[i].tx_index;
      tmp_tx_in.sequence      = utxos[i].sequence;
      tmp_tx_in.script.data   = NULL_BYTES;
    }
    btc_add_input_to_tx(req, &tmp_tx, &tmp_tx_in);
  }

  // prepare array for hashing
  bytes_t hash_message = NULL_BYTES;
  TRY_CATCH(build_tx_in_hash_msg(req, &hash_message, &tmp_tx, utxo_index, sighash), free_tx_ctx_inputs(&tmp_tx);)
  // Finally, sign transaction input
  // -- Obtain DER signature
  bytes_t sig = NULL_BYTES;
  int     l;
  TRY_CATCH(req_require_signature(req, SIGN_EC_BTC, SIGN_CURVE_ECDSA, PL_SIGN_BTCTX, &sig, hash_message, *signer_id, req->requests[0], sb), _free(hash_message.data); free_tx_ctx_inputs(&tmp_tx);)
  der_sig->data = _malloc(75);
  TRY_CATCH(crypto_convert(ECDSA_SECP256K1, CONV_SIG65_TO_DER, sig, der_sig->data, &l), _free(der_sig->data); _free(hash_message.data); free_tx_ctx_inputs(&tmp_tx);)
  der_sig->len                  = (uint32_t) l;
  der_sig->data[der_sig->len++] = sighash; // append sighash byte to end of DER signature
  // signature is complete
  _free(hash_message.data);
  free_tx_ctx_inputs(&tmp_tx);
  return IN3_OK;
}

static void add_sig_to_utxo(btc_utxo_t* utxo, const bytes_t* sig) {
  size_t current_size               = utxo->sig_count * sizeof(bytes_t);
  size_t new_size                   = current_size + sizeof(bytes_t);
  utxo->signatures                  = utxo->signatures ? _realloc(utxo->signatures, new_size, current_size) : _malloc(new_size);
  utxo->signatures[utxo->sig_count] = *sig;
  utxo->sig_count++;
}

in3_ret_t btc_sign_tx(in3_req_t* req, btc_tx_ctx_t* tx_ctx, sb_t* sb) {
  if (!tx_ctx->utxos || !tx_ctx->utxo_count) return req_set_error(req, "ERROR: in btc_sign_tx: utxo list cannot be empty or null.", IN3_EINVAL);
  if (!tx_ctx->outputs || !tx_ctx->output_count) return req_set_error(req, "ERROR: in btc_sign_tx: transaction should have at least one output.", IN3_EINVAL);
  if (tx_ctx->inputs || tx_ctx->input_count) return req_set_error(req, "ERROR: in btc_sign_tx: transaction should not already contain input data.", IN3_EINVAL);

  // Cleanup old inputs from transaction
  tx_ctx->tx.input       = NULL_BYTES;
  tx_ctx->tx.input_count = 0;

  // for each selected utxo in a tx:
  for (uint32_t i = 0; i < tx_ctx->utxo_count; i++) {
    // if script type is unknown, try to identify it
    btc_stype_t script_type = (tx_ctx->utxos[i].tx_out.script.type == BTC_UNKNOWN) ? btc_get_script_type(&tx_ctx->utxos[i].tx_out.script.data) : tx_ctx->utxos[i].tx_out.script.type;

    if (!script_is_standard(script_type)) {
      return req_set_error(req, "ERROR: in btc_sign_tx: utxo script is non-standard or unsupported.", IN3_EINVAL);
    }

    // Build new input from utxo
    btc_tx_in_t tx_in;
    bytes_t     witness = NULL_BYTES;
    btc_init_tx_in(&tx_in);
    TRY(prepare_tx_in(req, tx_ctx->utxos + i, &tx_in))

    bool is_segwit = (script_type == BTC_V0_P2WPKH || script_type == BTC_P2WSH);

    // -- for each signature we need to provide:
    for (uint32_t j = 0; j < tx_ctx->utxos[i].req_sigs; j++) {
      bytes_t sig = NULL_BYTES;
      // TODO: select random unused key to sign if multisig
      TRY_CATCH(btc_sign_tx_in(req, &sig, tx_ctx, i, j, &tx_in, BTC_SIGHASH_ALL, sb),
                _free(tx_in.script.data.data);
                _free(tx_in.prev_tx_hash);)
      add_sig_to_utxo(&tx_ctx->utxos[i], &sig);
    }
    // We have the signatures, now write the unlocking script to input
    TRY_CATCH(build_unlocking_script(req, &tx_in, &witness, &tx_ctx->utxos[i]),
              _free(tx_in.script.data.data);
              _free(tx_in.prev_tx_hash);)
    // Add signed input and witness to transaction
    if (is_segwit) {
      TRY_CATCH(btc_add_witness_to_tx(req, tx_ctx, &witness),
                _free(tx_in.script.data.data);
                _free(tx_in.prev_tx_hash);
                _free(witness.data);)
      _free(witness.data);
    }
    TRY_FINAL(btc_add_input_to_tx(req, tx_ctx, &tx_in),
              _free(tx_in.script.data.data);
              _free(tx_in.prev_tx_hash);)
  }
  return IN3_OK;
}