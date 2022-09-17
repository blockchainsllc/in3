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

#include "signer.h"
#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/crypto.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "../../verifier/eth1/nano/serialize.h"
#include "pk-signer_rpc.h"
#include <string.h>

static in3_ret_t pk_add_from_config(in3_configure_ctx_t* ctx, in3_curve_type_t type) {
  if (d_type(ctx->token) == T_ARRAY) {
    for_children_of(iter, ctx->token) {
      bytes_t b = d_bytes(iter.token);
      if (b.len != 32) {
        ctx->error_msg = _strdupn("invalid key-length, must be 32", -1);
        return IN3_EINVAL;
      }
      signer_add_key(ctx->client, b.data, type);
    }
  }
  else {
    bytes_t b = d_bytes(ctx->token);
    if (b.len != 32) {
      ctx->error_msg = _strdupn("invalid key-length, must be 32", -1);
      return IN3_EINVAL;
    }
    signer_add_key(ctx->client, b.data, type);
  }
  return IN3_OK;
}

static in3_ret_t add_hd(in3_configure_ctx_t* ctx, d_token_t* token) {
  uint8_t          seed[64]   = {0};
  bytes32_t        seed_id    = {0};
  in3_curve_type_t ct         = ECDSA_SECP256K1;
  char*            mnemomic   = d_get_string(token, key("seed_phrase"));
  char*            passphrase = d_get_string(token, key("seed_password"));
  bytes_t          seed_val   = d_get_bytes(token, key("seed"));
  d_token_t*       derivation = d_get(token, key("paths"));

  // verify
  if (mnemomic) {
    if (mnemonic_verify(mnemomic)) CNF_ERROR("Invalid seedphrase");
    // extract seed
    mnemonic_to_seed(mnemomic, passphrase ? passphrase : "", seed, NULL);
    seed_val = bytes(seed, 64);
  }
  else if (!seed_val.len || seed_val.len > 64)
    CNF_ERROR("Invalid seed");

  // register hd signer
  TRY(register_hd_signer(ctx->client, seed_val, ct, seed_id))

  // derrive
  if (d_type(derivation) == T_ARRAY) {
    for_children_of(iter, derivation) hd_signer_add_path(ctx->client, seed_id, d_string(iter.token), NULL);
  }
  else {
    char* path = d_string(derivation);
    if (!path) path = "m/44'/60'/0'/0/0";
    hd_signer_add_path(ctx->client, seed_id, path, NULL);
  }
  return IN3_OK;
}

static in3_ret_t pk_config_set(in3_configure_ctx_t* ctx) {
  if (d_is_key(ctx->token, CONFIG_KEY("key"))) {
    bytes_t b = d_bytes(ctx->token);
    if (b.len != 32) {
      ctx->error_msg = _strdupn("invalid key-length, must be 32", -1);
      return IN3_EINVAL;
    }
    eth_set_request_signer(ctx->client, b.data);
    return IN3_OK;
  }
  if (d_is_key(ctx->token, CONFIG_KEY("pk"))) return pk_add_from_config(ctx, ECDSA_SECP256K1);
  if (d_is_key(ctx->token, CONFIG_KEY("pk_ed25519"))) return pk_add_from_config(ctx, EDDSA_ED25519);
  if (d_is_key(ctx->token, CONFIG_KEY("hd"))) {
    if (d_type(ctx->token) == T_OBJECT)
      return add_hd(ctx, ctx->token);
    else if (d_type(ctx->token) == T_ARRAY) {
      for_children_of(iter, ctx->token) TRY(add_hd(ctx, iter.token));
      return IN3_OK;
    }
    else
      CNF_ERROR("Invalid hd-wallet");
  }

  return IN3_EIGNORE;
}

// RPC-Handler
static in3_ret_t pk_rpc(void* data, in3_plugin_act_t action, void* action_ctx) {
  UNUSED_VAR(data);
  switch (action) {
    case PLGN_ACT_CONFIG_SET: return pk_config_set(action_ctx);
    case PLGN_ACT_RPC_HANDLE: return pk_signer_rpc(action_ctx);
    default: return IN3_ENOTSUP;
  }
}

/** register the signer as rpc-plugin providing accounts management functions */
in3_ret_t eth_register_pk_signer(in3_t* in3) {
  return in3_plugin_register(in3, PLGN_ACT_CONFIG_SET | PLGN_ACT_RPC_HANDLE, pk_rpc, NULL, true);
}
