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

// WARNING: You need to free tx_in->script.data after calling this function!
in3_ret_t btc_sign_tx_in(in3_req_t* req, btc_tx_t* tx, const btc_utxo_t* utxo_list, const uint32_t utxo_list_len, const uint32_t utxo_index, const bool is_segwit, const bytes_t* account, const bytes_t* pub_key, btc_tx_in_t* tx_in, uint8_t sighash) {
  if (!tx_in || !account) {
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

  bytes_t hash_message;

  if (is_segwit) {
    // segwit transaction
    // TODO: Abstract all code inside this block in a separate function
    bytes_t prev_outputs, sequence;
    uint8_t hash_prev_outputs[32], hash_sequence[32], hash_outputs[32];

    prev_outputs.len = utxo_list_len * 36; // 32 bytes tx_hash + 4 bytes tx_index
    sequence.len     = utxo_list_len * 4;

    prev_outputs.data = alloca(prev_outputs.len * sizeof(*prev_outputs.data));
    sequence.data     = alloca(sequence.len * sizeof(*sequence.data));

    uint32_t default_sequence = 0xffffffff;
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
    uint32_t index   = 0;
    hash_message.len = (get_compact_uint_size((uint64_t) utxo_index) +
                        utxo_list[utxo_index].tx_out.script.len +
                        +156);

    hash_message.data = alloca(hash_message.len * sizeof(uint8_t));
    uint8_t* d        = hash_message.data;
    uint_to_le(&hash_message, index, tx->version);
    index += 4;
    memcpy(d + index, hash_prev_outputs, 32);
    index += 32;
    memcpy(d + index, hash_sequence, 32);
    index += 32;
    rev_memcpy(d + index, utxo_list[utxo_index].tx_hash, 32);
    index += 32;
    uint_to_le(&hash_message, index, utxo_list[utxo_index].tx_index);
    index += 4;
    memcpy(d + index, utxo_list[utxo_index].tx_out.script.data, utxo_list[utxo_index].tx_out.script.len);
    index += utxo_list[utxo_index].tx_out.script.len;
    long_to_le(&hash_message, index, utxo_list[utxo_index].tx_out.value);
    index += 8;
    uint_to_le(&hash_message, index, 0xffffffff); // This is the 'sequence' field. Until BIP 125 sequence fields were unused. TODO: Implement support for BIP 125
    index += 4;
    memcpy(d + index, hash_outputs, 32);
    index += 32;
    memcpy(d + index, (uint8_t*) &tx->lock_time, 4);
    index += 4;
    uint_to_le(&hash_message, index, sighash);
  }
  else {
    // legacy transaction

    // TODO: Implement this in a more efficient way. Right now we copy
    // the whole tx just to add 4 bytes at the end of the stream

    btc_serialize_tx(&tmp_tx, &(tmp_tx.all));
    hash_message.len  = tmp_tx.all.len + 4;
    hash_message.data = alloca(hash_message.len * sizeof(uint8_t));

    // Copy serialized transaction
    memcpy(hash_message.data, tmp_tx.all.data, tmp_tx.all.len);

    // write sighash (4 bytes) at the end of the input
    uint_to_le(&hash_message, tmp_tx.all.len, sighash);
  }

  // Finally, sign transaction input
  // -- Obtain DER signature
  bytes_t sig = NULL_BYTES;
  // sig.data = NULL;
  // sig.len  = 65;

  bytes_t der_sig;

  der_sig.data = alloca(sizeof(uint8_t) * 75);

  TRY(req_require_signature(req, SIGN_EC_BTC, PL_SIGN_BTCTX, &sig, hash_message, *account, req->requests[0]))
  
  der_sig.len                 = ecdsa_sig_to_der(sig.data, der_sig.data);
  der_sig.data[der_sig.len++] = sig.data[64]; // append verification byte to end of DER signature

  // -- build scriptSig
  if (is_segwit) {
    // witness-enabled input
    if (tmp_tx_in.script.len == 21) {
      // Pay-To-Witness-Public-Key-Hash (P2WPKH).
      // scriptPubKey(received from utxo) = VERSION_BYTE | HASH160(PUB_KEY) --> total: 21 bytes
      // scriptSig(written to tx_in) should be empty. Data will be written in witness field
      // witness(we write this to transaction) = NUM_ELEMENTS | ZERO_BYTE | DER_SIG_LEN | DER_SIG | PUB_KEY_LEN | PUB_KEY

      // As we don't still support multisig, NUM_ELEMENTS is fixed in 2 (the signature and the public key)
      // TODO: IMplement multisig support

      tx_in->script.len  = 0;
      tx_in->script.data = NULL;

      bytes_t witness;
      witness.len    = 1 + 1 + der_sig.len + 1 + pub_key->len; // NUM_ELEMENTS + DER_SIG_LEN + DER_SIG + PUB_KEY_LEN + PUB_KEY
      witness.data   = alloca(sizeof(uint8_t) * witness.len);
      uint32_t index = 0;

      witness.data[index++] = 2;                            // write NUM_ELEMENTS. When multisig is implemented, this value should change according to the number of signatures
      long_to_compact_uint(&witness, index++, der_sig.len); // it is safe to assume this field only has 1 byte in a correct execution
      memcpy(witness.data + index, der_sig.data, der_sig.len);
      index += der_sig.len;
      witness.data[index++] = pub_key->len; // write PUB_KEY_LEN
      memcpy(witness.data + index, pub_key->data, pub_key->len);

      add_witness_to_tx(req, tx, &witness);
    }
    else if (tmp_tx_in.script.len == 33) {
      // Pay-To-Witness-Script-Hash (P2WSH)
      // TODO: Implement multisig support
      // TODO: Implement BIP16 support (Where P2SH was defined)
      // TODO: Implement support to Pay-To-Witness-Script-Hash (P2WSH)
      return req_set_error(req, "ERROR: in btc_sign_tx_in: P2WSH is not implemented yet", IN3_ERPC);
    }
    else {
      return req_set_error(req, "ERROR: in btc_sign_tx_in: signature algorithm could not be determined.", IN3_ERPC);
    }
  }
  else {
    // Pay-To-Public-Key-Hash (P2PKH). scriptSig = DER_LEN|DER_SIG|PUB_KEY_LEN|PUB_BEY
    // TODO: Abstract this block of code into a separate function
    tx_in->script.len = 1 + der_sig.len + 1 + 64; // DER_SIG_LEN + DER_SIG + PUBKEY_LEN + PUBKEY
    if (tx->flag) tx_in->script.len++;            // We need to include a zero byte it it is a witness transaction
    tx_in->script.data = malloc(sizeof(uint8_t) * tx_in->script.len);

    bytes_t* b     = &tx_in->script;
    uint32_t index = 0;

    if (tx->flag) b->data[index++] = 0;          // write zero byte if we are dealing with a witness transaction
    long_to_compact_uint(b, index, der_sig.len); // write der_sig len field
    index++;                                     // it is safe to assume the previous field only has 1 byte in a correct execution.
    // write DER signature
    uint32_t i = 0;
    while (i < der_sig.len) {
      b->data[index++] = der_sig.data[i++];
    }
    b->data[index++] = 64; // write pubkey len
    // write pubkey
    i = 0;
    while (i < 64) {
      b->data[index++] = pub_key->data[i++];
    }
  }

  // signature is complete
  _free(tmp_tx.all.data);
  _free(tmp_tx.input.data);

  return IN3_OK;
}

in3_ret_t btc_sign_tx(in3_req_t* req, btc_tx_t* tx, const btc_utxo_t* selected_utxo_list, uint32_t utxo_list_len, bytes_t* account, bytes_t* pub_key) {
  // for each input in a tx:
  for (uint32_t i = 0; i < utxo_list_len; i++) {
    // -- for each public key (assume we only have one pub key for now):
    // TODO: Allow setting a specific public key for each input
    btc_tx_in_t tx_in = {0};
    TRY(prepare_tx_in(req, &selected_utxo_list[i], &tx_in))
    bool is_segwit = (selected_utxo_list[i].tx_out.script.data[0] < OP_PUSHDATA1);
    TRY_CATCH(btc_sign_tx_in(req, tx, selected_utxo_list, utxo_list_len, i, is_segwit, account, pub_key, &tx_in, BTC_SIGHASH_ALL),
              _free(tx_in.script.data);
              _free(tx_in.prev_tx_hash);)
    TRY_FINAL(add_input_to_tx(req, tx, &tx_in), 
              _free(tx_in.script.data);
              _free(tx_in.prev_tx_hash);)
  }
  return IN3_OK;
}