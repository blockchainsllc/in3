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

#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/crypto.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "../../verifier/eth1/nano/serialize.h"
#include "signer.h"
#include <string.h>

typedef struct signer_key {
  bytes32_t        pk;
  uint8_t          account[64];
  unsigned int     account_len;
  in3_curve_type_t type;
} signer_key_t;

// signer_pk implementation
static in3_ret_t pk_handle(void* data, in3_plugin_act_t action, void* action_ctx) {
  signer_key_t* k = data;
  switch (action) {
    case PLGN_ACT_SIGN: {
      in3_sign_ctx_t* ctx = action_ctx;

      // is it our key?
      if (ctx->account.len && (ctx->account.len != k->account_len || memcmp(k->account, ctx->account.data, k->account_len))) return IN3_EIGNORE;
      switch (ctx->curve_type) {
        case ECDSA_SECP256K1:
          ctx->signature = sign_with_pk(k->pk, ctx->message, ctx->digest_type);
          break;
        case EDDSA_ED25519: {
          uint8_t sig[64];
          TRY(crypto_sign_digest(EDDSA_ED25519, ctx->message, k->pk, k->account, sig))
          ctx->signature = bytes_dup(bytes(sig, 64));
        }
      }
      return ctx->signature.data ? IN3_OK : IN3_ENOTSUP;
    }

    case PLGN_ACT_SIGN_ACCOUNT: {
      // generate the address from the key
      in3_sign_account_ctx_t* ctx = action_ctx;
      ctx->signer_type            = SIGNER_ECDSA;
      ctx->curve_type             = k->type;
      ctx->accounts               = _malloc(20);
      ctx->accounts_len           = 1;
      memcpy(ctx->accounts, k->account, 20);
      return IN3_OK;
    }

    case PLGN_ACT_SIGN_PUBLICKEY: {
      // generate the address from the key
      in3_sign_public_key_ctx_t* ctx = action_ctx;

      // is it our key?
      if (k->type != ctx->curve_type || (ctx->account && memcmp(ctx->account, k->account, k->account_len))) return IN3_EIGNORE;
      switch (ctx->curve_type) {
        case ECDSA_SECP256K1:
          return crypto_convert(ECDSA_SECP256K1, ctx->convert_type, bytes(k->pk, 32), ctx->public_key, NULL);
        case EDDSA_ED25519:
          return crypto_convert(EDDSA_ED25519, CONV_PK32_TO_PUB32, bytes(k->pk, 32), ctx->public_key, NULL);
        default:
          return IN3_ENOTSUP;
      }
    }

    case PLGN_ACT_TERM: {
      _free(k);
      return IN3_OK;
    }

    default:
      return IN3_ENOTSUP;
  }
}

/** sets the signer and a pk to the client*/
in3_ret_t eth_set_pk_signer(in3_t* in3, bytes32_t pk, in3_curve_type_t type, uint8_t** address) {
  if (!pk) return IN3_EINVAL;
  signer_key_t* k = _malloc(sizeof(signer_key_t));
  k->type         = type;
  k->account_len  = eth_get_address(pk, k->account, type);
  memcpy(k->pk, pk, 32);
  if (address) *address = k->account;
  return in3_plugin_register(in3, PLGN_ACT_SIGN_ACCOUNT | PLGN_ACT_SIGN | PLGN_ACT_TERM | PLGN_ACT_SIGN_PUBLICKEY, pk_handle, k, false);
}
