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

#include "../../nodeselect/cache.h"
#include "../../nodeselect/nodelist.h"
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

static void whitelist_free(in3_whitelist_t* wl) {
  if (!wl) return;
  if (wl->addresses.data) _free(wl->addresses.data);
  _free(wl);
}

IN3_EXPORT_TEST void initChain(in3_chain_t* chain, chain_id_t chain_id, char* contract, char* registry_id, uint8_t version, int boot_node_count, in3_chain_type_t type, char* wl_contract) {
  assert(chain);
  assert(contract && strlen(contract) == 40);
  assert(chain_id == CHAIN_ID_LOCAL || registry_id);

  chain->conf            = NULL;
  chain->chain_id        = chain_id;
  chain->verified_hashes = NULL;
  chain->contract        = hex_to_new_bytes(contract, 40);
  chain->type            = type;
  chain->version         = version;
  memset(chain->registry_id, 0, 32);
  if (version > 1) {
    int l = hex_to_bytes(registry_id, -1, chain->registry_id, 32);
    if (l < 32) {
      memmove(chain->registry_id + 32 - l, chain->registry_id, l);
      memset(chain->registry_id, 0, 32 - l);
    }
  }
}

static void init_ipfs(in3_chain_t* chain) {
  initChain(chain, 0x7d0, "a93b57289070550c82edb1106e12bb37138948b8", "f0162ec6d785ee990e36bad865251f45af0916cf136169540c02b0dd9cb69196", 2, 2, CHAIN_IPFS, NULL);
}

static void init_mainnet(in3_chain_t* chain) {
  initChain(chain, 0x01, "ac1b824795e1eb1f6e609fe0da9b9af8beaab60f", "23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb", 2, 4, CHAIN_ETH, NULL);
}
static void init_ewf(in3_chain_t* chain) {
  initChain(chain, 0xf6, "039562872008f7a76674a6e7842804f0ad37cb13", "313454c05fc6e5336a3315ed2233da6b831d4cb826d836c3d603f2e2a9f1ed75", 2, 2, CHAIN_ETH, NULL);
}

static void init_btc(in3_chain_t* chain) {
  initChain(chain, 0x99, "c2c05fbfe76ee7748ae5f5b61b57a46cc4061c32", "53786c93e54c21d9852d093c394eee9df8d714d8f2534cdf92f9c9998c528d19", 2, 2, CHAIN_BTC, NULL);
}
static void init_kovan(in3_chain_t* chain) {
#ifdef IN3_STAGING
  // kovan
  initChain(chain, 0x2a, "0604014f2a5fdfafce3f2ec10c77c31d8e15ce6f", "d440f01322c8529892c204d3705ae871c514bafbb2f35907832a07322e0dc868", 2, 2, CHAIN_ETH, NULL);
#else
  // kovan
  initChain(chain, 0x2a, "4c396dcf50ac396e5fdea18163251699b5fcca25", "92eb6ad5ed9068a24c1c85276cd7eb11eda1e8c50b17fbaffaf3e8396df4becf", 2, 2, CHAIN_ETH, NULL);
#endif
}

static void init_goerli(in3_chain_t* chain) {

#ifdef IN3_STAGING
  // goerli
  initChain(chain, 0x05, "814fb2203f9848192307092337340dcf791a3fed", "0f687341e0823fa5288dc9edd8a00950b35cc7e481ad7eaccaf61e4e04a61e08", 2, 2, CHAIN_ETH, NULL);
#else
  // goerli
  initChain(chain, 0x05, "5f51e413581dd76759e9eed51e63d14c8d1379c8", "67c02e5e272f9d6b4a33716614061dd298283f86351079ef903bf0d4410a44ea", 2, 2, CHAIN_ETH, NULL);
#endif
}

static in3_ret_t in3_client_init(in3_t* c, chain_id_t chain_id) {
  assert(c);

  c->flags                 = FLAGS_STATS | FLAGS_AUTO_UPDATE_LIST | FLAGS_BOOT_WEIGHTS;
  c->cache_timeout         = 0;
  c->chain_id              = chain_id ? chain_id : CHAIN_ID_MAINNET; // mainnet
  c->finality              = 0;
  c->max_attempts          = 7;
  c->max_verified_hashes   = 5;
  c->alloc_verified_hashes = 0;
  c->min_deposit           = 0;
  c->node_limit            = 0;
  c->proof                 = PROOF_STANDARD;
  c->replace_latest_block  = 0;
  c->request_count         = 1;
  c->chains_length         = chain_id ? 1 : 7;
  c->chains                = _malloc(sizeof(in3_chain_t) * c->chains_length);
  c->filters               = NULL;
  c->timeout               = 10000;

#ifndef DEV_NO_INC_RPC_ID
  c->id_count = 1;
#endif

  in3_chain_t* chain = c->chains;

  if (!chain_id || chain_id == CHAIN_ID_MAINNET)
    init_mainnet(chain++);

  if (!chain_id || chain_id == CHAIN_ID_KOVAN)
    init_kovan(chain++);

  if (!chain_id || chain_id == CHAIN_ID_GOERLI)
    init_goerli(chain++);

  if (!chain_id || chain_id == CHAIN_ID_IPFS)
    init_ipfs(chain++);

  if (!chain_id || chain_id == CHAIN_ID_BTC)
    init_btc(chain++);

  if (!chain_id || chain_id == CHAIN_ID_EWC)
    init_ewf(chain++);

  if (!chain_id || chain_id == CHAIN_ID_LOCAL) {
    initChain(chain, 0x11, "f0fb87f4757c77ea3416afe87f36acaa0496c7e9", NULL, 1, 1, CHAIN_ETH, NULL);
  }
  if (chain_id && chain == c->chains) {
    c->chains_length = 0;
    return IN3_ECONFIG;
  }
  return IN3_OK;
}
in3_chain_t* in3_get_chain(const in3_t* c) {
  assert(c);

  // shortcut for single chain
  if (c->chains_length == 1)
    return c->chains->chain_id == c->chain_id ? c->chains : NULL;

  // search for multi chain
  for (int i = 0; i < c->chains_length; i++) {
    if (c->chains[i].chain_id == c->chain_id) return &c->chains[i];
  }
  return NULL;
}

void in3_set_chain_id(in3_t* c, chain_id_t chain_id) {
  c->chain_id = chain_id;
  in3_plugin_execute_all(c, PLGN_ACT_CHAIN_CHANGE, NULL);
}

in3_chain_t* in3_find_chain(const in3_t* c, chain_id_t chain_id) {
  assert(c);

  // shortcut for single chain
  if (c->chains_length == 1)
    return c->chains->chain_id == chain_id ? c->chains : NULL;

  // search for multi chain
  for (int i = 0; i < c->chains_length; i++) {
    if (c->chains[i].chain_id == chain_id) return &c->chains[i];
  }
  return NULL;
}

in3_ret_t in3_client_register_chain(in3_t* c, chain_id_t chain_id, in3_chain_type_t type, address_t contract, bytes32_t registry_id, uint8_t version) {
  assert(chain_id);
  assert(c);
  assert(contract);
  assert(registry_id);

  in3_chain_t* chain = in3_find_chain(c, chain_id);
  if (!chain) {
    c->chains = _realloc(c->chains, sizeof(in3_chain_t) * (c->chains_length + 1), sizeof(in3_chain_t) * c->chains_length);
    if (c->chains == NULL) return IN3_ENOMEM;
    chain                  = c->chains + c->chains_length;
    chain->conf            = NULL;
    chain->verified_hashes = NULL;
    c->chains_length++;
  }
  else {
    if (chain->contract)
      b_free(chain->contract);
  }

  chain->chain_id = chain_id;
  chain->contract = b_new(contract, 20);
  chain->type     = type;
  chain->version  = version;
  memcpy(chain->registry_id, registry_id, 32);
  return chain->contract ? IN3_OK : IN3_ENOMEM;
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

  for (int i = 0; i < a->chains_length; i++) {
    if (a->chains[i].verified_hashes) _free(a->chains[i].verified_hashes);
    b_free(a->chains[i].contract);
  }
  if (a->chains) _free(a->chains);

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

in3_t* in3_new() {
  return in3_for_chain(0);
}

static chain_id_t get_chain_from_key(d_key_t k) {
  if (k == key("0x1")) return CHAIN_ID_MAINNET;
  if (k == key("0x2a")) return CHAIN_ID_KOVAN;
  if (k == key("0x5")) return CHAIN_ID_GOERLI;
  if (k == key("0xf6")) return CHAIN_ID_EWC;
  if (k == key("0x99")) return CHAIN_ID_BTC;
  if (k == key("0x7d0")) return CHAIN_ID_IPFS;
  if (k == key("0xdeaf")) return 0xdeaf;
  char tmp[7];
  for (int i = 2; i < 0xffff; i++) {
    sprintf(tmp, "0x%x", i);
    if (k == key(tmp)) return i;
  }
  return 0;
}

static chain_id_t chain_id(d_token_t* t) {
  if (d_type(t) == T_STRING) {
    char* c = d_string(t);
    if (!strcmp(c, "mainnet")) return CHAIN_ID_MAINNET;
    if (!strcmp(c, "kovan")) return CHAIN_ID_KOVAN;
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

char* in3_get_config(in3_t* c) {
  sb_t*        sb    = sb_new("");
  in3_chain_t* chain = in3_get_chain(c);
  add_bool(sb, '{', "autoUpdateList", c->flags & FLAGS_AUTO_UPDATE_LIST);
  add_uint(sb, ',', "chainId", c->chain_id);
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
  if (c->chain_id == CHAIN_ID_LOCAL && chain)
    add_string(sb, ',', "rpc", chain->nodelist->url);

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
    else if (token->key == key("chainId")) {
      EXPECT_TOK(token, IS_D_UINT32(token) || (d_type(token) == T_STRING && chain_id(token) != 0), "expected uint32 or string value (mainnet/goerli/kovan)");
      c->chain_id = chain_id(token);
    }
    else if (token->key == key("signatureCount")) {
      EXPECT_TOK_U8(token);
      c->signature_count = (uint8_t) d_int(token);
    }
    else if (token->key == key("finality")) {
      EXPECT_TOK_U16(token);
#ifdef POA
      if (c->chain_id == CHAIN_ID_GOERLI || c->chain_id == CHAIN_ID_KOVAN)
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
      in3_chain_t* chain = in3_get_chain(c);
      EXPECT_CFG(chain, "chain not found");
      if (c->max_verified_hashes < d_long(token)) {
        chain->verified_hashes = _realloc(chain->verified_hashes,
                                          sizeof(in3_verified_hash_t) * d_long(token),
                                          sizeof(in3_verified_hash_t) * c->max_verified_hashes);
        // clear newly allocated memory
        memset(chain->verified_hashes + c->max_verified_hashes, 0, (d_long(token) - c->max_verified_hashes) * sizeof(in3_verified_hash_t));
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
      in3_node_props_set(&c->node_props, NODE_PROP_MIN_BLOCK_HEIGHT, d_int(token));
    }
    else if (token->key == key("requestCount")) {
      EXPECT_TOK_U8(token);
      EXPECT_CFG(d_int(token), "requestCount must be at least 1");
      c->request_count = (uint8_t) d_int(token);
    }
    else if (token->key == key("rpc")) {
      EXPECT_TOK_STR(token);
      c->proof           = PROOF_NONE;
      c->chain_id        = CHAIN_ID_LOCAL;
      c->request_count   = 1;
      in3_chain_t* chain = in3_get_chain(c);
      in3_node_t*  n     = &chain->nodelist[0];
      if (n->url) _free(n->url);
      n->url = _malloc(d_len(token) + 1);
      strcpy(n->url, d_string(token));
      _free(chain->nodelist_upd8_params);
      chain->nodelist_upd8_params = NULL;
    }
    else if (token->key == key("servers") || token->key == key("nodes")) {
      EXPECT_TOK_OBJ(token);
      for (d_iterator_t ct = d_iter(token); ct.left; d_iter_next(&ct)) {
        EXPECT_TOK_OBJ(ct.token);
        EXPECT_TOK_KEY_HEXSTR(ct.token);

        // register chain
        chain_id_t   chain_id    = get_chain_from_key(ct.token->key);
        bytes_t*     contract    = d_get_byteskl(ct.token, key("contract"), 20);
        bytes_t*     registry_id = d_get_byteskl(ct.token, key("registryId"), 32);
        in3_chain_t* chain       = in3_find_chain(c, chain_id);

        if (!chain) {
          EXPECT_CFG(contract && registry_id, "invalid contract/registry!");
          EXPECT_CFG((in3_client_register_chain(c, chain_id, chain ? chain->type : CHAIN_ETH, contract ? contract->data : chain->contract->data, registry_id ? registry_id->data : chain->registry_id, 2)) == IN3_OK,
                     "register chain failed");
          chain = in3_find_chain(c, chain_id);
          EXPECT_CFG(chain != NULL, "invalid chain id!");
        }

        // chain_props
        for (d_iterator_t cp = d_iter(ct.token); cp.left; d_iter_next(&cp)) {
          if (cp.token->key == key("contract")) {
            EXPECT_TOK_ADDR(cp.token);
            memcpy(chain->contract->data, cp.token->data, cp.token->len);
          }
          else if (cp.token->key == key("registryId")) {
            EXPECT_TOK_B256(cp.token);
            bytes_t data = d_to_bytes(cp.token);
            memcpy(chain->registry_id, data.data, 32);
          }
          else if (cp.token->key == key("verifiedHashes")) {
            EXPECT_TOK_ARR(cp.token);
            EXPECT_TOK(cp.token, (unsigned) d_len(cp.token) <= c->max_verified_hashes, "expected array len <= maxVerifiedHashes");
            if (!chain->verified_hashes)
              chain->verified_hashes = _calloc(c->max_verified_hashes, sizeof(in3_verified_hash_t));
            else
              // clear extra verified_hashes (preceding ones will be overwritten anyway)
              memset(chain->verified_hashes + d_len(cp.token), 0, (c->max_verified_hashes - d_len(cp.token)) * sizeof(in3_verified_hash_t));
            int i = 0;
            for (d_iterator_t n = d_iter(cp.token); n.left; d_iter_next(&n), i++) {
              EXPECT_TOK_U64(d_get(n.token, key("block")));
              EXPECT_TOK_B256(d_get(n.token, key("hash")));
              chain->verified_hashes[i].block_number = d_get_longk(n.token, key("block"));
              memcpy(chain->verified_hashes[i].hash, d_get_byteskl(n.token, key("hash"), 32)->data, 32);
            }
            c->alloc_verified_hashes = c->max_verified_hashes;
          }
          else {
            EXPECT_TOK(cp.token, false, "unsupported config option!");
          }
        }
        in3_client_run_chain_whitelisting(chain);
      }
    }
    else {
      in3_configure_ctx_t cctx    = {.client = c, .json = json, .token = token, .error_msg = NULL};
      bool                handled = false;
      for (in3_plugin_t* p = c->plugins; p; p = p->next) {
        if (p->acts & PLGN_ACT_CONFIG_SET) {
          in3_ret_t r = p->action_fn(p->data, PLGN_ACT_CONFIG_SET, &cctx);
          if (r == IN3_EIGNORE)
            continue;
          else if (r != IN3_OK)
            EXPECT_TOK(token, false, cctx.error_msg ? cctx.error_msg : "error configuring this option!");
          handled = true;
          break;
        }
      }

      if (!handled) EXPECT_TOK(token, false, "unsupported config option!");
    }
  }

  if (c->signature_count && c->chain_id != CHAIN_ID_LOCAL && !c->replace_latest_block) {
    in3_log_warn("signatureCount > 0 without replaceLatestBlock is bound to fail; using default (" STR(DEF_REPL_LATEST_BLK) ")\n");
    c->replace_latest_block = DEF_REPL_LATEST_BLK;
  }

  EXPECT_CFG(in3_get_chain(c), "chain corresponding to chain id not initialized!");
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

  // didn't find any existing, so we add a new ...
  *p              = _malloc(sizeof(in3_plugin_t));
  (*p)->acts      = acts;
  (*p)->action_fn = action_fn;
  (*p)->data      = data;
  (*p)->next      = NULL;
#ifdef LOGGING
  (*p)->name = name;
#endif
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
    case PLGN_ACT_RPC_VERIFY: return "rpc_verrify";
    case PLGN_ACT_CACHE_SET: return "cache_set";
    case PLGN_ACT_CACHE_GET: return "cache_get";
    case PLGN_ACT_CACHE_CLEAR: return "cache_clear";
    case PLGN_ACT_CONFIG_SET: return "config_set";
    case PLGN_ACT_CONFIG_GET: return "config_get";
    case PLGN_ACT_PAY_PREPARE: return "pay_prepare";
    case PLGN_ACT_PAY_FOLLOWUP: return "pay_followup";
    case PLGN_ACT_PAY_HANDLE: return "pay_handle";
    case PLGN_ACT_PAY_SIGN_REQ: return "pay_sign_req";
    case PLGN_ACT_NL_PICK_DATA: return "nl_pick_data";
    case PLGN_ACT_NL_PICK_SIGNER: return "nl_pick_signer";
    case PLGN_ACT_NL_PICK_FOLLOWUP: return "nl_pick_followup";
    case PLGN_ACT_LOG_ERROR: return "log_error";
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
