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
#include "pk-signer_rpc.h"
#include "signer.h"
#include <string.h>

in3_ret_t in3_addJsonKey(in3_rpc_handle_ctx_t* ctx, d_token_t* data, char* passphrase) {
  address_t  adr    = {0};
  d_token_t* res    = NULL;
  char*      params = sprintx("%j,\"%S\"", data, passphrase);
  TRY_FINAL(req_send_sub_request(ctx->req, "in3_decryptKey", params, NULL, &res, NULL), _free(params))

  bytes_t pk = d_bytes(res);
  if (!pk.data && pk.len != 32) return req_set_error(ctx->req, "invalid key", IN3_EINVAL);
  if (!signer_add_key(ctx->req->client, pk.data, SIGN_CURVE_ECDSA)) return req_set_error(ctx->req, "key already exists", IN3_EINVAL);
  return in3_rpc_handle_with_bytes(ctx, bytes(adr, eth_get_address(pk.data, adr, SIGN_CURVE_ECDSA)));
}

/**
 * adds a raw private key as signer, which allows signing transactions.
 */
in3_ret_t in3_addRawKey(in3_rpc_handle_ctx_t* ctx, bytes_t pk, char* curve) {
  d_curve_type_t ct = SIGN_CURVE_ECDSA;
  if (curve && strcmp(curve, "ed25519") == 0) ct = SIGN_CURVE_ED25519;

  uint8_t adr[64];
  if (!signer_add_key(ctx->req->client, pk.data, ct)) return req_set_error(ctx->req, "key already exists", IN3_EINVAL);
  return in3_rpc_handle_with_bytes(ctx, bytes(adr, eth_get_address(pk.data, adr, ct)));
}

/**
 * adds a signer from a mnemomic phrase
 */
in3_ret_t in3_addMnemonic(in3_rpc_handle_ctx_t* ctx, char* mnemomic, char* passphrase, d_token_t* derivation, char* curve) {
  uint8_t        seed[64];
  sb_t           path = {0};
  d_curve_type_t ct   = SIGN_CURVE_ECDSA;
  if (curve && strcmp(curve, "ed25519") == 0) ct = SIGN_CURVE_ED25519;
  if (mnemonic_verify(mnemomic)) return req_set_error(ctx->req, "Invalid mnemonic!", IN3_ERPC);

  // extract seed
  mnemonic_to_seed(mnemomic, passphrase ? passphrase : "", seed, NULL);

  // derrive
  if (d_type(derivation) == T_ARRAY) {
    for_children_of(iter, derivation) {
      if (path.len) sb_add_char(&path, ' ');
      sb_add_chars(&path, d_string(iter.token));
    }
  }
  else if (d_type(derivation) == T_STRING)
    sb_add_chars(&path, d_string(derivation));
  else
    sb_add_chars(&path, "m/44'/60'/0'/0/0");

  // count the paths
  int l = 1;
  for (int i = 0; i < (int) path.len; i++) {
    if (path.data[i] == ' ' || path.data[i] == ',') l++;
  }

  // derrive all private keys
  uint8_t*  pks = _malloc(l * 32);
  in3_ret_t r   = bip32(bytes(seed, 64), ECDSA_SECP256K1, path.data, pks);
  _free(path.data);
  memzero(seed, 64);

  if (r == IN3_OK) {

    // add keys and prepare the response
    uint8_t adr[64] = {0};
    sb_t*   sb      = in3_rpc_handle_start(ctx);
    sb_add_char(sb, '[');
    for (int i = 0; i < l; i++) {
      if (signer_add_key(ctx->req->client, pks + i * 32, ct))
        sb_add_value(sb, "\"%B\"", bytes(adr, eth_get_address(pks + i * 32, adr, ct)));
    }
    sb_add_char(sb, ']');

    // cleanup
    memzero(pks, l * 32);
    _free(pks);
    return in3_rpc_handle_finish(ctx);
  }
  else {
    _free(pks);
    return req_set_error(ctx->req, "Invalid seed or bip39 not supported!", r);
  }
}

/**
 * returns a array of signer_ids the client is able to sign with.
 *
 * In order to add keys, you can use [in3_addRawKey](#in3-addrawkey) or configure them in the config. The result also contains the signer_ids of any signer signer-supporting the `PLGN_ACT_SIGN_ACCOUNT` action.
 */
in3_ret_t signer_ids(in3_rpc_handle_ctx_t* ctx) {
  sb_t*                  sb = in3_rpc_handle_start(ctx);
  in3_sign_account_ctx_t sc = {.req = ctx->req, .accounts = NULL, .accounts_len = 0, .signer_type = 0, .curve_type = SIGN_CURVE_ECDSA};

  sb_add_char(sb, '[');
  for (in3_plugin_t* p = ctx->req->client->plugins; p; p = p->next) {
    if (p->acts & PLGN_ACT_SIGN_ACCOUNT && p->action_fn(p->data, PLGN_ACT_SIGN_ACCOUNT, &sc) == IN3_OK) {
      for (int i = 0; i < sc.accounts_len; i++)
        sb_add_value(sb, "\"%B\"", bytes(sc.accounts + i * 20, 20));
      if (sc.accounts) {
        _free(sc.accounts);
        sc.accounts_len = 0;
      }
    }
  }
  sb_add_char(sb, ']');
  return in3_rpc_handle_finish(ctx);
}

/**
 * alias to signer_ids
 */
in3_ret_t eth_accounts(in3_rpc_handle_ctx_t* ctx) {
  return signer_ids(ctx);
}
