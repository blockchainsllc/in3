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

#include "signer.h"
#include "../../core/client/client.h"
#include "../../core/client/keys.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/secp256k1.h"
#include "../../verifier/eth1/nano/serialize.h"
/** hash data with given hasher type and sign the given data with give private key*/
in3_ret_t ec_sign_pk_hash(uint8_t* message, size_t len, uint8_t* pk, hasher_t hasher, uint8_t* dst) {
  if (hasher == hasher_sha3k && ecdsa_sign(&secp256k1, HASHER_SHA3K, pk, message, len, dst, dst + 64, NULL) < 0)
    return IN3_EUNKNOWN;
  return IN3_OK;
}

/**  sign the given data with give private key */
in3_ret_t ec_sign_pk_raw(uint8_t* message, uint8_t* pk, uint8_t* dst) {
  if (ecdsa_sign_digest(&secp256k1, pk, message, dst, dst + 64, NULL) < 0)
    return IN3_EUNKNOWN;
  return IN3_OK;
}
in3_ret_t eth_sign_pk(void* data, in3_plugin_act_t action, void* action_ctx) {
  uint8_t* pk = data;
  switch (action) {
    case PLGN_ACT_SIGN: {
      in3_sign_ctx_t* ctx = action_ctx;
      switch (ctx->type) {
        case SIGN_EC_RAW:
          return ec_sign_pk_raw(ctx->message.data, pk, ctx->signature);
        case SIGN_EC_HASH:
          return ec_sign_pk_hash(ctx->message.data, ctx->message.len, pk, hasher_sha3k, ctx->signature);
        default:
          return IN3_ENOTSUP;
      }
    }

    case PLGN_ACT_SIGN_ACCOUNT: {
      // generate the address from the key
      uint8_t                 public_key[65], sdata[32];
      in3_sign_account_ctx_t* ctx = action_ctx;

      ecdsa_get_public_key65(&secp256k1, pk, public_key);
      keccak(bytes(public_key + 1, 64), sdata);
      memcpy(ctx->account, sdata + 12, 20);
      return IN3_OK;
    }

    default:
      return IN3_ENOTSUP;
  }
}

/** sets the signer and a pk to the client*/
in3_ret_t eth_set_pk_signer(in3_t* in3, bytes32_t pk) {
  return in3_plugin_register(in3, PLGN_ACT_SIGN_ACCOUNT | PLGN_ACT_SIGN, eth_sign_pk, pk, false);
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
