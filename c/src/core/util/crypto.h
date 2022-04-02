/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
 *
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 *
 *
 * COMMERCIAL LICENSE USAGE
 *
 * Licensees holding a valid commercial license may use this file in accordance
 * with the commercial license agreement provided with the Software or, alternatively,
 * in accordance with the terms contained in a written agreement between you and
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further
 * information please contact slock.it at in3@slock.it.
 *
 * Alternatively, this file may be used under the AGPL license as follows:
 *
 * AGPL LICENSE USAGE
 *
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available
 * complete source code of licensed works and modifications, which include larger
 * works using a licensed work, under the same license. Copyright and license notices
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

// @PUBLIC_HEADER
/** @file
 * util function for crypto.
 * */

#ifndef ___CRYPTO_H
#define ___CRYPTO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bytes.h"
#include "error.h"
#include <stdint.h>

/** type pf hashing strategy */
typedef enum {
  DIGEST_KECCAK     = 1, /**< sha3 keccak, which is the default for all ethereum hashes, resulting in 32bytes */
  DIGEST_SHA256     = 2, /**< sha256, resulting in 32bytes */
  DIGEST_SHA256_BTC = 3, /**< sha256 hashed twice, as the bitcoin protocol requires it, resulting in 32bytes */
  DIGEST_RIPEMD_160 = 4  /**< ripemd160 which results in 20 bytes */
} in3_digest_type_t;

/** encoding types for bytes to string encoding and decoding */
typedef enum {
  ENC_HEX     = 1, /**< hexadecimal encoding (without any prefix ) */
  ENC_BASE58  = 2, /**< base58 encoding as used to represent addresses */
  ENC_BASE64  = 3, /**< base64 encoding */
  ENC_DECIMAL = 4  /**< converts the bytes as a decimal number as string. ( max 32 bytes )*/
} in3_encoding_type_t;

/** type of the eliptic or edward curve */
typedef enum {
  ECDSA_SECP256K1 = 1, /**< secp256k1 , which is used for bitcoin and ethereum */
  EDDSA_ED25519   = 2  /**< ED25519 */
} in3_curve_type_t;

/** type of converter for any signature or private key */
typedef enum {
  CONV_PK32_TO_PUB64 = 1, /**< extract the publickey from a private key as raw point (64bytes) without any prefix */
  CONV_PK32_TO_PUB32 = 2, /**< extract the publickey from a private key as raw point (32bytes) without any prefix ( for EDDSA )*/
  CONV_SIG65_TO_DER  = 3, /**< converts a 65 byte signtature to a DER format */
  CONV_PUB64_TO_DER  = 4  /**< converts a 64 byte public key to a DER format */
} in3_convert_type_t;

/** represents a digest to use for hashing */
typedef struct {
  void*             ctx;  /**< points the internal state, this will be cleaned up during the crypto_finalize_hash call. */
  in3_digest_type_t type; /**< the type of the digest */
} in3_digest_t;

/** writes the keccak hash of the data as 32 bytes to the dst pointer. */
in3_ret_t keccak(bytes_t data, void* dst);

/** create a digest based on the type passed */
in3_digest_t crypto_create_hash(
    in3_digest_type_t type /**< the type as defined in in3_digest_type_t*/
);

/** updates the hash with the passed bytes */
void crypto_update_hash(
    in3_digest_t digest, /**< the digest created with crypto_create_hash */
    bytes_t      data    /**< the data to hash */
);

/** finishes the hash and writes the result to the given pointer. This function will also clean up all resources used during hashing */
void crypto_finalize_hash(
    in3_digest_t digest, /**< the digest created with crypto_create_hash */
    void*        dst     /**< pointer to the data to write the final hash. the size depends on the diget type */
);

/**
 * encodes bytes to a string */
int encode(
    in3_encoding_type_t type, /**< the encoding typr */
    bytes_t             src,  /**< the src-data to encode */
    char*               dst   /**< the pointer to resulting string to write to. the caller has to ensure it is big enough. (see encode_size to allocate enough memory)*/
);

int encode_size(
    in3_encoding_type_t type,
    int                 src_len);

int decode(
    in3_encoding_type_t type,
    const char*         src,
    int                 src_len,
    uint8_t*            dst);

int decode_size(
    in3_encoding_type_t type,
    int                 src_len);

in3_ret_t crypto_sign_digest(in3_curve_type_t type, const bytes_t digest, const uint8_t* pk, const uint8_t* pubkey, uint8_t* signature);
in3_ret_t crypto_recover(in3_curve_type_t type, const bytes_t digest, bytes_t signature, uint8_t* pubkey);
in3_ret_t crypto_convert(in3_curve_type_t type, in3_convert_type_t conv_type, bytes_t src, uint8_t* dst, int* dst_len);

void random_buffer(uint8_t* dst, size_t len);
void memzero(void* const pnt, const size_t len);

in3_ret_t bip32(bytes_t seed, in3_curve_type_t curve, const char* path, uint8_t* dst);

in3_ret_t mnemonic_verify(const char* mnemonic);
void      mnemonic_to_seed(const char* mnemonic, const char* passphrase,
                           uint8_t seed[512 / 8],
                           void (*progress_callback)(uint32_t current,
                                                uint32_t total));
char*     mnemonic_create(bytes_t seed);

void pbkdf2_hmac_sha256(const uint8_t* pass, int passlen, const uint8_t* salt,
                        int saltlen, uint32_t iterations, uint8_t* key,
                        int keylen);

in3_ret_t aes_128_ctr_decrypt(uint8_t* aeskey, bytes_t cipher, uint8_t* iv_data, bytes32_t dst);

#ifdef __cplusplus
}
#endif
#endif
