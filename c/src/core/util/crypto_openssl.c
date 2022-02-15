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

#include "crypto.h"
#include <openssl/evp.h>

typedef struct {
  EVP_MD_CTX* ctx;
  EVP_MD*     digest;
} ssl_digest_t;

int openssl_hash(uint8_t* dst, const uint8_t* src, size_t len, char* type) {
  int          res    = -1;
  unsigned int dstlen = 0;
  EVP_MD_CTX*  ctx    = EVP_MD_CTX_new();
  if (!ctx)
    return -1;
  EVP_MD* sha256 = EVP_MD_fetch(NULL, type, NULL);
  if (!sha256)
    goto err;
  /* Initialise the digest operation */
  if (!EVP_DigestInit_ex(ctx, sha256, NULL))
    goto err;

  if (!EVP_DigestUpdate(ctx, src, len))
    goto err;

  if (!EVP_DigestFinal_ex(ctx, dst, &dstlen))
    goto err;
  res = 0;

err:
  /* Clean up all the resources we allocated */
  EVP_MD_free(sha256);
  EVP_MD_CTX_free(ctx);
  return res;
}

/** writes the keccak hash of the data as 32 bytes to the dst pointer. */
in3_ret_t keccak(bytes_t data, void* dst) {
  return openssl_hash(dst, data.data, data.len, "keccak-256") ? IN3_ENOTSUP : IN3_OK;
}

/** create a digest based on the type passed */
in3_digest_t crypto_create_hash(
    in3_digest_type_t type /**< the type as defined in in3_digest_type_t*/
) {
  in3_digest_t res       = {.ctx = NULL, .type = type};
  EVP_MD*      digestssl = NULL;
  switch (type) {
    case DIGEST_KECCAK:
      digestssl = EVP_MD_fetch(NULL, "keccak-256", NULL);
      break;
    case DIGEST_SHA256:
    case DIGEST_SHA256_BTC:
      digestssl = EVP_MD_fetch(NULL, "SHA256", NULL);
      break;
    case DIGEST_RIPEMD_160:
      digestssl = EVP_MD_fetch(NULL, "RIPEMD160", NULL);
      break;

    default:
      break;
  }
  if (digestssl) {
    res.ctx         = _malloc(sizeof(ssl_digest_t));
    ssl_digest_t* d = (ssl_digest_t*) res.ctx;
    d->digest       = digestssl;
    d->ctx          = EVP_MD_CTX_new();
    EVP_DigestInit_ex(d->ctx, d->digest, NULL);
  }

  return res;
}

/** updates the hash with the passed bytes */
void crypto_update_hash(
    in3_digest_t digest, /**< the digest created with crypto_create_hash */
    bytes_t      data    /**< the data to hash */
) {
  ssl_digest_t* d = (ssl_digest_t*) digest.ctx;
  if (!d || !data.len) return;
  EVP_DigestUpdate(d->ctx, data.data, data.len);
}

/** finishes the hash and writes the result to the given pointer. This function will also clean up all resources used during hashing */
void crypto_finalize_hash(
    in3_digest_t digest, /**< the digest created with crypto_create_hash */
    void*        dst     /**< pointer to the data to write the final hash. the size depends on the diget type */
) {
  ssl_digest_t* d = (ssl_digest_t*) digest.ctx;
  if (!d) return;
  unsigned int s;
  EVP_DigestFinal_ex(d->ctx, dst, &s);
  EVP_MD_free(d->digest);
  EVP_MD_CTX_free(d->ctx);
  _free(d);
}

in3_ret_t crypto_sign_digest(in3_curve_type_t type, const uint8_t* digest, const uint8_t* pk, uint8_t* dst) {
  UNUSED_VAR(type);
  UNUSED_VAR(digest);
  UNUSED_VAR(pk);
  UNUSED_VAR(dst);
  return IN3_ENOTSUP;
}
in3_ret_t crypto_recover(in3_curve_type_t type, const uint8_t* digest, bytes_t signature, uint8_t* dst) {
  UNUSED_VAR(type);
  UNUSED_VAR(digest);
  UNUSED_VAR(signature);
  UNUSED_VAR(dst);
  return IN3_ENOTSUP;
}
in3_ret_t crypto_convert(in3_curve_type_t type, in3_convert_type_t conv_type, bytes_t src, uint8_t* dst, int* dst_len) {
  UNUSED_VAR(type);
  UNUSED_VAR(conv_type);
  UNUSED_VAR(src);
  UNUSED_VAR(dst);
  UNUSED_VAR(dst_len);
  return IN3_ENOTSUP;
}

void memzero(void* const pnt, const size_t len) {
  memset(pnt, 0, len);
}

in3_ret_t bip32(bytes_t seed, in3_curve_type_t curve, const char* path, uint8_t* dst) {
  UNUSED_VAR(seed);
  UNUSED_VAR(curve);
  UNUSED_VAR(path);
  UNUSED_VAR(dst);
  return IN3_ENOTSUP;
}

in3_ret_t mnemonic_verify(const char* mnemonic) {
  UNUSED_VAR(mnemonic);
  return IN3_ENOTSUP;
}
void mnemonic_to_seed(const char* mnemonic, const char* passphrase,
                      uint8_t seed[512 / 8],
                      void (*progress_callback)(uint32_t current,
                                                uint32_t total)) {

  UNUSED_VAR(mnemonic);
  UNUSED_VAR(passphrase);
  UNUSED_VAR(seed);
  UNUSED_VAR(progress_callback);
  memset(seed, 0, 64);
}
char* mnemonic_create(bytes_t seed) {
  UNUSED_VAR(seed);
  return NULL;
}

void pbkdf2_hmac_sha256(const uint8_t* pass, int passlen, const uint8_t* salt,
                        int saltlen, uint32_t iterations, uint8_t* key,
                        int keylen) {
  UNUSED_VAR(pass);
  UNUSED_VAR(passlen);
  UNUSED_VAR(salt);
  UNUSED_VAR(saltlen);
  UNUSED_VAR(iterations);
  UNUSED_VAR(key);
  UNUSED_VAR(keylen);
}

in3_ret_t aes_128_ctr_decrypt(uint8_t* aeskey, bytes_t cipher, uint8_t* iv_data, bytes32_t dst) {
  UNUSED_VAR(aeskey);
  UNUSED_VAR(cipher);
  UNUSED_VAR(iv_data);
  UNUSED_VAR(dst);
  return IN3_ENOTSUP;
}
