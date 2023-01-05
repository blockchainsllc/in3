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
#include "cryptocell_signer.h"

#include <stdio.h>
#include <string.h>

#define PRIVATE_KEY_SIZE 32

typedef struct cryptocell_signer_key {
  uint8_t          pk[32];
  bytes_t*         data;
  uint8_t          pub_key[65];
  unsigned int     pub_key_len;
  in3_curve_type_t type;
} cryptocell_signer_key_t;

cryptocell_signer_key_t* configure_cryptocell_signer_key(cryptocell_signer_info_t* signer_info);

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

cryptocell_signer_key_t* configure_cryptocell_signer_key(cryptocell_signer_info_t* signer_info) {
  cryptocell_signer_key_t* signer_key = (cryptocell_signer_key_t*) k_malloc(sizeof(cryptocell_signer_key_t));
  signer_key->data                    = signer_info->msg;
  signer_key->type                    = ECDSA_SECP256K1;
  if (pk_identity_key_is_stored(signer_info->ik_slot) != 0) {
    if (generate_pk_keypair_ecdsa_sha256(signer_key->pk, signer_key->pub_key) == 0) {
      if (store_pk_identity_keyslot_kmu(signer_info->ik_slot, signer_key->pk) == 0) {
        if (load_pk_identity_keyslot_kmu(signer_info->ik_slot, signer_key->pk) == 0) {
          return signer_key;
        }
      }
    }
  }
  else {
    if (load_pk_identity_keyslot_kmu(signer_info->ik_slot, signer_key->pk) == 0) {
      return signer_key;
    }
  }
  return NULL;
}

/** Set the cryptocell signer and register as plugin to in3 client */
in3_ret_t eth_set_cryptocell_signer(in3_t* in3, cryptocell_signer_info_t* signer_info) {
  cryptocell_signer_key_t* signer_key = configure_cryptocell_signer_key(signer_info);
  if (signer_key != NULL) {
    switch (signer_info->curve_type) {
      case SIGN_CURVE_ECDSA: {
        if (signer_key->type == ECDSA_SECP256K1) {
          return in3_plugin_register(in3, PLGN_ACT_SIGN | PLGN_ACT_SIGN_PUBLICKEY, cryptocell_signer_cbk, signer_key, false);
        }
      }
      case SIGN_CURVE_ECDH: {
        return IN3_ENOTSUP;
      }
      default: {
        return IN3_ENOTSUP;
      }
    }
  }
  return IN3_OK;
}

/** set the cryptocell signer configuration informations */
int register_cryptocell_cbk(cryptocell_cbks_t* cbks) {
  if (cbks) {
    cbks->gen_pk_func = generate_pk_keypair_ecdsa_sha256;
    cbks->str_pk_func = store_pk_identity_keyslot_kmu;
    cbks->ld_pk_func  = load_pk_identity_keyslot_kmu;
    cbks->des_pk_func = destroy_key;
    return SUCCESS;
  }
  return ERROR;
}