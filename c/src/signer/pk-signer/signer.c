/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
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

#include "../../core/client/client.h"
#include "../../core/client/keys.h"
#include "../../core/util/mem.h"

#if defined(LEDGER_NANO)
#include "../../signer/ledger-nano/signer/ledger_signer.h"
#endif

#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/secp256k1.h"
#include "../../verifier/eth1/nano/serialize.h"
#include "signer-priv.h"

/** minimum signer for the wallet, returns the signed message which needs to be freed **/
char* eth_wallet_sign(const char* key, const char* data) {
  int     data_l = strlen(data) / 2 - 1;
  uint8_t key_bytes[32], *data_bytes = alloca(data_l + 1), dst[65];

  hex_to_bytes((char*) key + 2, -1, key_bytes, 32);
  data_l    = hex_to_bytes((char*) data + 2, -1, data_bytes, data_l + 1);
  char* res = malloc(133);

  if (ecdsa_sign(&secp256k1, HASHER_SHA3K, key_bytes, data_bytes, data_l, dst, dst + 64, NULL) >= 0) {
    bytes_to_hex(dst, 65, res + 2);
    res[0] = '0';
    res[1] = 'x';
  }

  return res;
}

/**  in3 utiliy to sign the given data with give private key with option to hash data or not */
in3_ret_t ec_sign_pk(d_signature_type_t type, bytes_t message, uint8_t* pk, uint8_t* dst) {
  switch (type) {
    case SIGN_EC_RAW:
      return ec_sign_pk_raw(message.data, pk, dst);
    case SIGN_EC_HASH:
      return ec_sign_pk_hash(message.data, message.len, pk, hasher_sha3k, dst);
    default:
      return IN3_ENOTSUP;
  }
  return 65;
}

/** hash data with given hasher type and sign the given data with give private key*/
in3_ret_t ec_sign_pk_hash(uint8_t* message, size_t len, uint8_t* pk, hasher_t hasher, uint8_t* dst) {
  if (hasher == hasher_sha3k && ecdsa_sign(&secp256k1, HASHER_SHA3K, pk, message, len, dst, dst + 64, NULL) < 0)
    return IN3_EUNKNOWN;
  return 65;
}

/**  sign the given data with give private key */
in3_ret_t ec_sign_pk_raw(uint8_t* message, uint8_t* pk, uint8_t* dst) {
  if (ecdsa_sign_digest(&secp256k1, pk, message, dst, dst + 64, NULL) < 0)
    return IN3_EUNKNOWN;
  return 65;
}

/** signs the given data */
in3_ret_t eth_sign_pk_ctx(void* ctx, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst) {
  UNUSED_VAR(account); // at least for now
  uint8_t* pk = ((in3_ctx_t*) ctx)->client->signer->wallet;
  switch (type) {
    case SIGN_EC_RAW:
      return ec_sign_pk_raw(message.data, pk, dst);
    case SIGN_EC_HASH:
      return ec_sign_pk_hash(message.data, message.len, pk, hasher_sha3k, dst);
    default:
      return IN3_ENOTSUP;
  }
  return 65;
}

/** sets the signer and a pk to the client*/
in3_ret_t eth_set_pk_signer(in3_t* in3, bytes32_t pk) {
  if (in3->signer) _free(in3->signer);
  in3->signer             = _malloc(sizeof(in3_signer_t));
  in3->signer->sign       = eth_sign_pk_ctx;
  in3->signer->prepare_tx = NULL;
  in3->signer->wallet     = pk;
  return IN3_OK;
}

/** sets the signer and a pk to the client*/
uint8_t* eth_set_pk_signer_hex(in3_t* in3, char* key) {
  if (key[0] == '0' && key[1] == 'x') key += 2;
  if (strlen(key) != 64) return NULL;
  uint8_t* key_bytes = _malloc(32);
  hex_to_bytes(key, 64, key_bytes, 32);
  in3_ret_t res = eth_set_pk_signer(in3, key_bytes);
  if (res) {
    _free(key_bytes);
    return NULL;
  }
  return key_bytes;
}
