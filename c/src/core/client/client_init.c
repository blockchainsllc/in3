/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
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
#include "../util/mem.h"
#include "client.h"
#include "context_internal.h"
#include "plugin.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#ifndef __ZEPHYR__
#include <sys/time.h>
#endif
#include <time.h>

#ifdef PAY
typedef struct payment {
  d_key_t         name;
  pay_configure   configure;
  struct payment* next;

} pay_configure_t;

static pay_configure_t* payments = NULL;

static pay_configure_t* find_payment(char* name) {
  d_key_t k = key(name);
  for (pay_configure_t* p = payments; p; p = p->next) {
    if (k == p->name) return p;
  }
  return NULL;
}

void in3_register_payment(
    char*         name,   /**< name of the payment-type */
    pay_configure handler /**< pointer to the handler- */
) {
  if (find_payment(name)) return;
  pay_configure_t* p = _malloc(sizeof(pay_configure_t));
  p->configure       = handler;
  p->name            = key(name);
  p->next            = payments;
  payments           = p;
}

#endif

// set the defaults
typedef struct default_fn {
  plgn_register      fn;
  struct default_fn* next;
} default_fn_t;

static default_fn_t* default_registry = NULL;

static in3_transport_legacy default_legacy_transport = NULL;
static in3_ret_t            handle_legacy_transport(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  UNUSED_VAR(plugin_data);
  UNUSED_VAR(action);
  assert(plugin_ctx);
  return default_legacy_transport((in3_request_t*) plugin_ctx);
}
static in3_ret_t register_legacy(in3_t* c) {
  assert(c);
  return plugin_register(c, PLGN_ACT_TRANSPORT, handle_legacy_transport, NULL, true);
}

void in3_set_default_legacy_transport(
    in3_transport_legacy transport /**< the default transport-function. */
) {
  default_legacy_transport = transport;
  in3_register_default(register_legacy);
}
void in3_register_default(plgn_register reg_fn) {
  assert(reg_fn);
  // check if it already exists
  default_fn_t** d   = &default_registry;
  default_fn_t** pre = NULL;
  for (; *d; d = &(*d)->next) {
    if ((*d)->fn == reg_fn) pre = d;
  }
  if (pre) {
    if ((*pre)->next) { // we are not the last one, so we need to make it the last
      default_fn_t* p = *pre;
      *pre            = p->next;
      *d              = p;
      p->next         = NULL;
    }
    return;
  }

  (*d)     = _calloc(1, sizeof(default_fn_t));
  (*d)->fn = reg_fn;
}

static void init_ipfs(in3_t* c) {
  in3_client_register_chain(c, 0x7d0, CHAIN_IPFS, 2);
}

static void init_mainnet(in3_t* c) {
  in3_client_register_chain(c, 0x01, CHAIN_ETH, 2);
}
static void init_ewf(in3_t* c) {
  in3_client_register_chain(c, 0xf6, CHAIN_ETH, 2);
}

static void init_btc(in3_t* c) {
  in3_client_register_chain(c, 0x99, CHAIN_BTC, 2);
}

static void init_goerli(in3_t* c) {
#ifdef IN3_STAGING
  // goerli
  in3_client_register_chain(c, 0x05, CHAIN_ETH, 2);
#else
  // goerli
  in3_client_register_chain(c, 0x05, CHAIN_ETH, 2);
#endif
}

static in3_ret_t in3_client_init(in3_t* c, chain_id_t chain_id) {
  assert(c);

  c->flags                 = FLAGS_STATS | FLAGS_AUTO_UPDATE_LIST | FLAGS_BOOT_WEIGHTS;
  c->cache_timeout         = 0;
  c->finality              = 0;
  c->max_attempts          = 7;
  c->max_verified_hashes   = 5;
  c->alloc_verified_hashes = 0;
  c->min_deposit           = 0;
  c->node_limit            = 0;
  c->proof                 = PROOF_STANDARD;
  c->replace_latest_block  = 0;
  c->request_count         = 1;
  c->filters               = NULL;
  c->timeout               = 10000;

#ifndef DEV_NO_INC_RPC_ID
  c->id_count = 1;
#endif

  if (chain_id == CHAIN_ID_MAINNET)
    init_mainnet(c);
  else if (chain_id == CHAIN_ID_GOERLI)
    init_goerli(c);
  else if (chain_id == CHAIN_ID_IPFS)
    init_ipfs(c);
  else if (chain_id == CHAIN_ID_BTC)
    init_btc(c);
  else if (chain_id == CHAIN_ID_EWC)
    init_ewf(c);
  else if (chain_id == CHAIN_ID_LOCAL)
    in3_client_register_chain(c, 0x11, CHAIN_ETH, 1);

  return IN3_OK;
}

in3_ret_t in3_client_register_chain(in3_t* c, chain_id_t chain_id, in3_chain_type_t type, uint8_t version) {
  assert(c);

  in3_chain_t* chain     = &c->chain;
  chain->conf            = NULL;
  chain->chain_id        = chain_id;
  chain->verified_hashes = NULL;
  chain->type            = type;
  chain->version         = version;
  return IN3_OK;
}

static void chain_free(in3_chain_t* chain) {
  if (chain->verified_hashes) _free(chain->verified_hashes);
}

/* frees the data */
void in3_free(in3_t* a) {
  if (!a) return;

  // cleanup plugins
  in3_plugin_t *p = a->plugins, *n;
  while (p) {
    if (p->acts & PLGN_ACT_TERM)
      p->action_fn(p->data, PLGN_ACT_TERM, a);
    n = p->next;
    _free(p);
    p = n;
  }

  chain_free(&a->chain);

  if (a->filters) {
    in3_filter_t* f = NULL;
    for (size_t j = 0; j < a->filters->count; j++) {
      f = a->filters->array[j];
      if (f) f->release(f);
    }
    _free(a->filters->array);
    _free(a->filters);
  }

#ifdef PAY
  if (a->pay) {
    if (a->pay->cptr) {
      if (a->pay->free)
        a->pay->free(a->pay->cptr);
      else
        _free(a->pay->cptr);
    }
    _free(a->pay);
  }
#endif
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
  for (default_fn_t* d = default_registry; d; d = d->next)
    d->fn(c);

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

static in3_chain_type_t chain_type(chain_id_t id) {
  switch (id) {
    case CHAIN_ID_MAINNET:
    case CHAIN_ID_GOERLI:
    case CHAIN_ID_EWC:
    case CHAIN_ID_TOBALABA:
    case CHAIN_ID_EVAN:
    case CHAIN_ID_LOCAL:
      return CHAIN_ETH;
    case CHAIN_ID_IPFS:
      return CHAIN_IPFS;
    case CHAIN_ID_BTC:
      return CHAIN_BTC;
    default: return CHAIN_GENERIC;
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
  add_uint(sb, ',', "maxVerifiedHashes", c->max_verified_hashes);
  add_uint(sb, ',', "timeout", c->timeout);
  add_uint(sb, ',', "minDeposit", c->min_deposit);
  add_uint(sb, ',', "nodeProps", c->node_props);
  add_uint(sb, ',', "nodeLimit", c->node_limit);
  add_string(sb, ',', "proof", (c->proof == PROOF_NONE) ? "none" : (c->proof == PROOF_STANDARD ? "standard" : "full"));
  if (c->replace_latest_block)
    add_uint(sb, ',', "replaceLatestBlock", c->replace_latest_block);
  add_uint(sb, ',', "requestCount", c->request_count);

  in3_get_config_ctx_t cctx = {.client = c, .sb = sb};
  in3_plugin_execute_all(c, PLGN_ACT_CONFIG_GET, &cctx);
  sb_add_chars(sb, "}");

  char* r = sb->data;
  _free(sb);
  return r;
}

char* in3_configure(in3_t* c, const char* config) {
  json_ctx_t* json = parse_json((char*) config);
  char*       res  = NULL;

  if (!json || !json->result) return config_err("in3_configure", "parse error");
  if (c->pending) return config_err("in3_configure", "can not change config because there are pending requests!");
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
      int ct_ = -1;
      for (d_iterator_t it_ = d_iter(json->result); it_.left; d_iter_next(&it_)) {
        if (it_.token->key == key("chainType")) {
          EXPECT_TOK_U8(it_.token);
          ct_ = d_int(it_.token);
        }
      }
      c->chain.chain_id = chain_id(token);
      c->chain.type     = (ct_ == -1) ? chain_type(c->chain.chain_id) : (uint8_t) ct_;
      in3_client_register_chain(c, c->chain.chain_id, c->chain.type, 2);
      in3_plugin_execute_all(c, PLGN_ACT_CHAIN_CHANGE, c);
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
    else if (token->key == key("minDeposit")) {
      EXPECT_TOK_U64(token);
      c->min_deposit = d_long(token);
    }
    else if (token->key == key("nodeProps")) {
      EXPECT_TOK_U64(token);
      c->node_props = d_long(token);
    }
    else if (token->key == key("nodeLimit")) {
      EXPECT_TOK_U16(token);
      c->node_limit = (uint16_t) d_int(token);
    }
    else if (token->key == key("pay")) {
      EXPECT_TOK_OBJ(token);
#ifdef PAY
      char* type = d_get_string(token, "type");
      if (!type) type = "eth";
      pay_configure_t* p = find_payment(type);
      EXPECT_TOK(token, p, "the payment type was not registered");
      char* err = p->configure(c, token);
      EXPECT_TOK(token, err == NULL, err);

#else
      EXPECT_TOK(token, false, "pay_eth is not supporterd. Please build with -DPAY_ETH");
#endif
    }
    else if (token->key == key("proof")) {
      EXPECT_TOK_STR(token);
      EXPECT_TOK(token, !strcmp(d_string(token), "full") || !strcmp(d_string(token), "standard") || !strcmp(d_string(token), "none"), "expected values - full/standard/none");
      c->proof = strcmp(d_string(token), "full") == 0
                     ? PROOF_FULL
                     : (strcmp(d_string(token), "standard") == 0 ? PROOF_STANDARD : PROOF_NONE);
    }
    else if (token->key == key("replaceLatestBlock")) {
      EXPECT_TOK_U8(token);
      c->replace_latest_block = (uint8_t) d_int(token);
    }
    else if (token->key == key("requestCount")) {
      EXPECT_TOK_U8(token);
      EXPECT_CFG(d_int(token), "requestCount must be at least 1");
      c->request_count = (uint8_t) d_int(token);
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
        c->chain.verified_hashes[i].block_number = d_get_longk(n.token, key("block"));
        memcpy(c->chain.verified_hashes[i].hash, d_get_byteskl(n.token, key("hash"), 32)->data, 32);
      }
      c->alloc_verified_hashes = c->max_verified_hashes;
    }
    else {
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

in3_ret_t in3_plugin_register(const char* name, in3_t* c, in3_plugin_supp_acts_t acts, in3_plugin_act_fn action_fn, void* data, bool replace_ex) {
  if (!acts || !action_fn)
    return IN3_EINVAL;

  in3_plugin_t** p = &c->plugins;
  while (*p) {
    // check for action-specific rules here like allowing only one action handler per action, etc.
    if (replace_ex && (*p)->acts == acts) {
      if ((*p)->acts & PLGN_ACT_TERM) (*p)->action_fn((*p)->data, PLGN_ACT_TERM, c);
      (*p)->action_fn = action_fn;
      (*p)->data      = data;
#ifdef LOGGING
      (*p)->name = name;
#endif
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
#ifdef LOGGING
  new_p->name = name;
#endif

  // didn't find any existing, so we add a new ...
  c->plugin_acts |= acts;
  return IN3_OK;
}

in3_ret_t in3_plugin_execute_all(in3_t* c, in3_plugin_act_t action, void* plugin_ctx) {
  if (!in3_plugin_is_registered(c, action))
    return IN3_OK;

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
    case PLGN_ACT_CHAIN_CHANGE: return "chain_change";
    case PLGN_ACT_GET_DATA: return "get_data";
    case PLGN_ACT_ADD_PAYLOAD: return "add_payload";
  }
  return "unknown";
}
#endif

in3_ret_t in3_plugin_execute_first(in3_ctx_t* ctx, in3_plugin_act_t action, void* plugin_ctx) {
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
  return ctx_set_error(ctx, msg, IN3_EPLGN_NONE);
}

in3_ret_t in3_plugin_execute_first_or_none(in3_ctx_t* ctx, in3_plugin_act_t action, void* plugin_ctx) {
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
