/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
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
#include "../../core/client/keys.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/debug.h"
#include "../../third-party/zkcrypto/lib.h"
#include "zk_helper.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static zk_sign_type_t get_sign_type(d_token_t* type) {
  if (type == NULL) return ZK_SIGN_PK;
  char* c = d_string(type);
  if (strcmp(c, "contract") == 0) return ZK_SIGN_CONTRACT;
  if (strcmp(c, "create2") == 0) return ZK_SIGN_CREATE2;
  return ZK_SIGN_PK;
}

static in3_ret_t zksync_get_key(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  bytes32_t k;
  TRY(zksync_get_sync_key(conf, ctx->req, k))
  return in3_rpc_handle_with_bytes(ctx, bytes(k, 32));
}

static in3_ret_t zksync_get_pubkeyhash(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  address_t pubkey_hash;
  if (d_len(ctx->params) == 1) {
    CHECK_PARAM_TYPE(ctx->req, ctx->params, 0, T_BYTES)
    CHECK_PARAM_LEN(ctx->req, ctx->params, 0, 32)
    TRY(zkcrypto_pubkey_hash(d_get_bytes_at(ctx->params, 0), pubkey_hash));
  }
  else
    TRY(zksync_get_pubkey_hash(conf, ctx->req, pubkey_hash))
  char res[48];
  strcpy(res, "\"sync:");
  bytes_to_hex(pubkey_hash, 20, res + 6);
  strcpy(res + 46, "\"");
  return in3_rpc_handle_with_string(ctx, res);
}

static in3_ret_t zksync_get_pubkey(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  bytes32_t pubkey;
  if (conf->musig_pub_keys.data)
    TRY(zkcrypto_compute_aggregated_pubkey(conf->musig_pub_keys, pubkey))
  else if (!memiszero(conf->pub_key, 32))
    memcpy(pubkey, conf->pub_key, 32);
  else {
    bytes32_t k;
    TRY(zksync_get_sync_key(conf, ctx->req, k))
    TRY(zkcrypto_pk_to_pubkey(k, pubkey))
    memcpy(conf->pub_key, pubkey, 32);
  }
  return in3_rpc_handle_with_bytes(ctx, bytes(pubkey, 32));
}

static in3_ret_t zksync_aggregate_pubkey(in3_rpc_handle_ctx_t* ctx) {
  bytes32_t dst;
  CHECK_PARAM_TYPE(ctx->req, ctx->params, 0, T_BYTES)
  CHECK_PARAM(ctx->req, ctx->params, 0, d_len(val) % 32 == 0)

  TRY(zkcrypto_compute_aggregated_pubkey(d_get_bytes_at(ctx->params, 0), dst))
  return in3_rpc_handle_with_bytes(ctx, bytes(dst, 32));
}

static in3_ret_t zksync_account_address(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  uint8_t* account = NULL;
  TRY(zksync_get_account(conf, ctx->req, &account));
  return in3_rpc_handle_with_bytes(ctx, bytes(account, 20));
}

static in3_ret_t zksync_contract_address(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  uint8_t* adr;
  TRY(zksync_get_contracts(conf, ctx->req, &adr))
  sb_t* sb = in3_rpc_handle_start(ctx);
  sb_add_rawbytes(sb, "{\"govContract\":\"0x", bytes(conf->gov_contract, 20), 0);
  sb_add_rawbytes(sb, "\",\"mainContract\":\"0x", bytes(conf->main_contract, 20), 0);
  sb_add_chars(sb, "\"}");
  return in3_rpc_handle_finish(ctx);
}
static in3_ret_t zksync_tokens(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  TRY(resolve_tokens(conf, ctx->req, NULL, NULL))
  sb_t* sb = in3_rpc_handle_start(ctx);
  sb_add_char(sb, '{');
  for (unsigned int i = 0; i < conf->token_len; i++) {
    if (i) sb_add_char(sb, ',');
    sb_add_char(sb, '\"');
    sb_add_chars(sb, conf->tokens[i].symbol);
    sb_add_rawbytes(sb, "\":{\"address\":\"0x", bytes(conf->tokens[i].address, 20), 0);
    sb_add_chars(sb, "\",\"decimals\":");
    sb_add_int(sb, conf->tokens[i].decimals);
    sb_add_chars(sb, ",\"id\":");
    sb_add_int(sb, conf->tokens[i].id);
    sb_add_chars(sb, ",\"symbol\":\"");
    sb_add_chars(sb, conf->tokens[i].symbol);
    sb_add_chars(sb, "\"}");
  }
  sb_add_char(sb, '}');
  return in3_rpc_handle_finish(ctx);
}

// --- handle rpc----
static in3_ret_t zksync_rpc(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  // check the prefix (zksync_ or zk_ is supported)
  if (strncmp(ctx->method, "zksync_", 7) == 0)
    ctx->method += 7;
  else if (strncmp(ctx->method, "zk_", 3) == 0 && strncmp(ctx->method, "zk_wallet_", 10))
    ctx->method += 3;
  else
    return IN3_EIGNORE;

  // mark zksync as experimental
  REQUIRE_EXPERIMENTAL(ctx->req, "zksync")

  // use custom conf if set
  const cache_entry_t* cached = in3_cache_get_entry_by_prop(ctx->req->cache, ZKSYNC_CACHED_CONFIG);
  if (cached) conf = (void*) cached->value.data;

    // handle rpc -functions
#if !defined(RPC_ONLY) || defined(RPC_DEPOSIT)
  TRY_RPC("deposit", zksync_deposit(conf, ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_TRANSFER)
  TRY_RPC("transfer", zksync_transfer(conf, ctx, ZK_TRANSFER))
#endif
#if !defined(RPC_ONLY) || defined(RPC_WITHDRAW)
  TRY_RPC("withdraw", zksync_transfer(conf, ctx, ZK_WITHDRAW))
#endif
#if !defined(RPC_ONLY) || defined(RPC_SET_KEY)
  TRY_RPC("set_key", zksync_set_key(conf, ctx, false))
#endif
#if !defined(RPC_ONLY) || defined(RPC_EMERGENCY_WITHDRAW)
  TRY_RPC("emergency_withdraw", zksync_emergency_withdraw(conf, ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_SYNC_KEY)
  TRY_RPC("sync_key", zksync_get_key(conf, ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_AGGREGATE_PUBKEY)
  TRY_RPC("aggregate_pubkey", zksync_aggregate_pubkey(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_PUBKEYHASH)
  TRY_RPC("pubkeyhash", zksync_get_pubkeyhash(conf, ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_PUBKEY)
  TRY_RPC("pubkey", zksync_get_pubkey(conf, ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_ACCOUNT_ADDRESS)
  TRY_RPC("account_address", zksync_account_address(conf, ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_CONTRACT_ADDRESS)
  TRY_RPC("contract_address", zksync_contract_address(conf, ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_TOKENS)
  TRY_RPC("tokens", zksync_tokens(conf, ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_SIGN)
  TRY_RPC("sign", zksync_musig_sign(conf, ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_VERIFY)
  TRY_RPC("verify", in3_rpc_handle_with_int(ctx, conf->musig_pub_keys.data
                                                     ? zkcrypto_verify_signatures(d_get_bytes_at(ctx->params, 0), conf->musig_pub_keys, d_get_bytes_at(ctx->params, 1))
                                                     : zkcrypto_verify_musig(d_get_bytes_at(ctx->params, 0), d_get_bytes_at(ctx->params, 1))))
#endif
#if !defined(RPC_ONLY) || defined(RPC_TX_DATA)
  TRY_RPC("tx_data", zksync_tx_data(conf, ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_ACCOUNT_HISTORY)
  TRY_RPC("account_history", zksync_account_history(conf, ctx))
#endif

  // prepare fallback to send to zksync-server
  str_range_t p            = d_to_json(ctx->params);
  char*       param_string = alloca(p.len - 1);
  memcpy(param_string, p.data + 1, p.len - 2);
  param_string[p.len - 2] = 0;

#if !defined(RPC_ONLY) || defined(RPC_ZKSYNC_ACCOUNT_INFO)
  if (strcmp(ctx->method, "account_info") == 0) {
    if (*param_string == 0 || strcmp(param_string, "null") == 0) {
      TRY(zksync_get_account(conf, ctx->req, NULL))
      param_string = alloca(45);
      set_quoted_address(param_string, conf->account);
    }
    else
      CHECK_PARAM_ADDRESS(ctx->req, ctx->params, 0)
  }
#endif

  // we need to show the arguments as integers
  if (strcmp(ctx->method, "ethop_info") == 0)
    sprintf(param_string, "%i", d_get_int_at(ctx->params, 0));

  // send request to the server
  d_token_t* result;
  TRY(send_provider_request(ctx->req, conf, ctx->method, param_string, &result))

  // format result
  char* json = d_create_json(NULL, result);

  if (strcmp(ctx->method, "get_token_price") == 0 && strchr(json, '.') && d_type(result) == T_STRING) {
    // remove pending zeros
    for (char* p = json + strlen(json) - 1; *p && p > json; p--) {
      if (*p == '"') continue;
      if (*p == '0' && p > json) {
        if (p[-1] == '.') {
          in3_rpc_handle_with_string(ctx, json + 1);
          _free(json);
          return IN3_OK;
        };
        *p = 0;
      }
      else {
        in3_rpc_handle_with_string(ctx, json + 1);
        _free(json);
        return IN3_OK;
      }
    }
  }
  in3_rpc_handle_with_string(ctx, json);
  _free(json);
  return IN3_OK;
}

static in3_ret_t config_free(zksync_config_t* conf, bool free_conf) {
  if (conf->musig_urls) {
    for (unsigned int i = 0; i < conf->musig_len; i++) {
      if (conf->musig_urls[i]) _free(conf->musig_urls[i]);
    }
    _free(conf->musig_urls);
  }
  if (conf->rest_api) _free(conf->rest_api);
  if (conf->provider_url) _free(conf->provider_url);
  if (conf->main_contract) _free(conf->main_contract);
  if (conf->gov_contract) _free(conf->gov_contract);
  if (conf->account) _free(conf->account);
  if (conf->tokens) _free(conf->tokens);
  if (conf->proof_verify_method) _free(conf->proof_verify_method);
  if (conf->proof_create_method) _free(conf->proof_create_method);
  if (conf->musig_pub_keys.data) _free(conf->musig_pub_keys.data);
  if (conf->incentive && conf->incentive->token) _free(conf->incentive->token);
  if (conf->incentive) {
    config_free(&conf->incentive->config, false);
    _free(conf->incentive);
  }

  while (conf->musig_sessions) conf->musig_sessions = zk_musig_session_free(conf->musig_sessions);
  if (free_conf) _free(conf);
  return IN3_OK;
}

static in3_ret_t config_get(zksync_config_t* conf, in3_get_config_ctx_t* ctx) {
  sb_add_chars(ctx->sb, ",\"zksync\":{\"provider_url\":\"");
  sb_add_chars(ctx->sb, conf->provider_url ? conf->provider_url : "");
  sb_add_char(ctx->sb, '\"');
  if (conf->account) {
    bytes_t ac = bytes(conf->account, 20);
    sb_add_bytes(ctx->sb, ",\"account\"=", &ac, 1, false);
  }
  sb_add_chars(ctx->sb, ",\"signer_type\":\"");
  if (conf->sign_type == ZK_SIGN_CONTRACT)
    sb_add_chars(ctx->sb, "contract\"");
  else if (conf->sign_type == ZK_SIGN_CREATE2)
    sb_add_chars(ctx->sb, "create2\"");
  else
    sb_add_chars(ctx->sb, "pk\"");
  sb_add_char(ctx->sb, '}');
  return IN3_OK;
}

static in3_ret_t config_set(zksync_config_t* conf, in3_configure_ctx_t* ctx) {
  if (!d_is_key(ctx->token, key("zksync"))) return IN3_EIGNORE;
  // TODO error-reporting for invalid config

  const char* provider = d_get_string(ctx->token, CONFIG_KEY("provider_url"));
  if (provider) {
    if (conf->provider_url) _free(conf->provider_url);
    conf->provider_url = _strdupn(provider, -1);
  }
  const char* rest_api = d_get_string(ctx->token, CONFIG_KEY("rest_api"));
  if (rest_api) {
    if (conf->rest_api) _free(conf->rest_api);
    conf->rest_api = _strdupn(rest_api, -1);
  }
  const char* pvm = d_get_string(ctx->token, CONFIG_KEY("verify_proof_method"));
  if (pvm) {
    if (conf->proof_verify_method) _free(conf->proof_verify_method);
    conf->proof_verify_method = _strdupn(pvm, -1);
  }
  const char* pcm = d_get_string(ctx->token, CONFIG_KEY("create_proof_method"));
  if (pcm) {
    if (conf->proof_create_method) _free(conf->proof_create_method);
    conf->proof_create_method = _strdupn(pcm, -1);
  }
  bytes_t account = d_get_bytes(ctx->token, CONFIG_KEY("account"));
  if (account.data && account.len == 20) memcpy(conf->account = _malloc(20), account.data, 20);
  bytes_t sync_key = d_bytes(d_get(ctx->token, CONFIG_KEY("sync_key")));
  if (sync_key.len) {
    zkcrypto_pk_from_seed(sync_key, conf->sync_key);
    zkcrypto_pk_to_pubkey(conf->sync_key, conf->pub_key);
    zkcrypto_pubkey_hash(bytes(conf->pub_key, 32), conf->pub_key_hash_pk);
  }

  bytes_t main_contract = d_get_bytes(ctx->token, CONFIG_KEY("main_contract"));
  if (main_contract.data && main_contract.len == 20) memcpy(conf->main_contract = _malloc(20), main_contract.data, 20);
  d_token_t* st = d_get(ctx->token, CONFIG_KEY("signer_type"));
  if (st)
    conf->sign_type = get_sign_type(st);
  else if (conf->sign_type == 0)
    conf->sign_type = ZK_SIGN_PK;
  conf->version = (uint32_t) d_intd(d_get(ctx->token, CONFIG_KEY("version")), conf->version);
  bytes_t musig = d_get_bytes(ctx->token, CONFIG_KEY("musig_pub_keys"));
  if (musig.data && musig.len % 32 == 0) {
    if (conf->musig_pub_keys.data) _free(conf->musig_pub_keys.data);
    conf->musig_pub_keys = bytes(_malloc(musig.len), musig.len);
    memcpy(conf->musig_pub_keys.data, musig.data, musig.len);
  }
  d_token_t* urls = d_get(ctx->token, CONFIG_KEY("musig_urls"));
  if (urls) {
    if (conf->musig_urls) {
      for (unsigned int i = 0; i < conf->musig_len; i++) {
        if (conf->musig_urls[i]) _free(conf->musig_urls[i]);
      }
      _free(conf->musig_urls);
    }
    if (d_type(urls) == T_STRING) {
      conf->musig_urls    = _calloc(2, sizeof(char*));
      conf->musig_urls[1] = _strdupn(d_string(urls), -1);
      conf->musig_len     = 2;
    }
    else if (d_type(urls) == T_ARRAY) {
      conf->musig_len  = (uint_fast8_t) d_len(urls);
      conf->musig_urls = _calloc(conf->musig_len, sizeof(char*));
      for (int i = 0; i < conf->musig_len; i++) {
        char* s = d_get_string_at(urls, i);
        if (s && strlen(s)) conf->musig_urls[i] = _strdupn(s, -1);
      }
    }
  }
  d_token_t* create2 = d_get(ctx->token, CONFIG_KEY("create2"));
  if (create2) {
    conf->sign_type = ZK_SIGN_CREATE2;
    bytes_t t       = d_get_bytes(create2, CONFIG_KEY("creator"));
    if (t.data && t.len == 20) memcpy(conf->create2.creator, t.data, 20);
    t = d_get_bytes(create2, CONFIG_KEY("saltarg"));
    if (t.data && t.len == 32) memcpy(conf->create2.salt_arg, t.data, 32);
    t = d_get_bytes(create2, CONFIG_KEY("codehash"));
    if (t.data && t.len == 32) memcpy(conf->create2.codehash, t.data, 32);
  }

  d_token_t* incentive = d_get(ctx->token, CONFIG_KEY("incentive"));
  if (incentive) {
    if (!conf->incentive) conf->incentive = _calloc(1, sizeof(pay_criteria_t));
    for (d_iterator_t iter = d_iter(incentive); iter.left; d_iter_next(&iter)) {
      if (d_is_key(iter.token, CONFIG_KEY("nodes"))) {
        conf->incentive->payed_nodes = d_int(iter.token);
        in3_req_t c                  = {0};
        c.client                     = ctx->client;
        in3_ret_t ret                = update_nodelist_from_cache(&c, conf->incentive->payed_nodes);
        if (c.error) {
          ctx->error_msg = c.error;
          return ret;
        }
      }
      else if (d_is_key(iter.token, CONFIG_KEY("max_price")))
        conf->incentive->max_price_per_hundred_igas = d_long(iter.token);
      else if (d_is_key(iter.token, CONFIG_KEY("token"))) {
        _free(conf->incentive->token);
        conf->incentive->token = _strdupn(d_string(iter.token), -1);
      }
    }
  }
  return IN3_OK;
}

static in3_ret_t zksync_init(zksync_config_t* conf, in3_req_t* ctx) {
  if (ctx->client->plugin_acts & PLGN_ACT_PAY_SIGN_REQ) {
    if (!conf->incentive) {
      conf->incentive              = _calloc(1, sizeof(pay_criteria_t));
      conf->incentive->payed_nodes = 1;
    }
    return update_nodelist_from_cache(ctx, conf->incentive->payed_nodes);
  }
  return IN3_OK;
}

in3_ret_t handle_zksync(void* conf, in3_plugin_act_t action, void* arg) {
  switch (action) {
    case PLGN_ACT_TERM: return config_free(conf, true);
    case PLGN_ACT_CONFIG_GET: return config_get(conf, arg);
    case PLGN_ACT_CONFIG_SET: return config_set(conf, arg);
    case PLGN_ACT_RPC_HANDLE: return zksync_rpc(conf, arg);
    case PLGN_ACT_ADD_PAYLOAD: return zksync_add_payload(arg);
    case PLGN_ACT_PAY_FOLLOWUP: return zksync_check_payment(conf, arg);
    case PLGN_ACT_INIT: return zksync_init(conf, arg);
    default: return IN3_ENOTSUP;
  }
  return IN3_EIGNORE;
}

zksync_config_t* zksync_get_conf(in3_req_t* req) {
  for (in3_plugin_t* p = req->client->plugins; p; p = p->next) {
    if (p->action_fn == handle_zksync) return p->data;
  }
  return NULL;
}

in3_ret_t in3_register_zksync(in3_t* c) {
  zksync_config_t* conf = _calloc(sizeof(zksync_config_t), 1);
  conf->version         = 1;
  conf->sign_type       = ZK_SIGN_PK;
  return in3_plugin_register(c,
                             PLGN_ACT_RPC_HANDLE | PLGN_ACT_INIT | PLGN_ACT_TERM | PLGN_ACT_CONFIG_GET | PLGN_ACT_CONFIG_SET | PLGN_ACT_ADD_PAYLOAD | PLGN_ACT_PAY_FOLLOWUP,
                             handle_zksync, conf, false);
}
