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

typedef struct account {
  char*        path;
  uint8_t      account[64];
  unsigned int account_len;
} account_t;

typedef struct seed {
  uint8_t          seed[64];
  unsigned int     seed_len;
  account_t*       paths;
  unsigned int     account_len;
  in3_curve_type_t type;
} seed_t;

static account_t* find_account(seed_t* seed, bytes_t account) {
  for (unsigned int i = 0; i < seed->account_len; i++) {
    if (account.len == 0 || (seed->paths[i].account_len == account.len && memcmp(seed->paths[i].account, account.data, account.len) == 0)) return seed->paths + i;
  }
  return NULL;
}

// signer_pk implementation
static in3_ret_t hd_handle(void* data, in3_plugin_act_t action, void* action_ctx) {
  seed_t* k = data;
  switch (action) {
    case PLGN_ACT_SIGN: {
      in3_sign_ctx_t* ctx     = action_ctx;
      bytes32_t       pk      = {0};
      account_t*      account = find_account(k, ctx->account);
      if (!account || k->type != ctx->curve_type) return IN3_EIGNORE;
      bip32(bytes(k->seed, k->seed_len), k->type, account->path, pk);

      switch (ctx->curve_type) {
        case ECDSA_SECP256K1:
          ctx->signature = sign_with_pk(pk, ctx->message, ctx->digest_type);
          break;
        case EDDSA_ED25519: {
          uint8_t sig[64];
          TRY(crypto_sign_digest(EDDSA_ED25519, ctx->message, pk, account->account, sig))
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
      ctx->accounts               = _malloc(20 * k->account_len);
      ctx->accounts_len           = k->account_len;
      for (unsigned int i = 0; i < k->account_len; i++)
        memcpy(ctx->accounts + i * 20, k->paths[i].account, 20);

      return IN3_OK;
    }

    case PLGN_ACT_SIGN_PUBLICKEY: {
      // generate the address from the key
      in3_sign_public_key_ctx_t* ctx     = action_ctx;
      bytes32_t                  pk      = {0};
      account_t*                 account = find_account(k, bytes(ctx->account, 20));
      if (!account || k->type != ctx->curve_type) return IN3_EIGNORE;
      bip32(bytes(k->seed, k->seed_len), k->type, account->path, pk);

      // is it our key?
      switch (ctx->curve_type) {
        case ECDSA_SECP256K1:
          return crypto_convert(ECDSA_SECP256K1, ctx->convert_type, bytes(pk, 32), ctx->public_key, NULL);
        case EDDSA_ED25519:
          return crypto_convert(EDDSA_ED25519, CONV_PK32_TO_PUB32, bytes(pk, 32), ctx->public_key, NULL);
        default:
          return IN3_ENOTSUP;
      }
    }

    case PLGN_ACT_TERM: {
      for (unsigned int i = 0; i < k->account_len; i++)
        _free(k->paths[i].path);
      _free(k->paths);
      _free(k);
      return IN3_OK;
    }

    default:
      return IN3_ENOTSUP;
  }
}

static seed_t* find_hd_signer(in3_t* in3, bytes32_t seed_id) {
  for (in3_plugin_t* p = in3->plugins; p; p = p->next) {
    if (p->action_fn == hd_handle) {
      seed_t* seed = p->data;
      if (seed_id) {
        bytes32_t tmp;
        keccak(bytes(seed->seed, seed->seed_len), tmp);
        if (memcmp(tmp, seed_id, 32)) continue;
      }
      return seed;
    }
  }
  return NULL;
}

/** sets the signer and a pk to the client*/
in3_ret_t register_hd_signer(in3_t* in3, bytes_t seed, in3_curve_type_t type, bytes32_t seed_id) {
  if (!seed.data || seed.len > 64) return IN3_EINVAL;
  seed_t* k   = _calloc(sizeof(seed_t), 1);
  k->type     = type;
  k->seed_len = seed.len;
  memcpy(k->seed, seed.data, seed.len);
  if (seed_id) keccak(seed, seed_id);
  return in3_plugin_register(in3, PLGN_ACT_SIGN_ACCOUNT | PLGN_ACT_SIGN | PLGN_ACT_TERM | PLGN_ACT_SIGN_PUBLICKEY, hd_handle, k, false);
}

/** sets the signer and a pk to the client*/
in3_ret_t hd_signer_add_path(in3_t* in3, bytes32_t seed_id, char* path, uint8_t** address) {
  seed_t*    seed = find_hd_signer(in3, seed_id);
  account_t* ac   = NULL;
  if (!seed) return IN3_EFIND;
  // already added?
  for (unsigned int i = 0; i < seed->account_len; i++) {
    if (strcmp(seed->paths[i].path, path) == 0) ac = seed->paths + i;
  }
  if (!ac) {
    bytes32_t pk = {0};
    seed->paths  = seed->account_len == 0 ? _malloc(sizeof(account_t)) : _realloc(seed->paths, sizeof(account_t) * (seed->account_len + 1), sizeof(account_t) * seed->account_len);
    ac           = seed->paths + seed->account_len;
    ac->path     = _strdupn(path, -1);
    bip32(bytes(seed->seed, seed->seed_len), seed->type, path, pk);
    ac->account_len = eth_get_address(pk, ac->account, seed->type);
    seed->account_len++;
  }

  if (address) *address = ac->account;
  return IN3_OK;
}
