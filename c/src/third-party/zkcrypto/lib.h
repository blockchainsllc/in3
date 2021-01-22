
#include "../../core/util/bytes.h"
#include "../../core/util/error.h"
#ifndef __ZK_CRYPTO_H__
#define __ZK_CRYPTO_H__

typedef uint32_t zkcrypto_signer_t;


void zkcrypto_initialize();

in3_ret_t zkcrypto_pk_from_seed(bytes_t seed, bytes32_t dst);
in3_ret_t zkcrypto_pk_to_pubkey(bytes32_t pk, uint8_t* dst);
in3_ret_t zkcrypto_pk_to_pubkey_hash(bytes32_t pk, uint8_t* dst);
in3_ret_t zkcrypto_sign_musig(bytes32_t pk, bytes_t msg, uint8_t* dst);

in3_ret_t zkcrypto_compute_aggregated_pubkey(bytes_t keys, uint8_t* dst);
bool zkcrypto_verify_signatures(bytes_t message, bytes_t pubkeys, bytes_t signature);

/**
 * creates a new Schnott Musig Signer
 */
zkcrypto_signer_t zkcrypto_signer_new(bytes_t pub_keys, uint32_t pos);
void zkcrypto_signer_free(zkcrypto_signer_t signer);
in3_ret_t zkcrypto_signer_compute_precommitment(zkcrypto_signer_t signer, bytes_t seed, uint8_t* dst);
in3_ret_t zkcrypto_signer_receive_precommitment(zkcrypto_signer_t signer, bytes_t input, uint8_t* dst);
in3_ret_t zkcrypto_signer_receive_commitment(zkcrypto_signer_t signer, bytes_t input, uint8_t* dst);
in3_ret_t zkcrypto_signer_receive_signature_shares(zkcrypto_signer_t signer,  bytes_t input, uint8_t* dst);
in3_ret_t zkcrypto_signer_sign(zkcrypto_signer_t signer, bytes32_t pk, bytes_t input, uint8_t* dst);

#endif