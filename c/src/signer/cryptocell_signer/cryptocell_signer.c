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
#include "../../../include/in3/cryptocell_signer.h"
#include <stdio.h>
#include <string.h>

#define PRIVATE_KEY_SIZE 32

typedef struct cryptocell_signer_key {
  uint8_t          pk[32];
  bytes_t*         data;
  uint8_t          account[64];
  unsigned int     account_len;
  in3_curve_type_t type;
} cryptocell_signer_key_t;

in3_ret_t configure_cryptocell_signer_key(cryptocell_signer_info_t* signer_info, cryptocell_signer_key_t* signer_key);

static bytes_t sign_with_cryptocell_pk(const bytes32_t pk, const bytes_t data, const d_digest_type_t type) {
  bytes_t res = bytes(_malloc(65), 65);
  switch (type) {
    case SIGN_EC_RAW:
      if (crypto_sign_digest(ECDSA_SECP256K1, data, pk, NULL, res.data)) {
        _free(res.data);
        res = NULL_BYTES;
      }
      break;
    default:
      _free(res.data);
      res = NULL_BYTES;
  }
  return res;
}

static in3_ret_t cryptocell_signer_cbk(void* data, in3_plugin_act_t action, void* action_ctx) {
  cryptocell_signer_key_t* signer_key = (cryptocell_signer_key_t*) data;
  switch (action) {
    case PLGN_ACT_SIGN: {
      in3_sign_ctx_t* ctx = (in3_sign_ctx_t*) action_ctx;
      switch (signer_key->type) {
        case ECDSA_SECP256K1:
          ctx->signature = sign_with_cryptocell_pk(signer_key->pk, bytes(signer_key->data->data, signer_key->data->len), SIGN_EC_RAW);
          break;
        default:
          break;
      }
      return ctx->signature.data ? IN3_OK : IN3_ENOTSUP;
    }
    case PLGN_ACT_SIGN_PUBLICKEY: {
      // generate the address from the key
      in3_sign_public_key_ctx_t* ctx = (in3_sign_public_key_ctx_t*) action_ctx;
      switch (ctx->curve_type) {
        case ECDSA_SECP256K1:
          return IN3_ENOTSUP;
        default:
          return IN3_ENOTSUP;
      }
    }
    default:
      return IN3_ENOTSUP;
  }
}

in3_ret_t configure_cryptocell_signer_key(cryptocell_signer_info_t* signer_info, cryptocell_signer_key_t* signer_key) {
  switch (signer_info->curve_type) {
    case SIGN_CURVE_ECDSA: {
      if (signer_info->cbks->ld_pk_func(signer_info->ik_slot, signer_key->pk) < 0) {
        // TODO: Private key generation and storage based on various conditions
        if (signer_info->cbks->gen_pk_func(signer_key->pk) == 0) {
          // memcpy(private_key, signer_key->pk, PRIVATE_KEY_SIZE);
          //  store the generated private key in selected identity key slot in KMU.
          // if (signer_info->cbks->str_pk_func(signer_info->ik_slot, private_key) < 0) {
          // printk("Identity key storage to %d failed\n", signer_info->ik_slot);}
        }
      }
      signer_key->data = signer_info->msg;
      signer_key->type = ECDSA_SECP256K1;
      return IN3_OK;
    }
    case SIGN_CURVE_ECDH: {
      return IN3_ENOTSUP;
    }
    default: {
      return IN3_ENOTSUP;
    }
  }
}

/** Set the cryptocell signer and register as plugin to in3 client */
in3_ret_t eth_set_cryptocell_signer(in3_t* in3, cryptocell_signer_info_t* signer_info) {
  in3_ret_t                status;
  cryptocell_signer_key_t* signer_key;
#ifdef __ZEPHYR__
  signer_key = (cryptocell_signer_key_t*) k_malloc(sizeof(cryptocell_signer_key_t));
#endif
  configure_cryptocell_signer_key(signer_info, signer_key);
  switch (signer_key->type) {
    case ECDSA_SECP256K1: {
      status = in3_plugin_register(in3, PLGN_ACT_SIGN | PLGN_ACT_SIGN_PUBLICKEY, cryptocell_signer_cbk, signer_key, false);
      return status;
    }
    case EDDSA_ED25519: {
      return IN3_ENOTSUP;
    }
    default: {
      return IN3_ENOTSUP;
    }
  }
}