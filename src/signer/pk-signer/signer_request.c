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

/// RPC-signer
/** signs a reques*/
in3_ret_t eth_sign_req(void* data, in3_plugin_act_t action, void* action_ctx) {
  signer_key_t* k = data;
  switch (action) {
    case PLGN_ACT_PAY_SIGN_REQ: {
      in3_pay_sign_req_ctx_t* ctx = action_ctx;
      in3_ret_t               r   = crypto_sign_digest(ECDSA_SECP256K1, bytes(ctx->request_hash, 32), k->pk, NULL, ctx->signature);
      ctx->signature[64] += 27;
      return r;
    }
    case PLGN_ACT_SIGN: {
      in3_sign_ctx_t* ctx = action_ctx;
      if (ctx->account.len != 20 || memcmp(k->account, ctx->account.data, 20)) return IN3_EIGNORE;
      ctx->signature = bytes(_malloc(65), 65);
      switch (ctx->digest_type) {
        case SIGN_EC_RAW:
          return crypto_sign_digest(ECDSA_SECP256K1, ctx->message, k->pk, NULL, ctx->signature.data);
        case SIGN_EC_HASH: {
          bytes32_t hash;
          keccak(ctx->message, hash);
          return crypto_sign_digest(ECDSA_SECP256K1, bytes(hash, 32), k->pk, NULL, ctx->signature.data);
        }
        default:
          _free(ctx->signature.data);
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
in3_ret_t eth_set_request_signer(in3_t* in3, bytes32_t pk) {
  signer_key_t* k = _malloc(sizeof(signer_key_t));
  memcpy(k->pk, pk, 32);
  k->account_len = eth_get_address(pk, k->account, ECDSA_SECP256K1);
  k->type        = ECDSA_SECP256K1;
  return in3_plugin_register(in3, PLGN_ACT_PAY_SIGN_REQ | PLGN_ACT_TERM | PLGN_ACT_SIGN, eth_sign_req, k, true);
}
