#include "btc_sign.h"
#include "../../third-party/crypto/secp256k1.h"
#include "btc_serialize.h"
#include "btc_types.h"

// Fill tx_in fields, preparing the input for signing
// WARNING: You need to free tx_in->prev_tx_hash after calling this function
static void init_tx_in(const btc_utxo_t* utxo, btc_tx_in_t* tx_in) {
  if (!utxo || !tx_in) {
    // TODO: Implement better error treatment
    printf("ERROR: in init_tx_in: function arguments can not be null!\n");
    return;
  }
  tx_in->prev_tx_index = utxo->tx_index;
  tx_in->prev_tx_hash  = malloc(32 * sizeof(uint8_t));
  memcpy(tx_in->prev_tx_hash, utxo->tx_hash, 32);
  tx_in->script.len  = 0;
  tx_in->script.data = NULL;
  tx_in->sequence    = 0xffffffff;
}

// WARNING: You need to free tx_in->script.data after calling this function!
void btc_sign_tx_in(const btc_tx_t* tx, const btc_tx_out_t* utxo, const bytes_t* priv_key, btc_tx_in_t* tx_in, uint8_t sighash) {
  if (!tx_in || !priv_key) {
    // TODO: Implement better error handling
    printf("ERROR: in btc_sign_tx_in: function arguments cannot be NULL.");
    return;
  }
  // TODO: Implement support for other sighashes
  if (sighash != BTC_SIGHASH_ALL) {
    printf("ERROR: in btc_sign_tx_in: Sighash not supported.");
    return;
  }

  btc_tx_t tmp_tx;
  tmp_tx.version       = tx->version;
  tmp_tx.flag          = 0; // TODO: Implement segwit support
  tmp_tx.input_count   = 1; // TODO: support more than one input
  tmp_tx.output_count  = tx->output_count;
  tmp_tx.output.len    = tx->output.len;
  tmp_tx.output.data   = alloca(sizeof(uint8_t) * tmp_tx.output.len);
  tmp_tx.witnesses.len = 0;
  for (uint32_t i = 0; i < tmp_tx.output.len; i++) tmp_tx.output.data[i] = tx->output.data[i];
  tmp_tx.lock_time = tx->lock_time;

  // TODO: This should probably be set before calling the signer function. If this is done outside this function, then the utxo is probably not needed.
  tx_in->script = utxo->script; // This should temporarily be the scriptPubKey of the output we want to redeem
  btc_serialize_tx_in(tx_in, &tmp_tx.input);

  // prepare array for hashing
  btc_serialize_tx(&tmp_tx, &(tmp_tx.all));
  bytes_t hash_input;
  hash_input.len  = tmp_tx.all.len + 4;
  hash_input.data = malloc(hash_input.len * sizeof(uint8_t));

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
  uint8_t sig[65];
  bytes_t der_sig;

  der_sig.data = alloca(sizeof(uint8_t) * 75);
  ecdsa_sign(&secp256k1, HASHER_SHA2D, priv_key->data, hash_input.data, hash_input.len, sig, sig + 64, NULL);
  der_sig.len                 = ecdsa_sig_to_der(sig, der_sig.data);
  der_sig.data[der_sig.len++] = sig[64]; // append verification byte to end of DER signature

  // -- Extract public key out of the provided private key. This will be used to build the tx_in scriptSig
  uint8_t pub_key[65];
  ecdsa_get_public_key65(&secp256k1, (const uint8_t*) priv_key->data, pub_key);

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
    b->data[index++] = pub_key[i++];
  }

  // signature is complete
  _free(tmp_tx.all.data);
  _free(tmp_tx.input.data);
  _free(hash_input.data);
}

void btc_sign_tx(btc_tx_t* tx, const btc_utxo_t* selected_utxo_list, uint32_t utxo_list_len, bytes_t* priv_key) {
  // for each input in a tx:
  for (uint32_t i = 0; i < utxo_list_len; i++) {
    // -- for each pub_key (assume we only have one pub key for now):
    // TODO: Allow setting a specific pub_key for each input
    btc_tx_in_t tx_in;
    init_tx_in(&selected_utxo_list[i], &tx_in);
    btc_sign_tx_in(tx, &selected_utxo_list[i].tx_out, priv_key, &tx_in, BTC_SIGHASH_ALL);
    add_input_to_tx(tx, &tx_in);
    _free(tx_in.script.data);
    _free(tx_in.prev_tx_hash);
  }
  return;
}