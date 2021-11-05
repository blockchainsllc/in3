#include "btc_sign.h"
#include "../../core/client/plugin.h"
#include "../../core/client/request.h"
#include "../../core/client/request_internal.h"
#include "../../third-party/crypto/secp256k1.h"
#include "btc_serialize.h"
#include "btc_types.h"

// Fill tx_in fields, preparing the input for signing
// WARNING: You need to free tx_in->prev_tx_hash after calling this function
static void prepare_tx_in(const btc_utxo_t* utxo, btc_tx_in_t* tx_in) {
  if (!utxo || !tx_in) {
    // TODO: Implement better error treatment
    printf("ERROR: in prepare_tx_in: function arguments can not be null!\n");
    return;
  }

  tx_in->prev_tx_index = utxo->tx_index;
  tx_in->prev_tx_hash  = malloc(32 * sizeof(uint8_t));
  memcpy(tx_in->prev_tx_hash, utxo->tx_hash, 32);

  // Before signing, input script field should temporarilly be equal to the utxo we want to redeem
  tx_in->script.len  = utxo->tx_out.script.len;
  tx_in->script.data = malloc(tx_in->script.len * sizeof(uint8_t));
  memcpy(tx_in->script.data, utxo->tx_out.script.data, tx_in->script.len);

  tx_in->sequence = 0xffffffff; // Until BIP 125 sequence fields were unused. TODO: Implement support for BIP 125
}

// WARNING: You need to free tx_in->script.data after calling this function!
in3_ret_t btc_sign_tx_in(in3_req_t* req, const btc_tx_t* tx, const btc_utxo_t* utxo_list, const uint32_t utxo_index, const bytes_t* pub_key, btc_tx_in_t* tx_in, uint8_t sighash) {
  if (!tx_in || !pub_key) {
    return req_set_error(req, "ERROR: in btc_sign_tx_in: function arguments cannot be NULL.", IN3_ERPC);
  }
  // TODO: Implement support for other sighashes
  if (sighash != BTC_SIGHASH_ALL) {
    return req_set_error(req, "ERROR: in btc_sign_tx_in: Sighash not supported.", IN3_ERPC);
  }

  // Generate an unsigned transaction. This will be used to henerate the hash provided to
  // the ecdsa signing algorithm
  btc_tx_t tmp_tx;
  btc_init_tx(&tmp_tx);
  tmp_tx.version      = tx->version;
  tmp_tx.output_count = tx->output_count;
  tmp_tx.output.len   = tx->output.len;
  tmp_tx.output.data  = alloca(sizeof(uint8_t) * tmp_tx.output.len);
  for (uint32_t i = 0; i < tmp_tx.output.len; i++) tmp_tx.output.data[i] = tx->output.data[i];
  tmp_tx.lock_time = tx->lock_time;

  // TODO: This should probably be set before calling the signer function. If this is done outside this function, then the utxo is probably not needed.
  // Include inputs into unsigned tx
  btc_tx_in_t tmp_tx_in;
  for (uint32_t i = 0; i < tx->input_count; i++) {
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
  btc_serialize_tx(&tmp_tx, &(tmp_tx.all));
  bytes_t hash_input;
  hash_input.len  = tmp_tx.all.len + 4;
  hash_input.data = alloca(hash_input.len * sizeof(uint8_t));

  // TODO: Implement this in a more efficient way. Right now we copy
  // the whole tx just to add 4 bytes at the end of the stream

  // Copy serialized transaction
  for (uint32_t i = 0; i < tmp_tx.all.len; i++) {
    hash_input.data[i] = tmp_tx.all.data[i];
  }
  // write sighash (4 bytes) at the end of the input
  uint_to_le(&hash_input, tmp_tx.all.len, sighash);

  // Finally, sign transaction input
  // -- Obtain DER signature
  bytes_t sig;
  sig.data = NULL;
  sig.len  = 65;

  bytes_t der_sig;

  der_sig.data = alloca(sizeof(uint8_t) * 75);

  TRY(req_require_signature(req, SIGN_EC_BTC, PL_SIGN_BTCTX, &sig, hash_input, *pub_key, req->requests[0]))

  der_sig.len                 = ecdsa_sig_to_der(sig.data, der_sig.data);
  der_sig.data[der_sig.len++] = sig.data[64]; // append verification byte to end of DER signature

  // -- build scriptSig: DER_LEN|DER_SIG|PUB_KEY_LEN|PUB_BEY
  uint32_t scriptsig_len = der_sig.len + 1 + 65; // DER_SIG + 1 byte PUBKEY_LEN + PUBKEY
  tx_in->script.len      = 1 + scriptsig_len;    // Also account for 1 byte DER_SIG_LEN
  tx_in->script.data     = malloc(sizeof(uint8_t) * tx_in->script.len);

  bytes_t* b     = &tx_in->script;
  uint32_t index = 0;

  long_to_compact_uint(b, index, der_sig.len); // write der_sig len field
  index += 1;                                  // it is safe to assume the previous field only has 1 byte in a correct execution.
  // write der signature
  uint32_t i = 0;
  while (i < der_sig.len) {
    b->data[index++] = der_sig.data[i++];
  }
  b->data[index++] = 65; // write pubkey len
  // write pubkey
  i = 0;
  while (i < 65) {
    b->data[index++] = pub_key->data[i++];
  }

  // signature is complete
  _free(tmp_tx.all.data);
  _free(tmp_tx.input.data);
  // _free(hash_input.data);

  return IN3_OK;
}

in3_ret_t btc_sign_tx(in3_req_t* req, btc_tx_t* tx, const btc_utxo_t* selected_utxo_list, uint32_t utxo_list_len, bytes_t* pub_key) {
  // for each input in a tx:
  for (uint32_t i = 0; i < utxo_list_len; i++) {
    // -- for each pub_key (assume we only have one pub key for now):
    // TODO: Allow setting a specific pub_key for each input
    btc_tx_in_t tx_in = {0};
    prepare_tx_in(&selected_utxo_list[i], &tx_in);
    TRY_CATCH(btc_sign_tx_in(req, tx, selected_utxo_list, i, pub_key, &tx_in, BTC_SIGHASH_ALL),
              _free(tx_in.script.data);
              _free(tx_in.prev_tx_hash);)
    add_input_to_tx(req, tx, &tx_in);
    _free(tx_in.script.data);
    _free(tx_in.prev_tx_hash);
  }
  return IN3_OK;
}