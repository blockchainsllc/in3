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

#include "../util/bitset.h"
#include "../util/data.h"
#include "../util/debug.h"
#include "../util/log.h"
#include "client.h"
#include "plugin.h"
#include "request_internal.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// set the defaults
typedef struct default_fn {
  plgn_register      fn;
  struct default_fn* next;
} default_fn_t;

static default_fn_t* default_registry = NULL;

// registers a default plugin, which means all registered reg_fn
// will be called whenever a new client is created
void in3_register_default(plgn_register reg_fn) {
  assert(reg_fn);

  // check if it already exists
  default_fn_t** d   = &default_registry;
  default_fn_t** pre = NULL;
  for (; *d; d = &(*d)->next) {
    if ((*d)->fn == reg_fn) pre = d;
  }

  // so the plugin is already registered,
  // but if registered again, we need to change the order.
  if (pre) {
    if ((*pre)->next) { // we are not the last one, so we need to make it the last
      default_fn_t* p = *pre;
      *pre            = p->next;
      *d              = p;
      p->next         = NULL;
    }
    return;
  }

  // not registered yet, so we create one and put it at the end
  (*d)     = _calloc(1, sizeof(default_fn_t));
  (*d)->fn = reg_fn;
}

static in3_ret_t in3_client_init(in3_t* c, chain_id_t chain_id) {
  assert(c);

  c->flags                 = FLAGS_STATS | FLAGS_AUTO_UPDATE_LIST | FLAGS_BOOT_WEIGHTS;
  c->cache_timeout         = 0;
  c->finality              = 0;
  c->max_attempts          = 7;
  c->max_verified_hashes   = 5;
  c->alloc_verified_hashes = 0;
  c->proof                 = PROOF_STANDARD;
  c->replace_latest_block  = 0;
  c->timeout               = 10000;
  c->id_count              = 1;

  if (chain_id == CHAIN_ID_MAINNET)
    in3_client_register_chain(c, 0x01, CHAIN_ETH, 2);
  else if (chain_id == CHAIN_ID_GOERLI)
    in3_client_register_chain(c, 0x05, CHAIN_ETH, 2);
  else if (chain_id == CHAIN_ID_IPFS)
    in3_client_register_chain(c, 0x7d0, CHAIN_IPFS, 2);
  else if (chain_id == CHAIN_ID_BTC)
    in3_client_register_chain(c, 0x99, CHAIN_BTC, 2);
  else if (chain_id == CHAIN_ID_EWC)
    in3_client_register_chain(c, 0xf6, CHAIN_ETH, 2);
  else if (chain_id == CHAIN_ID_LOCAL)
    in3_client_register_chain(c, 0x11, CHAIN_ETH, 1);

  return IN3_OK;
}

// init the chain with the given parameters
in3_ret_t in3_client_register_chain(in3_t* c, chain_id_t chain_id, in3_chain_type_t type, uint8_t version) {
  assert(c);

  in3_chain_t* chain = &c->chain;
  chain->chain_id    = chain_id;
  if (chain->verified_hashes) _free(chain->verified_hashes);
  chain->verified_hashes = NULL;
  chain->type            = type;
  chain->version         = version;
  return IN3_OK;
}

/* frees the data */
void in3_free(in3_t* a) {
  if (!a) return;

  // cleanup plugins
  for (in3_plugin_t* p = a->plugins; p;) {
    // we let the plugin free the resources, but don't care about the return-value.
    if (p->acts & PLGN_ACT_TERM) p->action_fn(p->data, PLGN_ACT_TERM, a);
    in3_plugin_t* n = p->next;
    _free(p);
    p = n;
  }

  if (a->chain.verified_hashes) _free(a->chain.verified_hashes);
  _free(a);
}

in3_t* in3_for_chain_default(chain_id_t chain_id) {

  // initialize random with the timestamp (in nanoseconds) as seed
  in3_srand(current_ms());

  // create new client
  in3_t* c = _calloc(1, sizeof(in3_t));
  if (in3_client_init(c, chain_id) != IN3_OK) {
    in3_free(c);
    return NULL;
  }

  // init from default plugins
  for (default_fn_t* d = default_registry; d; d = d->next) d->fn(c);

  return c;
}

static chain_id_t chain_id(d_token_t* t) {
  if (d_type(t) == T_STRING) {
    char* c = d_string(t);
    if (!strcmp(c, "mainnet")) return CHAIN_ID_MAINNET;
    if (!strcmp(c, "goerli")) return CHAIN_ID_GOERLI;
    if (!strcmp(c, "ewc")) return CHAIN_ID_EWC;
    if (!strcmp(c, "btc")) return CHAIN_ID_BTC;
    if (!strcmp(c, "ipfs")) return CHAIN_ID_IPFS;
    // 0 is allowed (as chain_id for local chain) if t is T_INT,
    // but for T_STRING it's an error
    return 0;
  }
  return d_long(t);
}

static int chain_type(d_token_t* t) {
  if (d_type(t) == T_STRING) {
    const char* c = d_string(t);
    if (!strcmp(c, "btc")) return CHAIN_BTC;
    if (!strcmp(c, "eth")) return CHAIN_ETH;
    if (!strcmp(c, "ipfs")) return CHAIN_IPFS;
  }
  else if (IS_D_UINT8(t))
    return d_int(t);
  return -1;
}

static in3_chain_type_t chain_type_from_id(chain_id_t id) {
  switch (id) {
    case CHAIN_ID_MAINNET:
    case CHAIN_ID_GOERLI:
    case CHAIN_ID_EWC:
    case CHAIN_ID_LOCAL:
      return CHAIN_ETH;
    case CHAIN_ID_IPFS:
      return CHAIN_IPFS;
    case CHAIN_ID_BTC:
      return CHAIN_BTC;
    default:
      return CHAIN_GENERIC;
  }
}

char* in3_get_config(in3_t* c) {
  sb_t* sb = sb_new("");
  add_bool(sb, '{', "autoUpdateList", c->flags & FLAGS_AUTO_UPDATE_LIST);
  add_uint(sb, ',', "chainId", c->chain.chain_id);
  add_uint(sb, ',', "signatureCount", c->signature_count);
  add_uint(sb, ',', "finality", c->finality);
  add_bool(sb, ',', "includeCode", c->flags & FLAGS_INCLUDE_CODE);
  add_bool(sb, ',', "bootWeights", c->flags & FLAGS_BOOT_WEIGHTS);
  add_uint(sb, ',', "maxAttempts", c->max_attempts);
  add_bool(sb, ',', "keepIn3", c->flags & FLAGS_KEEP_IN3);
  add_bool(sb, ',', "stats", c->flags & FLAGS_STATS);
  add_bool(sb, ',', "useBinary", c->flags & FLAGS_BINARY);
  add_bool(sb, ',', "useHttp", c->flags & FLAGS_HTTP);
  add_bool(sb, ',', "experimental", c->flags & FLAGS_ALLOW_EXPERIMENTAL);
  add_uint(sb, ',', "maxVerifiedHashes", c->max_verified_hashes);
  add_uint(sb, ',', "timeout", c->timeout);
  add_string(sb, ',', "proof", (c->proof == PROOF_NONE) ? "none" : (c->proof == PROOF_STANDARD ? "standard" : "full"));
  if (c->replace_latest_block)
    add_uint(sb, ',', "replaceLatestBlock", c->replace_latest_block);

  in3_get_config_ctx_t cctx = {.client = c, .sb = sb};
  in3_plugin_execute_all(c, PLGN_ACT_CONFIG_GET, &cctx);
  sb_add_chars(sb, "}");

  char* r = sb->data;
  _free(sb);
  return r;
}

char* in3_configure(in3_t* c, const char* config) {
  // config can not be changed as long as there are pending requests.
  if (c->pending) return config_err("in3_configure", "can not change config because there are pending requests!");

  // make sure the json-config is parseable.
  json_ctx_t* json = parse_json((char*) config);
  if (!json || !json->result) return config_err("in3_configure", "parse error");

  // the error-message we will return in case of an error.
  char* res = NULL;

  // we iterate over the root-props
  for (d_iterator_t iter = d_iter(json->result); iter.left; d_iter_next(&iter)) {
    d_token_t* token = iter.token;

    if (token->key == key("autoUpdateList")) {
      EXPECT_TOK_BOOL(token);
      BITMASK_SET_BOOL(c->flags, FLAGS_AUTO_UPDATE_LIST, (d_int(token) ? true : false));
    }
    else if (token->key == key("chainType"))
      ; // Ignore - handled within `chainId` case
    else if (token->key == key("chainId")) {
      EXPECT_TOK(token, IS_D_UINT32(token) || (d_type(token) == T_STRING && chain_id(token) != 0), "expected uint32 or string value (mainnet/goerli)");

      // check if chainType is set
      int        ct_      = -1;
      d_token_t* ct_token = d_get(json->result, key("chainType"));
      if (ct_token) {
        ct_ = chain_type(ct_token);
        EXPECT_TOK(ct_token, ct_ != -1, "expected (btc|eth|ipfs|<u8-value>)");
      }
      else
        ct_ = chain_type_from_id(c->chain.chain_id);

      bool changed      = (c->chain.chain_id != chain_id(token));
      c->chain.chain_id = chain_id(token);
      c->chain.type     = (uint8_t) ct_;
      in3_client_register_chain(c, c->chain.chain_id, c->chain.type, 2);
      if (changed) in3_plugin_execute_all(c, PLGN_ACT_CHAIN_CHANGE, c);
    }
    else if (token->key == key("signatureCount")) {
      EXPECT_TOK_U8(token);
      c->signature_count = (uint8_t) d_int(token);
    }
    else if (token->key == key("finality")) {
      EXPECT_TOK_U16(token);
#ifdef POA
      if (c->chain.chain_id == CHAIN_ID_GOERLI)
        EXPECT_CFG(d_int(token) > 0 && d_int(token) <= 100, "expected % value");
#endif
      c->finality = (uint16_t) d_int(token);
    }
    else if (token->key == key("includeCode")) {
      EXPECT_TOK_BOOL(token);
      BITMASK_SET_BOOL(c->flags, FLAGS_INCLUDE_CODE, (d_int(token) ? true : false));
    }
    else if (token->key == key("bootWeights")) {
      EXPECT_TOK_BOOL(token);
      BITMASK_SET_BOOL(c->flags, FLAGS_BOOT_WEIGHTS, (d_int(token) ? true : false));
    }
    else if (token->key == key("maxAttempts")) {
      EXPECT_TOK_U16(token);
      EXPECT_CFG(d_int(token), "maxAttempts must be at least 1");
      c->max_attempts = d_int(token);
    }
    else if (token->key == key("keepIn3")) {
      EXPECT_TOK_BOOL(token);
      BITMASK_SET_BOOL(c->flags, FLAGS_KEEP_IN3, (d_int(token) ? true : false));
    }
    else if (token->key == key("debug")) {
      if (d_int(token)) {
        in3_log_set_level(LOG_TRACE);
        in3_log_set_quiet(false);
      }
      else
        in3_log_set_quiet(true);
    }
    else if (token->key == key("stats")) {
      EXPECT_TOK_BOOL(token);
      BITMASK_SET_BOOL(c->flags, FLAGS_STATS, (d_int(token) ? true : false));
    }
    else if (token->key == key("useBinary")) {
      EXPECT_TOK_BOOL(token);
      BITMASK_SET_BOOL(c->flags, FLAGS_BINARY, (d_int(token) ? true : false));
    }
    else if (token->key == key("experimental")) {
      EXPECT_TOK_BOOL(token);
      BITMASK_SET_BOOL(c->flags, FLAGS_ALLOW_EXPERIMENTAL, (d_int(token) ? true : false));
    }
    else if (token->key == key("useHttp")) {
      EXPECT_TOK_BOOL(token);
      BITMASK_SET_BOOL(c->flags, FLAGS_HTTP, (d_int(token) ? true : false));
    }
    else if (token->key == key("maxVerifiedHashes")) {
      EXPECT_TOK_U16(token);
      if (c->max_verified_hashes < d_long(token)) {
        c->chain.verified_hashes = _realloc(c->chain.verified_hashes,
                                            sizeof(in3_verified_hash_t) * d_long(token),
                                            sizeof(in3_verified_hash_t) * c->max_verified_hashes);
        // clear newly allocated memory
        memset(c->chain.verified_hashes + c->max_verified_hashes, 0, (d_long(token) - c->max_verified_hashes) * sizeof(in3_verified_hash_t));
      }
      c->max_verified_hashes   = d_long(token);
      c->alloc_verified_hashes = c->max_verified_hashes;
    }
    else if (token->key == key("timeout")) {
      EXPECT_TOK_U32(token);
      c->timeout = d_long(token);
    }
    else if (token->key == key("proof")) {
      EXPECT_TOK_STR(token);
      EXPECT_TOK(token, !strcmp(d_string(token), "full") || !strcmp(d_string(token), "standard") || !strcmp(d_string(token), "none"), "expected values - full/standard/none");
      c->proof = strcmp(d_string(token), "full") == 0
                     ? PROOF_FULL
                     : (strcmp(d_string(token), "standard") == 0 ? PROOF_STANDARD : PROOF_NONE);
    }
    else if (token->key == key("verifiedHashes")) {
      EXPECT_TOK_ARR(token);
      EXPECT_TOK(token, (unsigned) d_len(token) <= c->max_verified_hashes, "expected array len <= maxVerifiedHashes");
      if (!c->chain.verified_hashes)
        c->chain.verified_hashes = _calloc(c->max_verified_hashes, sizeof(in3_verified_hash_t));
      else
        // clear extra verified_hashes (preceding ones will be overwritten anyway)
        memset(c->chain.verified_hashes + d_len(token), 0, (c->max_verified_hashes - d_len(token)) * sizeof(in3_verified_hash_t));
      int i = 0;
      for (d_iterator_t n = d_iter(token); n.left; d_iter_next(&n), i++) {
        EXPECT_TOK_U64(d_get(n.token, key("block")));
        EXPECT_TOK_B256(d_get(n.token, key("hash")));
        c->chain.verified_hashes[i].block_number = d_get_long(n.token, key("block"));
        memcpy(c->chain.verified_hashes[i].hash, d_get_byteskl(n.token, key("hash"), 32)->data, 32);
      }
      c->alloc_verified_hashes = c->max_verified_hashes;
    }
    else {
      // since the token was not handled yet, we will ask the plugins..
      in3_configure_ctx_t cctx    = {.client = c, .json = json, .token = token, .error_msg = NULL};
      bool                handled = false;
      for (in3_plugin_t* p = c->plugins; p; p = p->next) {
        if (p->acts & PLGN_ACT_CONFIG_SET) {
          in3_ret_t r = p->action_fn(p->data, PLGN_ACT_CONFIG_SET, &cctx);
          if (r == IN3_EIGNORE)
            continue;
          else if (r != IN3_OK) {
            res = cctx.error_msg ? cctx.error_msg : _strdupn("error configuring this option!", -1);
            goto cleanup;
          }
          handled = true;
          break;
        }
      }

      if (!handled) EXPECT_TOK(token, false, "unsupported config option!");
    }
  }

  if (c->signature_count && c->chain.chain_id != CHAIN_ID_LOCAL && !c->replace_latest_block) {
    in3_log_warn("signatureCount > 0 without replaceLatestBlock is bound to fail; using default (" STR(DEF_REPL_LATEST_BLK) ")\n");
    c->replace_latest_block = DEF_REPL_LATEST_BLK;
  }

  EXPECT_CFG(c->chain.chain_id, "chain corresponding to chain id not initialized!");
  assert_in3(c);

cleanup:
  json_free(json);
  return res;
}

in3_ret_t in3_plugin_register(in3_t* c, in3_plugin_supp_acts_t acts, in3_plugin_act_fn action_fn, void* data, bool replace_ex) {
  if (!acts || !action_fn)
    return IN3_EINVAL;

  in3_plugin_t** p = &c->plugins;
  while (*p) {
    // check for action-specific rules here like allowing only one action handler per action, etc.
    if (replace_ex && (*p)->acts == acts) {
      if ((*p)->acts & PLGN_ACT_TERM) (*p)->action_fn((*p)->data, PLGN_ACT_TERM, c);
      (*p)->action_fn = action_fn;
      (*p)->data      = data;
      return IN3_OK;
    }

    // we don't allow 2 plugins with the same fn with no extra data
    // TODO maybe we can have a rule based filter instead of onlly repllace_ex
    if ((*p)->action_fn == action_fn && data == NULL && (*p)->data == NULL) return IN3_OK;

    p = &(*p)->next;
  }

  assert(p != NULL);
  in3_plugin_t* new_p = _malloc(sizeof(in3_plugin_t));
  *p                  = new_p;
  new_p->acts         = acts;
  new_p->action_fn    = action_fn;
  new_p->data         = data;
  new_p->next         = NULL;

  // didn't find any existing, so we add a new ...
  c->plugin_acts |= acts;
  return IN3_OK;
}

in3_ret_t in3_plugin_execute_all(in3_t* c, in3_plugin_act_t action, void* plugin_ctx) {
  if (!in3_plugin_is_registered(c, action)) return IN3_OK;

  in3_plugin_t* p   = c->plugins;
  in3_ret_t     ret = IN3_OK, ret_;
  while (p) {
    if (p->acts & action) {
      ret_ = p->action_fn(p->data, action, plugin_ctx);
      if (ret == IN3_OK && ret_ != IN3_OK)
        ret = ret_; // only record first err
    }
    p = p->next;
  }
  return ret;
}

#ifdef LOGGING
static char* action_name(in3_plugin_act_t action) {
  switch (action) {
    case PLGN_ACT_INIT: return "init";
    case PLGN_ACT_TERM: return "terrm";
    case PLGN_ACT_TRANSPORT_SEND: return "transport_send";
    case PLGN_ACT_TRANSPORT_RECEIVE: return "transport_receive";
    case PLGN_ACT_TRANSPORT_CLEAN: return "transport_clean";
    case PLGN_ACT_SIGN_ACCOUNT: return "sign_account";
    case PLGN_ACT_SIGN_PREPARE: return "sign_prepare";
    case PLGN_ACT_SIGN: return "sign";
    case PLGN_ACT_RPC_HANDLE: return "rpc_handle";
    case PLGN_ACT_RPC_VERIFY: return "rpc_verify";
    case PLGN_ACT_CACHE_SET: return "cache_set";
    case PLGN_ACT_CACHE_GET: return "cache_get";
    case PLGN_ACT_CACHE_CLEAR: return "cache_clear";
    case PLGN_ACT_CONFIG_SET: return "config_set";
    case PLGN_ACT_CONFIG_GET: return "config_get";
    case PLGN_ACT_PAY_PREPARE: return "pay_prepare";
    case PLGN_ACT_PAY_FOLLOWUP: return "pay_followup";
    case PLGN_ACT_PAY_HANDLE: return "pay_handle";
    case PLGN_ACT_PAY_SIGN_REQ: return "pay_sign_req";
    case PLGN_ACT_LOG_ERROR: return "log_error";
    case PLGN_ACT_NL_PICK: return "nl_pick";
    case PLGN_ACT_NL_PICK_FOLLOWUP: return "nl_pick_followup";
    case PLGN_ACT_NL_BLACKLIST: return "nl_blacklist";
    case PLGN_ACT_NL_FAILABLE: return "nl_failable";
    case PLGN_ACT_NL_OFFLINE: return "nl_offline";
    case PLGN_ACT_CHAIN_CHANGE: return "chain_change";
    case PLGN_ACT_GET_DATA: return "get_data";
    case PLGN_ACT_ADD_PAYLOAD: return "add_payload";
    default:
      assert("unknown plugin");
      return "unknown";
  }
}
#endif

in3_ret_t in3_plugin_execute_first(in3_req_t* ctx, in3_plugin_act_t action, void* plugin_ctx) {
  assert(ctx);
  for (in3_plugin_t* p = ctx->client->plugins; p; p = p->next) {
    if (p->acts & action) {
      in3_ret_t ret = p->action_fn(p->data, action, plugin_ctx);
      if (ret != IN3_EIGNORE) return ret;
    }
  }
#ifdef LOGGING
  char* name = action_name(action);
  char* msg  = alloca(strlen(name) + 60);
  sprintf(msg, "no plugin found that handled the %s action", name);
#else
  char* msg = "E";
#endif
  return req_set_error(ctx, msg, IN3_EPLGN_NONE);
}

in3_ret_t in3_plugin_execute_first_or_none(in3_req_t* ctx, in3_plugin_act_t action, void* plugin_ctx) {
  assert(ctx);
  if (!in3_plugin_is_registered(ctx->client, action))
    return IN3_OK;

  for (in3_plugin_t* p = ctx->client->plugins; p; p = p->next) {
    if (p->acts & action) {
      in3_ret_t ret = p->action_fn(p->data, action, plugin_ctx);
      if (ret != IN3_EIGNORE) return ret;
    }
  }

  return IN3_OK;
}
