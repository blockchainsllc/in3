/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2019 slock.it GmbH, Blockchains LLC
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
#include "zksync.h"
#include "../../core/client/context_internal.h"
#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/util/debug.h"
#include "../../core/util/mem.h"
#include "provider.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
static in3_ret_t zksync_get_account(zksync_config_t* conf, in3_ctx_t* ctx, uint8_t** account) {
  if (!conf->account) {
    in3_sign_account_ctx_t sctx = {.ctx = ctx, .account = {0}};
    if (in3_plugin_execute_first(ctx, PLGN_ACT_SIGN_ACCOUNT, &sctx)) return ctx_set_error(ctx, "No account configured or signer set", IN3_ECONFIG);
    memcpy(conf->account = _malloc(20), sctx.account, 20);
  }

  if (account) *account = conf->account;
  return IN3_OK;
}

static in3_ret_t zksync_get_contracts(zksync_config_t* conf, in3_ctx_t* ctx, uint8_t** main) {
  if (!conf->main_contract) {
    d_token_t* result;
    TRY(send_provider_request(ctx, conf, "contract_address", "", &result))
    bytes_t* main_contract = d_get_bytesk(result, key("mainContract"));
    if (!main_contract || main_contract->len != 20) return ctx_set_error(ctx, "could not get the main_contract from provider", IN3_ERPC);
    memcpy(conf->main_contract = _malloc(20), main_contract->data, 20);
    // clean up
    ctx_remove_required(ctx, ctx_find_required(ctx, "contract_address"), false);
  }

  if (main) *main = conf->main_contract;
  return IN3_OK;
}
static bool is_eth(d_token_t* t) {
  if (!t) return true;
  if (d_type(t) == T_STRING && strcmp(d_string(t), "ETH") == 0) return true;
  if (d_type(t) == T_BYTES && t->len == 20 && memiszero(t->data, 20)) return true;
  if (d_type(t) == T_INTEGER && d_int(t) == 0) return true;
  return false;
}

//static in3_ret_t payin_approve()

static in3_ret_t payin(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, d_token_t* tx) {
  d_token_t* tmp;

  // make sure we have an account
  uint8_t* account = conf->account;
  if ((tmp = d_get(tx, key("depositTo")))) {
    if (tmp->len != 20) return ctx_set_error(ctx->ctx, "invalid depositTo", IN3_ERPC);
    account = tmp->data;
  }
  else if (!account)
    TRY(zksync_get_account(conf, ctx->ctx, &account))

  //  amount
  bytes_t    amount        = d_to_bytes(d_get(tx, key("amount")));
  d_token_t* token         = d_get(tx, key("token"));
  bool       approve       = d_get_intk(tx, key("approveDepositAmountForERC20"));
  uint8_t*   main_contract = conf->main_contract;
  if (!main_contract) TRY(zksync_get_contracts(conf, ctx->ctx, &main_contract))

  bytes32_t amount32 = {0};
  memcpy(((uint8_t*) amount32) + 32 - amount.len, amount.data, amount.len);

  d_token_t* tx_receipt;
  if (is_eth(token)) {
    sb_t sb = {0};
    sb_add_rawbytes(&sb, "{\"to\":\"0x", bytes(main_contract, 20), 0);
    sb_add_rawbytes(&sb, "\",\"data\":\"0x2d2da806", bytes(account, 20), 32);
    sb_add_rawbytes(&sb, "\",\"value\":\"0x", amount, 0);
    sb_add_chars(&sb, "\",\"gas\":\"0x30d40\"}");

    in3_ret_t res = send_provider_request(ctx->ctx, NULL, "eth_sendTransactionAndWait", sb.data, &tx_receipt);
    _free(sb.data);
    if (res) return res;

    // we have a transactionhash
  }
  else {
    if (approve) {
    }
  }

  return IN3_OK;
}

static in3_ret_t zksync_rpc(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  char*      method = d_get_stringk(ctx->ctx->requests[0], K_METHOD);
  d_token_t* params = d_get(ctx->ctx->requests[0], K_PARAMS);
  if (strncmp(method, "zksync_", 7)) return IN3_EIGNORE;

  if (strcmp(method, "zksync_depositToSyncFromEthereum") == 0) return payin(conf, ctx, params + 1);

  // do we have a provider?
  if (!conf->provider_url) {
    switch (ctx->ctx->client->chain_id) {
      case CHAIN_ID_MAINNET:
        conf->provider_url = _strdupn("https://api.zksync.io/jsrpc", -1);
        break;
      default:
        return ctx_set_error(ctx->ctx, "no provider_url in config", IN3_EINVAL);
    }
  }

  str_range_t p            = d_to_json(params);
  char*       param_string = alloca(p.len - 1);
  memcpy(param_string, p.data + 1, p.len - 2);
  param_string[p.len - 2] = 0;

  if (strcmp(method, "zksync_account_info") == 0 && *param_string == 0) {
    TRY(zksync_get_account(conf, ctx->ctx, NULL))
    param_string    = alloca(45);
    param_string[0] = '"';
    param_string[1] = '0';
    param_string[2] = 'x';
    bytes_to_hex(conf->account, 20, param_string + 3);
    param_string[43] = '"';
    param_string[44] = 0;
  }

  d_token_t* result;
  TRY(send_provider_request(ctx->ctx, conf, method + 7, param_string, &result))

  char* json = d_create_json(result);
  in3_rpc_handle_with_string(ctx, json);
  _free(json);
  return IN3_OK;
}

static in3_ret_t handle_zksync(void* cptr, in3_plugin_act_t action, void* arg) {
  zksync_config_t* conf = cptr;
  switch (action) {
    case PLGN_ACT_TERM: {
      if (conf->provider_url) _free(conf->provider_url);
      _free(conf);
      return IN3_OK;
    }

    case PLGN_ACT_CONFIG_GET: {
      in3_get_config_ctx_t* ctx = arg;
      sb_add_chars(ctx->sb, ",\"zksync\":{\"provider_url\":\"");
      sb_add_chars(ctx->sb, conf->provider_url ? conf->provider_url : "");
      sb_add_char(ctx->sb, '\"');
      if (conf->account) {
        bytes_t ac = bytes(conf->account, 20);
        sb_add_bytes(ctx->sb, ",\"account\"=", &ac, 1, false);
      }
      sb_add_char(ctx->sb, '}');
      return IN3_OK;
    }

    case PLGN_ACT_CONFIG_SET: {
      in3_configure_ctx_t* ctx = arg;
      if (ctx->token->key == key("zksync")) {
        char* provider = d_get_string(ctx->token, "provider_url");
        if (provider) conf->provider_url = _strdupn(provider, -1);
        bytes_t* account = d_get_bytes(ctx->token, "account");
        if (account && account->len == 20) memcpy(conf->account = _malloc(20), account->data, 20);
        return IN3_OK;
      }
      return IN3_EIGNORE;
    }

    case PLGN_ACT_RPC_HANDLE:
      return zksync_rpc(conf, arg);

    default:
      return IN3_ENOTSUP;
  }

  return IN3_EIGNORE;
}

in3_ret_t in3_register_zksync(in3_t* c) {
  return in3_plugin_register(c, PLGN_ACT_RPC_HANDLE | PLGN_ACT_TERM | PLGN_ACT_CONFIG_GET | PLGN_ACT_CONFIG_SET, handle_zksync, _calloc(sizeof(zksync_config_t), 1), false);
}