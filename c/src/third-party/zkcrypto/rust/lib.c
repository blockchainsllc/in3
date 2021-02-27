#include <stdio.h>
#include <stdlib.h>

#include "../lib.h"
#include "zkcrypto.h"


void zkcrypto_initialize() {
  zc_init();
}


in3_ret_t zkcrypto_pk_from_seed(bytes_t seed, bytes32_t dst) {
  zc_private_key_from_seed(seed.data,seed.len,dst);
  return IN3_OK;
}
in3_ret_t zkcrypto_pk_to_pubkey(bytes32_t pk, uint8_t* dst) {
  zc_private_key_to_pubkey(pk,dst);
  return IN3_OK;
}

in3_ret_t zkcrypto_pk_to_pubkey_hash(bytes32_t pk, uint8_t* dst) {
  zc_private_key_to_pubkey_hash(pk,dst);
  return IN3_OK;
}


in3_ret_t zkcrypto_sign_musig(bytes32_t pk, bytes_t msg, uint8_t* dst) {
  zc_sign_musig(pk,msg.data,msg.len,dst);
  return IN3_OK;
}

in3_ret_t zkcrypto_compute_aggregated_pubkey(bytes_t keys, uint8_t* dst) {
  zc_compute_aggregated_pubkey(keys.data,keys.len,dst);
  return IN3_OK;
}

zkcrypto_signer_t zkcrypto_signer_new(bytes_t pub_keys, uint32_t pos) {
  return zc_signer_new(pub_keys.data, pub_keys.len,pos);
}

void zkcrypto_signer_free(zkcrypto_signer_t signer) {
  zc_signer_free(signer);
}

in3_ret_t zkcrypto_signer_compute_precommitment(zkcrypto_signer_t signer, bytes_t seed, uint8_t* dst) {
  zc_signer_compute_precommitment(signer, (uint32_t*) (void*) seed.data, seed.len/4,dst);
  return IN3_OK;
}

in3_ret_t zkcrypto_signer_receive_precommitment(zkcrypto_signer_t signer, bytes_t input, uint8_t* dst) {
  zc_signer_receive_precommitments(signer, input.data, input.len,dst);
  return IN3_OK;
}


in3_ret_t zkcrypto_signer_receive_commitment(zkcrypto_signer_t signer, bytes_t input, uint8_t* dst) {
  zc_signer_receive_commitments(signer, input.data, input.len,dst);
  return IN3_OK;
}



in3_ret_t zkcrypto_signer_sign(zkcrypto_signer_t signer, bytes32_t pk, bytes_t input, uint8_t* dst) {
  zc_signer_sign(signer, pk, input.data, input.len,dst);
  return IN3_OK;
}

in3_ret_t zkcrypto_signer_receive_signature_shares(zkcrypto_signer_t signer,  bytes_t input, uint8_t* dst) {
  zc_signer_receive_signature_shares(signer, input.data, input.len,dst);
  return IN3_OK;
}

bool zkcrypto_verify_signatures(bytes_t message, bytes_t pubkeys, bytes_t signature) {
  return zc_verify_signatures(message.data,message.len, pubkeys.data,pubkeys.len, signature.data + (signature.len==96?32:0));
}

bool zkcrypto_verify_musig(bytes_t message,  bytes_t signature) {
  return zc_verify_musig(message.data,message.len, signature.data);
}



in3_ret_t zkcrypto_pubkey_hash(bytes_t pubkey, uint8_t* dst) {
  zc_pubkey_to_pubkey_hash(pubkey.data,dst);
  return IN3_OK;
}


