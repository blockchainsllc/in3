#include "btc_sign.h"
#include "../../core/client/plugin.h"
#include "../../core/client/request.h"
#include "../../core/client/request_internal.h"
#include "../../third-party/crypto/secp256k1.h"
#include "btc_script.h"
#include "btc_serialize.h"
#include "btc_types.h"

// copy a byte array in reverse order
static void rev_memcpy(uint8_t* dst, uint8_t* src, uint32_t len) {
  // TODO: Accuse error in case the following statement is false
  if (src && dst) {
    for (uint32_t i = 0; i < len; i++) {
      dst[(len - 1) - i] = src[i];
    }
  }
}

static void append_bytes(bytes_t* dst, const bytes_t* src) {
  if (dst && src && src->len > 0 && src->data) {
    dst->data = (dst->data) ? _realloc(dst->data, dst->len + src->len, dst->len) : _malloc(dst->len + src->len);
    memcpy(dst->data + dst->len, src->data, src->len);
    dst->len += src->len;
  }
}

// WARNING: You need to free hash_message.data after calling this function!
static in3_ret_t build_tx_in_hash_msg(in3_req_t* req, bytes_t* hash_message, const btc_tx_t* tx, const btc_utxo_t* utxo_list, const uint32_t utxo_list_len, const uint32_t utxo_index, const uint8_t sighash, const alg_t script_type) {
  switch (script_type) {
    case BTC_BARE_MULTISIG:
      // Bare multisig hash message will be built same way as P2PK or P2PKH
    case BTC_P2SH:
      // Pay-To-Script-Hash hash message will be built same way as P2PK or P2PKH
    case BTC_P2PK:
    case BTC_P2PKH: {
      hash_message = b_new(NULL, btc_get_raw_tx_size(tx) + 4);
      btc_serialize_tx(tx, hash_message);

      // write sighash (4 bytes) at the end of the input
      uint_to_le(hash_message, hash_message->len - 4, sighash);
    } break;
    case BTC_P2WSH:
      // Pay-To-Witness-Script-Hash hash message will be built same way as V0_P2WPKH
    case BTC_V0_P2WPKH: {
      bytes_t prev_outputs, sequence;
      uint8_t hash_prev_outputs[32], hash_sequence[32], hash_outputs[32];

      prev_outputs.len = utxo_list_len * 36; // 32 bytes tx_hash + 4 bytes tx_index
      sequence.len     = utxo_list_len * 4;

      prev_outputs.data = _malloc(prev_outputs.len);
      sequence.data     = _malloc(sequence.len);

      uint32_t default_sequence = 0xffffffff; // Change this to use provided sequence value once BIP68 is implemented

      for (uint32_t i = 0; i < utxo_list_len; i++) {
        rev_memcpy(prev_outputs.data + (36 * i), utxo_list[i].tx_hash, 32);
        rev_memcpy(prev_outputs.data + (32 * i), (uint8_t*) &utxo_list[i].tx_index, 4);
        rev_memcpy(sequence.data + (4 * i), (uint8_t*) &default_sequence, 4);
      }

      if (!(sighash & BTC_SIGHASH_ANYONECANPAY)) {
        btc_hash(prev_outputs, hash_prev_outputs);
        if ((sighash & 0x1f) != BTC_SIGHASH_ALL) {
          btc_hash(sequence, hash_sequence);
        }
      }

      if ((sighash & 0x1f) == BTC_SIGHASH_ALL) {
        btc_hash(tx->output, hash_outputs);
      }
      else if (((sighash & 0x1f) != BTC_SIGHASH_SINGLE) && utxo_index < tx->output_count) {
        // TODO: Implement support for sighashes other than SIGHASH_ALL
        // In pseudo-code, this is what should happen:
        // -- serialized_outputs = little_endian(outputs[utxo_index].value, 8_bytes)
        // -- serialized_outputs.append(outputs[utxo_index].script)
        // -- hash_outputs = dsha256(serialized_outputs)
      }

      // Build message for hashing and signing:
      // version | hash_prev_outputs | hash_sequence | prev_tx | prev_tx_index | utxo_script | utxo_value | input_sequence | hash_outputs | locktime | sighash
      uint32_t index    = 0;
      hash_message->len = (get_compact_uint_size((uint64_t) utxo_index) +
                           utxo_list[utxo_index].tx_out.script.len +
                           +156);

      hash_message->data = _calloc(hash_message->len, 1);
      uint8_t* d         = hash_message->data;
      uint_to_le(hash_message, index, tx->version);
      index += 4;
      memcpy(d + index, hash_prev_outputs, 32);
      index += 32;
      memcpy(d + index, hash_sequence, 32);
      index += 32;
      rev_memcpy(d + index, utxo_list[utxo_index].tx_hash, 32);
      index += 32;
      uint_to_le(hash_message, index, utxo_list[utxo_index].tx_index);
      index += 4;
      memcpy(d + index, utxo_list[utxo_index].tx_out.script.data, utxo_list[utxo_index].tx_out.script.len);
      index += utxo_list[utxo_index].tx_out.script.len;
      long_to_le(hash_message, index, utxo_list[utxo_index].tx_out.value);
      index += 8;
      uint_to_le(hash_message, index, 0xffffffff); // This is the 'sequence' field. Until BIP 125 sequence fields were unused. TODO: Implement support for BIP 125
      index += 4;
      memcpy(d + index, hash_outputs, 32);
      index += 32;
      memcpy(d + index, (uint8_t*) &tx->lock_time, 4);
      index += 4;
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

  if (utxo->script_type == BTC_P2SH || utxo->script_type == BTC_P2WSH) {
    if (utxo->unlocking_script.len == 0) {
      return req_set_error(req, "ERROR: in build_unlocking_script: trying to redeem a P2SH or P2WSH utxo without providing a valid script.", IN3_EINVAL);
    }
    if (utxo->script_type == BTC_P2SH && utxo->unlocking_script.len > MAX_P2SH_SCRIPT_SIZE_BYTES) {
      char message[100];
      sprintf(message, "ERROR: in build_unlocking_script: provided redeem script is bigger than the %d bytes limit.", MAX_P2SH_SCRIPT_SIZE_BYTES);
      return req_set_error(req, message, IN3_EINVAL);
    }
    if (utxo->unlocking_script.len == MAX_SCRIPT_SIZE_BYTES) {
      char message[100];
      sprintf(message, "ERROR: in build_unlocking_script: provided redeem script is bigger than the %d bytes limit.", MAX_SCRIPT_SIZE_BYTES);
      return req_set_error(req, message, IN3_EINVAL);
    }
  }

  if (!witness && (utxo->script_type == BTC_V0_P2WPKH || utxo->script_type == BTC_P2WSH)) {
    return req_set_error(req, "ERROR: in build_unlocking_script: witness missing.", IN3_EINVAL);
  }

  bytes_t **signatures = (bytes_t * *const) &(utxo->signatures), *pub_key = &(utxo->accounts[0].pub_key), *unlocking_script = NULL, num_elements = NULL_BYTES;
  switch (utxo->script_type) {
    case BTC_P2PK: {
      // Unlocking script format is: DER_SIG_LEN|DER_SIG
      tx_in->script.data    = tx_in->script.data ? _realloc(tx_in->script.data, signatures[0]->len + 1, tx_in->script.len) : _malloc(signatures[0]->len + 1);
      tx_in->script.len     = signatures[0]->len + 1;
      tx_in->script.data[0] = (uint8_t) signatures[0]->len;
      memcpy(tx_in->script.data + 1, signatures[0]->data, signatures[0]->len);
    } break;
    case BTC_P2PKH: {
      // Unlocking script format is: DER_LEN|DER_SIG|PUB_KEY_LEN|PUB_BEY
      uint32_t script_len = 1 + signatures[0]->len + 1 + 64; // DER_SIG_LEN + DER_SIG + PUBKEY_LEN + PUBKEY
      tx_in->script.data  = tx_in->script.data ? _realloc(tx_in->script.data, script_len, tx_in->script.len) : _malloc(script_len);
      tx_in->script.len   = script_len;

      bytes_t* b     = &tx_in->script;
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
      tx_in->script.len = 0;

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
    case BTC_BARE_MULTISIG: {
      // Unlocking script format is: ZERO_BYTE | SIG_1_LEN | SIG_1 | SIG_2_LEN | SIG_2 | .....
      // Zero byte is present to remedy a bug in OP_CHECKMULTISIG which makes it read one more input
      // than it should.
      uint32_t req_sigs = (utxo->req_sigs > utxo->sig_count) ? utxo->sig_count : utxo->req_sigs; // get how many signatures are required from the locking script
      tx_in->script.len = 0;                                                                     // cleanup tx_in script to receive unlocking script
      bytes_t zero_byte = {alloca(1), 1};
      zero_byte.data[0] = 0x0;
      append_bytes(&tx_in->script, &zero_byte);
      for (uint32_t i = 0; i < req_sigs; i++) {
        bytes_t sig_field;
        sig_field.len     = utxo->signatures->len + 1;
        sig_field.data    = alloca(sig_field.len);
        sig_field.data[0] = utxo->signatures->len;
        memcpy(sig_field.data + 1, utxo->signatures->data, utxo->signatures->len);
        append_bytes(&tx_in->script, &sig_field);
      }
    } break;
    case BTC_P2WSH:
    case BTC_P2SH:
      tx_in->script.len = 0;
      if (utxo->script_type == BTC_P2WSH) {
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
        unlocking_script = &tx_in->script;
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
      append_bytes(unlocking_script, &utxo->unlocking_script);
      break;
    default:
      return req_set_error(req, "ERROR: utxo script type is non-standard or unsupported", IN3_EINVAL);
  }
  return IN3_OK;
}

// Fill tx_in fields, preparing the input for signing
// WARNING: You need to free tx_in->prev_tx_hash after calling this function
static in3_ret_t prepare_tx_in(in3_req_t* req, const btc_utxo_t* utxo, btc_tx_in_t* tx_in) {
  if (!utxo || !tx_in) {
    // TODO: Implement better error treatment
    return req_set_error(req, "ERROR: in prepare_tx_in: function arguments can not be null!", IN3_EINVAL);
  }

  tx_in->prev_tx_index = utxo->tx_index;
  tx_in->prev_tx_hash  = _malloc(32);
  memcpy(tx_in->prev_tx_hash, utxo->tx_hash, 32);

  // Before signing, input script field should temporarilly be equal to the utxo we want to redeem
  tx_in->script.len  = utxo->tx_out.script.len;
  tx_in->script.data = _malloc(tx_in->script.len);
  memcpy(tx_in->script.data, utxo->tx_out.script.data, tx_in->script.len);

  tx_in->sequence = 0xffffffff; // Until BIP 125 sequence fields were unused. TODO: Implement support for BIP 125
  return IN3_OK;
}

// WARNING: You need to free der_sig.data after calling this function!
in3_ret_t btc_sign_tx_in(in3_req_t* req, bytes_t* der_sig, const btc_tx_t* tx, const btc_utxo_t* utxo_list, const uint32_t utxo_list_len, const uint32_t utxo_index, const bytes_t* account, const bytes_t* pub_key, const btc_tx_in_t* tx_in, uint8_t sighash) {
  if (!der_sig || !tx_in || !account) {
    return req_set_error(req, "ERROR: in btc_sign_tx_in: function arguments cannot be NULL.", IN3_ERPC);
  }

  // TODO: Implement support for other sighashes
  if (sighash != BTC_SIGHASH_ALL) {
    return req_set_error(req, "ERROR: in btc_sign_tx_in: Sighash not yet supported.", IN3_ERPC);
  }

  if (pub_key->len != 33 && pub_key->len != 65) {
    return req_set_error(req, "ERROR: in btc_sign_tx_in: Public key not supported. BTC public keys should be either compressed (33 bytes) or uncompressed (65 bytes).", IN3_ERPC);
  }
  else if ((pub_key->len == 65 && pub_key->data[0] != 0x4) || (pub_key->len == 33 && pub_key->data[0] != 0x2 && pub_key->data[0] != 0x3)) {
    return req_set_error(req, "ERROR: in btc_sign_tx_in: Invalid public key format", IN3_ERPC);
  }

  // Check for key compression
  bytes_t* uncomp_pub_key = alloca(sizeof(bytes_t));
  uncomp_pub_key->len     = 65;
  uncomp_pub_key->data    = alloca(65);

  if (pub_key->len == 33) {
    ecdsa_uncompress_pubkey(&secp256k1, pub_key->data, uncomp_pub_key->data);
  }
  else {
    memcpy(uncomp_pub_key->data, pub_key->data, uncomp_pub_key->len);
  }

  // Generate an unsigned transaction. This will be used to generate the hash provided to
  // the ecdsa signing algorithm
  btc_tx_t tmp_tx;
  btc_init_tx(&tmp_tx);
  tmp_tx.flag         = tx->flag;
  tmp_tx.version      = tx->version;
  tmp_tx.output_count = tx->output_count;
  tmp_tx.output.len   = tx->output.len;
  tmp_tx.output.data  = alloca(sizeof(uint8_t) * tmp_tx.output.len);
  for (uint32_t i = 0; i < tmp_tx.output.len; i++) tmp_tx.output.data[i] = tx->output.data[i];
  tmp_tx.lock_time = tx->lock_time;

  // Include inputs into unsigned tx
  btc_tx_in_t tmp_tx_in;
  for (uint32_t i = 0; i < utxo_list_len; i++) {
    if (i == utxo_index) {
      tmp_tx_in = *tx_in;
    }
    else {
      tmp_tx_in.prev_tx_hash  = utxo_list[i].tx_hash;
      tmp_tx_in.prev_tx_index = utxo_list[i].tx_index;
      tmp_tx_in.sequence      = 0xffffffff; // Until BIP 125 sequence fields were unused. TODO: Implement support for BIP 125
      tmp_tx_in.script.data   = NULL;
      tmp_tx_in.script.len    = 0;
    }
    add_input_to_tx(req, &tmp_tx, &tmp_tx_in);
  }

  // prepare array for hashing
  bytes_t hash_message = NULL_BYTES;
  build_tx_in_hash_msg(req, &hash_message, &tmp_tx, utxo_list, utxo_list_len, utxo_index, sighash, utxo_list[utxo_index].script_type);

  // Finally, sign transaction input
  // -- Obtain DER signature
  bytes_t sig   = NULL_BYTES;
  der_sig->data = _malloc(75);

  TRY(req_require_signature(req, SIGN_EC_BTC, PL_SIGN_BTCTX, &sig, hash_message, *account, req->requests[0]))

  der_sig->len                  = ecdsa_sig_to_der(sig.data, der_sig->data);
  der_sig->data[der_sig->len++] = sig.data[64]; // append verification byte to end of DER signature

  // signature is complete
  _free(hash_message.data);
  _free(tmp_tx.input.data);

  return IN3_OK;
}

static void add_sig_to_utxo(btc_utxo_t* utxo, const bytes_t* sig) {
  size_t current_size               = utxo->sig_count * sizeof(bytes_t);
  size_t new_size                   = current_size + sizeof(bytes_t);
  utxo->signatures                  = utxo->signatures ? _realloc(utxo->signatures, new_size, current_size) : _malloc(new_size);
  utxo->signatures[utxo->sig_count] = *sig;
  utxo->sig_count++;
}

in3_ret_t btc_sign_tx(in3_req_t* req, btc_tx_t* tx, btc_utxo_t* selected_utxo_list, uint32_t utxo_list_len) {
  // for each input in a tx:
  for (uint32_t i = 0; i < utxo_list_len; i++) {
    btc_tx_in_t tx_in   = {0};
    bytes_t     witness = NULL_BYTES;
    TRY(prepare_tx_in(req, &selected_utxo_list[i], &tx_in))
    bool is_segwit = selected_utxo_list[i].script_type == BTC_V0_P2WPKH || selected_utxo_list[i].script_type == BTC_P2WSH;

    if (selected_utxo_list[i].script_type == BTC_UNSUPPORTED || selected_utxo_list[i].script_type == BTC_NON_STANDARD) {
      return req_set_error(req, "ERROR: in btc_sign_tx: utxo script is non-standard or unsupported.", IN3_EINVAL);
    }
    // -- for each signature:
    for (uint32_t j = 0; j < selected_utxo_list[i].accounts_count; j++) {
      bytes_t sig = NULL_BYTES;
      // TODO: select random unused key to sign if multisig
      TRY_CATCH(btc_sign_tx_in(req, &sig, tx, selected_utxo_list, utxo_list_len, i, &selected_utxo_list[i].accounts[j].account, &selected_utxo_list[i].accounts[j].pub_key, &tx_in, BTC_SIGHASH_ALL),
                _free(tx_in.script.data);
                _free(tx_in.prev_tx_hash);)

      add_sig_to_utxo(&selected_utxo_list[i], &sig);
    }
    // We have the signatures, now write the unlocking script to input
    TRY_CATCH(build_unlocking_script(req, &tx_in, &witness, &selected_utxo_list[i]),
              _free(tx_in.script.data);
              _free(tx_in.prev_tx_hash);)

    // Add signed input and witness to transaction
    if (is_segwit) {
      TRY_CATCH(add_witness_to_tx(req, tx, &witness),
                _free(tx_in.script.data);
                _free(tx_in.prev_tx_hash);
                _free(witness.data);)
      _free(witness.data);
    }
    TRY_FINAL(add_input_to_tx(req, tx, &tx_in),
              _free(tx_in.script.data);
              _free(tx_in.prev_tx_hash);)
  }
  return IN3_OK;
}