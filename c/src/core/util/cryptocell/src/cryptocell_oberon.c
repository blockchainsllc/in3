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
#ifdef CRYPTOCELL_OBERON
#include "crypto.h"
#include <ocrypto_curve_p256.h>
#include <ocrypto_ecdsa_p256.h>
#include <ocrypto_sha256.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/** create a digest based on the type passed */
in3_digest_t crypto_create_hash(
    in3_digest_type_t type /**< the type as defined in in3_digest_type_t*/
) {
  in3_digest_t digest = {.ctx = NULL, .type = type};
  switch (type) {
    case DIGEST_SHA256:
    case DIGEST_SHA256_BTC: {
      digest.ctx = _calloc(1, sizeof(ocrypto_sha256_ctx));
      ocrypto_sha256_init((ocrypto_sha256_ctx*) digest.ctx); /**< generator state is initialized */
      return digest;
    }
    default: return digest;
  }
}

/** updates the hash with the passed bytes */
void crypto_update_hash(
    in3_digest_t digest, /**< the digest created with crypto_create_hash */
    bytes_t      data    /**< the data to hash */
) {
  switch (digest.type) {
    case DIGEST_SHA256:
    case DIGEST_SHA256_BTC: {
      if (data.len) ocrypto_sha256_update(digest.ctx, data.data, data.len); /**< generator state is updated to hash a message chunk */
      return;
    }
    default: return;
  }
}

/** finishes the hash and writes the result to the given pointer. This function will also clean up all resources used during hashing */
void crypto_finalize_hash(
    in3_digest_t digest, /**< the digest created with crypto_create_hash */
    void*        dst     /**< pointer to the data to write the final hash. the size depends on the diget type */
) {
  if (dst && digest.ctx) {
    switch (digest.type) {
      case DIGEST_SHA256:
      case DIGEST_SHA256_BTC: {
        if (digest.type == DIGEST_SHA256_BTC) {
          bytes32_t tmp;
          ocrypto_sha256_final(digest.ctx, tmp);
          ocrypto_sha256_init(digest.ctx);
          ocrypto_sha256_update(digest.ctx, tmp, 32);
        }
        ocrypto_sha256_final(digest.ctx, dst); /**< generator state is updated to final hash for the message chunk */
        break;
      }
      default: break;
    }
  }
  _free(digest.ctx);
}

static in3_ret_t crypto_pk_to_public_key(
    in3_curve_type_t type,  /**<  type of elliptic curve algorithm */
    const uint8_t*   pk,    /**<  secret key */
    uint8_t*         pubkey /**<  generated pbulickey */
) {
  in3_ret_t res = IN3_OK;
  switch (type) {
    case ECDSA_SECP256R1: {
      res = (ocrypto_ecdsa_p256_public_key(pubkey, pk) < 0 ? IN3_EINVAL : IN3_OK); /**<  public key is computed using the ecdsa secp256r1 digital signature scheme */
      return res;
    }
    case EDDSA_ED25519: {
      ocrypto_ed25519_public_key(pubkey, pk); /**<  public key is computed using the eddsa  digital signature scheme */
      return IN3_OK;
    }
    case ECDH_SECP256R1: {
      res = (ocrypto_ecdh_p256_public_key(pubkey, pk) < 0 ? IN3_EINVAL : IN3_OK); /**<  public key is computed using the eddsa  digital signature scheme */
      return res;
    }
    default: return IN3_ENOTSUP;
  }
}

in3_ret_t crypto_sign_digest(
    in3_curve_type_t type,   /**<  type of elliptic curve algorithm */
    const bytes_t    digest, /**<  created hash */
    const uint8_t*   pk,     /**<  secret key */
    const uint8_t*   pubkey, /**<  signers public key */
    uint8_t*         dst     /**<  generated signature */
) {
  switch (type) {
    case ECDSA_SECP256R1: {
      ocrypto_ecdsa_p256_det_sign_hash(dst, digest.data, pk); /**< SHA256 hash digest is signed using secret key and session key generated from hash and key */
      return IN3_OK;
    }
    case EDDSA_ED25519: {
      ocrypto_ed25519_sign(dst, digest.data, digest.len, pk, pubkey); /**< SHA256 hash digest is signed using public key and secret key using ECDSA P-256 signature generation */
      return IN3_OK;
    }
    default: return IN3_ENOTSUP;
  }
}

in3_ret_t crypto_verify_signature(
    in3_curve_type_t type,    /**<  type of elliptic curve algorithm */
    const uint8_t    sig[64], /**<  signature generated */
    const uint8_t*   pubkey,  /**<  signers public key */
    const bytes_t    digest   /**<  input hash */
) {
  in3_ret_t res = IN3_OK;
  switch (type) {
    case ECDSA_SECP256R1: {
      res = ocrypto_ecdsa_p256_verify_hash(sig, digest.data, pubkey); /**< SHA256 hash digest is signed using secret key and session key generated from hash and key */
      return res;
    }
    case EDDSA_ED25519: {
      res = ocrypto_ed25519_verify(sig, digest.data, digest.len, pubkey); /**< SHA256 hash digest is signed using public key and secret key using ECDSA P-256 signature generation */
      return res;
    }
    default: return IN3_ENOTSUP;
  }
}

in3_ret_t crypto_recover(in3_curve_type_t type, const bytes_t digest, bytes_t signature, uint8_t* dst) {
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
#endif