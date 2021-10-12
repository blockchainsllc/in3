#include "btc_sign.h"
#include "../../third-party/crypto/secp256k1.h"
#include "btc_serialize.h"
#include "btc_types.h"

static void btc_sha256(uint8_t* data, uint32_t len, bytes32_t output) {
  SHA256_CTX c;
  sha256_Init(&c);
  sha256_Update(&c, data, len);
  sha256_Final(&c, output);
}

void btc_dsha256(uint8_t* data, uint32_t len, bytes32_t output) {
  bytes32_t tmp_hash;
  btc_sha256(data, len, tmp_hash);
  btc_sha256(tmp_hash, 32, output);
}

// WARNING: You need to free tx_in pointer after calling this function!
void btc_sign_tx_in(const btc_tx_t* tx, const btc_tx_out_t* utxo, const bytes_t* priv_key, btc_tx_in_t* tx_in, uint8_t sighash) {
  if (!tx_in || !priv_key) {
    printf("ERROR: in btc_sign_tx_in: function arguments can not be NULL.");
    return;
  }
  // TODO: Implement support for other sighashes
  if (sighash != BTC_SIGHASH_ALL) {
    printf("ERROR: in btc_sign_tx_in: Sighash not supported.");
    return;
  }

  btc_tx_t tmp_tx;
  tmp_tx.version      = tx->version;
  tmp_tx.flag         = 0; // TODO: Implement segwit support
  tmp_tx.input_count  = 1;
  tmp_tx.output_count = tx->output_count;
  tmp_tx.output       = tx->output;
  tmp_tx.lock_time    = tx->lock_time;

  tx_in->script = utxo->script; // This should temporarily be the scriptPubKey of the output we want to redeem
  btc_serialize_tx_in(tx_in, &tmp_tx.input);

  // prepare array for hashing
  btc_serialize_tx(&tmp_tx, &(tmp_tx.all));
  bytes_t hash_input;
  hash_input.len  = tmp_tx.all.len + 4;
  hash_input.data = malloc(hash_input.len * sizeof(uint8_t));

  // TODO: Implement this in a more fficient way. There is no need to copy
  // the whole tx just to add 4 bytes at the end of the stream

  // Copy serialized transaction
  for (uint32_t i = 0; i < tmp_tx.all.len; i++) {
    hash_input.data[i] = tmp_tx.all.data[i];
  }
  // write sighash (4 bytes) at the end of the input
  uint_to_le(&hash_input, tmp_tx.all.len, sighash);

  // Finally, sign transaction input
  // -- Obtain DER signature
  uint8_t sig[64];
  bytes_t der_sig;
  der_sig.data = alloca(sizeof(uint8_t) * 75);
  ecdsa_sign(&secp256k1, HASHER_SHA2D, (const uint8_t*) priv_key->data, hash_input.data, hash_input.len, sig, NULL, NULL); // TODO: check if it is really DER encoded or if aditional logic is necessary
  der_sig.len = ecdsa_sig_to_der(sig, der_sig.data);

  // -- Extract public key out of the provided private key. This will be used to build the tx_in scriptSig
  uint8_t pub_key[65];
  ecdsa_get_public_key65(&secp256k1, (const uint8_t*) priv_key->data, pub_key);

  // -- build scriptSig: SCRIPT_SIG_LEN|DER_LEN|DER_SIG|PUB_KEY_LEN|PUB_BEY
  tx_in->script.len = 1 + der_sig.len + 1 + 1 + 65;              // 1 byte DER_SIG_LEN + DER_SIG + 1 byte SIGHASH + 1 byte PUBKEY_LEN + PUBKEY
  tx_in->script.len += get_compact_uint_size(tx_in->script.len); // Also account for the compact uint at the beginning of the byte stream
  tx_in->script.data = malloc(sizeof(uint8_t) * tx_in->script.len);

  bytes_t* b     = &tx_in->script;
  uint32_t index = 0;
  long_to_compact_uint(b, index, tx_in->script.len); // write scriptSig len field
  index += get_compact_uint_size(tx_in->script.len);
  long_to_compact_uint(b, index, der_sig.len); // write der_sig len field
  index += 1;                                  // it is safe to assume the previous field only has 1 byte in a correct execution. TODO: Return an error in case der_sig_len is not 1 byte long
  // write der signature
  uint32_t i = 0;
  while (i < der_sig.len) {
    b->data[index++] = der_sig.data[i++];
  }
  b->data[index++] = sighash; // write sighash
  b->data[index++] = 65;      // write pubkey len
  // write pubkey
  i = 0;
  while (i < 65) {
    b->data[index++] = pub_key[i++];
  }
  // signature is complete

  _free(tmp_tx.all.data);
  _free(tmp_tx.input.data);
  _free(tmp_tx.output.data);
  _free(hash_input.data);
}

void btc_sign_tx(bytes_t* raw_tx, const btc_tx_out_t* utxo_list, bytes_t* priv_key, bytes_t* dst_sig) {
  UNUSED_VAR(raw_tx);
  UNUSED_VAR(priv_key);
  UNUSED_VAR(dst_sig);
  UNUSED_VAR(utxo_list);
  // for each input in a tx:
  // -- for each pub_key (assume we only have one pub key for now):
  // -- -- sig = btc_sign_tx_in()
  // -- -- add_sig_to_tx_in()
  return;
}