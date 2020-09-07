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
#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/secp256k1.h"
#include "../../verifier/eth1/nano/serialize.h"
#include <string.h>

typedef struct key {
  bytes32_t key;
} signer_key_t;

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

static void get_address(uint8_t* pk, uint8_t* address) {
  uint8_t public_key[65], sdata[32];
  ecdsa_get_public_key65(&secp256k1, pk, public_key);
  keccak(bytes(public_key + 1, 64), sdata);
  memcpy(address, sdata + 12, 20);
}
in3_ret_t eth_sign_pk(void* data, in3_plugin_act_t action, void* action_ctx) {
  uint8_t* pk = data;
  switch (action) {
    case PLGN_ACT_SIGN: {
      in3_sign_ctx_t* ctx = action_ctx;
      if (ctx->account.len == 20) {
        address_t adr;
        get_address(pk, adr);
        if (memcmp(adr, ctx->account.data, 20)) return IN3_EIGNORE;
      }
      ctx->signature = bytes(_malloc(65), 65);

      switch (ctx->type) {
        case SIGN_EC_RAW:
          return ec_sign_pk_raw(ctx->message.data, pk, ctx->signature.data);
        case SIGN_EC_HASH:
          return ec_sign_pk_hash(ctx->message.data, ctx->message.len, pk, hasher_sha3k, ctx->signature.data);
        default:
          _free(ctx->signature.data);
          return IN3_ENOTSUP;
      }
    }

    case PLGN_ACT_SIGN_ACCOUNT: {
      // generate the address from the key
      in3_sign_account_ctx_t* ctx = action_ctx;
      get_address(pk, ctx->account);
      return IN3_OK;
    }

    default:
      return IN3_ENOTSUP;
  }
}
in3_ret_t eth_sign_req(void* data, in3_plugin_act_t action, void* action_ctx) {
  signer_key_t* pk = data;
  switch (action) {
    case PLGN_ACT_PAY_SIGN_REQ: {
      in3_pay_sign_req_ctx_t* ctx = action_ctx;
      return ec_sign_pk_raw(ctx->request_hash, pk->key, ctx->signature);
    }

    case PLGN_ACT_TERM: {
      _free(pk);
      return IN3_OK;
    }

    case PLGN_ACT_CONFIG_SET: {
      in3_configure_ctx_t* ctx = action_ctx;
      if (ctx->token->key == key("key")) {
        if (d_type(ctx->token) != T_BYTES || d_len(ctx->token) != 32) {
          ctx->error_msg = "invalid key-length, must be 32";
          return IN3_EINVAL;
        }
        memcpy(pk->key, ctx->token->data, 32);
        return IN3_OK;
      }
      return IN3_EIGNORE;
    }

    case PLGN_ACT_CONFIG_GET: {
      in3_get_config_ctx_t* ctx = action_ctx;
      bytes_t               k   = bytes(pk->key, 32);
      sb_add_bytes(ctx->sb, ",\"key\"=", &k, 1, false);
      return IN3_OK;
    }

    default:
      return IN3_ENOTSUP;
  }
}

/** sets the signer and a pk to the client*/
in3_ret_t eth_set_pk_signer(in3_t* in3, bytes32_t pk) {
  return plugin_register(in3, PLGN_ACT_SIGN_ACCOUNT | PLGN_ACT_SIGN, eth_sign_pk, pk, false);
}

/** sets the signer and a pk to the client*/
in3_ret_t eth_set_request_signer(in3_t* in3, bytes32_t pk) {
  signer_key_t* k = _malloc(sizeof(signer_key_t));
  if (pk) memcpy(k->key, pk, 32);
  return plugin_register(in3, PLGN_ACT_PAY_SIGN_REQ | PLGN_ACT_TERM | PLGN_ACT_CONFIG_GET | PLGN_ACT_CONFIG_SET, eth_sign_req, k, true);
}

in3_ret_t eth_register_request_signer(in3_t* in3) {
  return plugin_register(in3, PLGN_ACT_PAY_SIGN_REQ | PLGN_ACT_TERM | PLGN_ACT_CONFIG_GET | PLGN_ACT_CONFIG_SET, eth_sign_req, _calloc(1, sizeof(signer_key_t)), true);
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
