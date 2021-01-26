
#include "../../core/util/bytes.h"
#include "../../core/util/error.h"
#ifndef __ZK_CRYPTO_H__
#define __ZK_CRYPTO_H__

typedef void*  zkcrypto_signer_t;

/**
 * this must be called in order to initialize the crypto-curves
 */
void zkcrypto_initialize();

/**
 * generates a private key based on see ( which must be at least 32 bytes)
 */
in3_ret_t zkcrypto_pk_from_seed(bytes_t seed, bytes32_t dst);

/**
 * generates the public key (32bytes) from a private key (32 bytes)
 */
in3_ret_t zkcrypto_pk_to_pubkey(bytes32_t pk, uint8_t* dst);

/**
 * generates the public key hash(20bytes) from a private key (32 bytes)
 */
in3_ret_t zkcrypto_pk_to_pubkey_hash(bytes32_t pk, uint8_t* dst);

/**
 * creates a public key hash (20 bytes) from a public key (32 bytes)
 */
in3_ret_t zkcrypto_pubkey_hash(bytes_t pubkey, uint8_t* dst);

/**
 * signs a message with a private key.
 * The signature will will be a 96 byte schnorr musig signature.
 */
in3_ret_t zkcrypto_sign_musig(bytes32_t pk, bytes_t msg, uint8_t* dst);

/**
 * aggregates multiple public keys (each 32 byte)  to one single public key.
 */
in3_ret_t zkcrypto_compute_aggregated_pubkey(bytes_t keys, uint8_t* dst);

/** 
 * verifies a musig schnorr signatures.
 */
bool zkcrypto_verify_signatures(bytes_t message, bytes_t pubkeys, bytes_t signature);

/**
 * creates a new Schnott Musig Signer, which of course needs to be freed after use.
 */
zkcrypto_signer_t zkcrypto_signer_new(bytes_t pub_keys, uint32_t pos);

/**
 * frees the signer.
 */
void zkcrypto_signer_free(zkcrypto_signer_t signer);

/**
 * computes a precommit based on the seed
 */
in3_ret_t zkcrypto_signer_compute_precommitment(zkcrypto_signer_t signer, bytes_t seed, uint8_t* dst);

/**
 * sets the precommit in the signer and calculate the commit.
 */
in3_ret_t zkcrypto_signer_receive_precommitment(zkcrypto_signer_t signer, bytes_t input, uint8_t* dst);

/**
 * sets the commit.
 */
in3_ret_t zkcrypto_signer_receive_commitment(zkcrypto_signer_t signer, bytes_t input, uint8_t* dst);

/**
 * creates the share.
 */
in3_ret_t zkcrypto_signer_receive_signature_shares(zkcrypto_signer_t signer,  bytes_t input, uint8_t* dst);

/**
 * creates the final signature based on shares (as input)
 */
in3_ret_t zkcrypto_signer_sign(zkcrypto_signer_t signer, bytes32_t pk, bytes_t input, uint8_t* dst);

/**
 * verify a signature ( works for simple pk key or schnorr signatures)
 */
bool zkcrypto_verify_musig(bytes_t message,  bytes_t signature) ;


#endif