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
#include "cache.h"
#include "client.h"
#include "nodelist.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#ifndef __ZEPHYR__
#include <sys/time.h>
#endif
#include <time.h>

#define EXPECT(cond, exit) \
  do {                     \
    if (!(cond))           \
      (exit);              \
  } while (0)

#define EXPECT_CFG(cond, err) EXPECT(cond, { \
  res = malloc(strlen(err) + 1);             \
  strcpy(res, err);                          \
  goto cleanup;                              \
})
#define EXPECT_CFG_NCP_ERR(cond, err) EXPECT(cond, { res = err; goto cleanup; })
#define EXPECT_TOK(token, cond, err) EXPECT_CFG_NCP_ERR(cond, config_err(d_get_keystr(token->key), err))
#define EXPECT_TOK_BOOL(token) EXPECT_TOK(token, d_type(token) == T_BOOLEAN, "expected boolean value")
#define EXPECT_TOK_STR(token) EXPECT_TOK(token, d_type(token) == T_STRING, "expected string value")
#define EXPECT_TOK_ARR(token) EXPECT_TOK(token, d_type(token) == T_ARRAY, "expected array")
#define EXPECT_TOK_OBJ(token) EXPECT_TOK(token, d_type(token) == T_OBJECT, "expected object")
#define EXPECT_TOK_ADDR(token) EXPECT_TOK(token, d_type(token) == T_BYTES && d_len(token) == 20, "expected address")
#define EXPECT_TOK_B256(token) EXPECT_TOK(token, d_type(token) == T_BYTES && d_len(token) == 32, "expected 256 bit data")
#define IS_D_UINT64(token) ((d_type(token) == T_INTEGER || (d_type(token) == T_BYTES && d_len(token) <= 8)) && d_long(token) <= UINT64_MAX)
#define IS_D_UINT32(token) ((d_type(token) == T_INTEGER || d_type(token) == T_BYTES) && d_long(token) <= UINT32_MAX)
#define IS_D_UINT16(token) (d_type(token) == T_INTEGER && d_int(token) >= 0 && d_int(token) <= UINT16_MAX)
#define IS_D_UINT8(token) (d_type(token) == T_INTEGER && d_int(token) >= 0 && d_int(token) <= UINT8_MAX)
#define EXPECT_TOK_U8(token) EXPECT_TOK(token, IS_D_UINT8(token), "expected uint8 value")
#define EXPECT_TOK_U16(token) EXPECT_TOK(token, IS_D_UINT16(token), "expected uint16 value")
#define EXPECT_TOK_U32(token) EXPECT_TOK(token, IS_D_UINT32(token), "expected uint32 value")
#define EXPECT_TOK_U64(token) EXPECT_TOK(token, IS_D_UINT64(token), "expected uint64 value")
#define EXPECT_TOK_KEY_HEXSTR(token) EXPECT_TOK(token, is_hex_str(d_get_keystr(token->key)), "expected hex str")

// set the defaults
static in3_transport_send     default_transport = NULL;
static in3_storage_handler_t* default_storage   = NULL;
static in3_signer_t*          default_signer    = NULL;

/**
 * defines a default transport which is used when creating a new client.
 */
void in3_set_default_transport(in3_transport_send transport) {
  default_transport = transport;
}

/**
 * defines a default storage handler which is used when creating a new client.
 */
void in3_set_default_storage(in3_storage_handler_t* cacheStorage) {
  default_storage = cacheStorage;
}
/**
 * defines a default signer which is used when creating a new client.
 */
void in3_set_default_signer(in3_signer_t* signer) {
  default_signer = signer;
}

static void whitelist_free(in3_whitelist_t* wl) {
  if (!wl) return;
  if (wl->addresses.data) _free(wl->addresses.data);
  _free(wl);
}

static uint16_t avg_block_time_for_chain_id(chain_id_t id) {
  switch (id) {
    case ETH_CHAIN_ID_MAINNET: return 15;
    case ETH_CHAIN_ID_KOVAN: return 6;
    case ETH_CHAIN_ID_GOERLI: return 15;
    default: return 5;
  }
}

IN3_EXPORT_TEST void initChain(in3_chain_t* chain, chain_id_t chain_id, char* contract, char* registry_id, uint8_t version, int boot_node_count, in3_chain_type_t type, char* wl_contract) {
  chain->chain_id             = chain_id;
  chain->init_addresses       = NULL;
  chain->last_block           = 0;
  chain->verified_hashes      = NULL;
  chain->contract             = hex_to_new_bytes(contract, 40);
  chain->nodelist             = _calloc(boot_node_count, sizeof(in3_node_t));
  chain->nodelist_length      = boot_node_count;
  chain->weights              = _calloc(boot_node_count, sizeof(in3_node_weight_t));
  chain->type                 = type;
  chain->version              = version;
  chain->whitelist            = NULL;
  chain->nodelist_upd8_params = _calloc(1, sizeof(*(chain->nodelist_upd8_params)));
  chain->avg_block_time       = avg_block_time_for_chain_id(chain_id);
  if (wl_contract) {
    chain->whitelist                 = _malloc(sizeof(in3_whitelist_t));
    chain->whitelist->addresses.data = NULL;
    chain->whitelist->addresses.len  = 0;
    chain->whitelist->needs_update   = true;
    chain->whitelist->last_block     = 0;
    hex_to_bytes(wl_contract, -1, chain->whitelist->contract, 20);
  }
  memset(chain->registry_id, 0, 32);
  if (version > 1) {
    int l = hex_to_bytes(registry_id, -1, chain->registry_id, 32);
    if (l < 32) {
      memmove(chain->registry_id + 32 - l, chain->registry_id, l);
      memset(chain->registry_id, 0, 32 - l);
    }
  }
}

static void initNode(in3_chain_t* chain, int node_index, char* address, char* url) {
  in3_node_t* node = chain->nodelist + node_index;
  node->address    = hex_to_new_bytes(address, 40);
  node->index      = node_index;
  node->capacity   = 1;
  node->deposit    = 0;
  node->props      = 0xFF;
  node->url        = _malloc(strlen(url) + 1);
  BIT_CLEAR(node->attrs, ATTR_WHITELISTED);
  BIT_SET(node->attrs, ATTR_BOOT_NODE);
  memcpy(node->url, url, strlen(url) + 1);

  in3_node_weight_t* weight   = chain->weights + node_index;
  weight->blacklisted_until   = 0;
  weight->response_count      = 0;
  weight->total_response_time = 0;
}

static void init_ipfs(in3_chain_t* chain) {
  // ipfs
  initChain(chain, 0x7d0, "a93b57289070550c82edb1106e12bb37138948b8", "f0162ec6d785ee990e36bad865251f45af0916cf136169540c02b0dd9cb69196", 2, 2, CHAIN_IPFS, NULL);
  initNode(chain, 0, "45d45e6ff99e6c34a235d263965910298985fcfe", "https://in3-v2.slock.it/ipfs/nd-1");
  initNode(chain, 1, "1fe2e9bf29aa1938859af64c413361227d04059a", "https://in3-v2.slock.it/ipfs/nd-5");
}

static void init_mainnet(in3_chain_t* chain) {
  initChain(chain, 0x01, "ac1b824795e1eb1f6e609fe0da9b9af8beaab60f", "23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb", 2, 2, CHAIN_ETH, NULL);
  initNode(chain, 0, "45d45e6ff99e6c34a235d263965910298985fcfe", "https://in3-v2.slock.it/mainnet/nd-1");
  initNode(chain, 1, "1fe2e9bf29aa1938859af64c413361227d04059a", "https://in3-v2.slock.it/mainnet/nd-2");
}

static void init_btc(in3_chain_t* chain) {
  initChain(chain, 0xFF01, "85613723dB1Bc29f332A37EeF10b61F8a4225c7e", "23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb", 1, 1, CHAIN_BTC, NULL);
  initNode(chain, 0, "8f354b72856e516f1e931c97d1ed3bf1709f38c9", "http://localhost:8500");
  if (chain->nodelist_upd8_params) {
    _free(chain->nodelist_upd8_params);
    chain->nodelist_upd8_params = NULL;
  }
}

static void init_kovan(in3_chain_t* chain) {
#ifdef IN3_STAGING
  // kovan
  initChain(chain, 0x2a, "0604014f2a5fdfafce3f2ec10c77c31d8e15ce6f", "d440f01322c8529892c204d3705ae871c514bafbb2f35907832a07322e0dc868", 2, 2, CHAIN_ETH, NULL);
  initNode(chain, 0, "784bfa9eb182c3a02dbeb5285e3dba92d717e07a", "https://in3.stage.slock.it/kovan/nd-1");
  initNode(chain, 1, "17cdf9ec6dcae05c5686265638647e54b14b41a2", "https://in3.stage.slock.it/kovan/nd-2");
#else
  // kovan
  initChain(chain, 0x2a, "4c396dcf50ac396e5fdea18163251699b5fcca25", "92eb6ad5ed9068a24c1c85276cd7eb11eda1e8c50b17fbaffaf3e8396df4becf", 2, 2, CHAIN_ETH, NULL);
  initNode(chain, 0, "45d45e6ff99e6c34a235d263965910298985fcfe", "https://in3-v2.slock.it/kovan/nd-1");
  initNode(chain, 1, "1fe2e9bf29aa1938859af64c413361227d04059a", "https://in3-v2.slock.it/kovan/nd-2");
#endif
}

static void init_goerli(in3_chain_t* chain) {

#ifdef IN3_STAGING
  // goerli
  initChain(chain, 0x05, "814fb2203f9848192307092337340dcf791a3fed", "0f687341e0823fa5288dc9edd8a00950b35cc7e481ad7eaccaf61e4e04a61e08", 2, 2, CHAIN_ETH, NULL);
  initNode(chain, 0, "45d45e6ff99e6c34a235d263965910298985fcfe", "https://in3.stage.slock.it/goerli/nd-1");
  initNode(chain, 1, "1fe2e9bf29aa1938859af64c413361227d04059a", "https://in3.stage.slock.it/goerli/nd-2");
#else
  // goerli
  initChain(chain, 0x05, "5f51e413581dd76759e9eed51e63d14c8d1379c8", "67c02e5e272f9d6b4a33716614061dd298283f86351079ef903bf0d4410a44ea", 2, 2, CHAIN_ETH, NULL);
  initNode(chain, 0, "45d45e6ff99e6c34a235d263965910298985fcfe", "https://in3-v2.slock.it/goerli/nd-1");
  initNode(chain, 1, "1fe2e9bf29aa1938859af64c413361227d04059a", "https://in3-v2.slock.it/goerli/nd-2");
#endif
}

static in3_ret_t in3_client_init(in3_t* c, chain_id_t chain_id) {
  c->flags                = FLAGS_STATS | FLAGS_AUTO_UPDATE_LIST;
  c->cache                = NULL;
  c->signer               = NULL;
  c->cache_timeout        = 0;
  c->chain_id             = chain_id ? chain_id : ETH_CHAIN_ID_MAINNET; // mainnet
  c->key                  = NULL;
  c->finality             = 0;
  c->max_attempts         = 7;
  c->max_block_cache      = 0;
  c->max_code_cache       = 0;
  c->max_verified_hashes  = 5;
  c->min_deposit          = 0;
  c->node_limit           = 0;
  c->proof                = PROOF_STANDARD;
  c->replace_latest_block = 0;
  c->request_count        = 1;
  c->chains_length        = chain_id ? 1 : 6;
  c->chains               = _malloc(sizeof(in3_chain_t) * c->chains_length);
  c->filters              = NULL;
  c->timeout              = 10000;

  //TODO check for failed malloc!

  in3_chain_t* chain = c->chains;

  if (!chain_id || chain_id == ETH_CHAIN_ID_MAINNET)
    init_mainnet(chain++);

  if (!chain_id || chain_id == ETH_CHAIN_ID_KOVAN)
    init_kovan(chain++);

  if (!chain_id || chain_id == ETH_CHAIN_ID_GOERLI)
    init_goerli(chain++);

  if (!chain_id || chain_id == ETH_CHAIN_ID_IPFS)
    init_ipfs(chain++);

  if (!chain_id || chain_id == ETH_CHAIN_ID_BTC)
    init_btc(chain++);

  if (!chain_id || chain_id == ETH_CHAIN_ID_LOCAL) {
    initChain(chain, 0xFFFF, "f0fb87f4757c77ea3416afe87f36acaa0496c7e9", NULL, 1, 1, CHAIN_ETH, NULL);
    initNode(chain++, 0, "784bfa9eb182c3a02dbeb5285e3dba92d717e07a", "http://localhost:8545");
  }
  if (chain_id && chain == c->chains) {
    c->chains_length = 0;
    return IN3_ECONFIG;
  }
  return IN3_OK;
}

in3_chain_t* in3_find_chain(in3_t* c, chain_id_t chain_id) {
  // shortcut for single chain
  if (c->chains_length == 1)
    return c->chains->chain_id == chain_id ? c->chains : NULL;

  // search for multi chain
  for (int i = 0; i < c->chains_length; i++) {
    if (c->chains[i].chain_id == chain_id) return &c->chains[i];
  }
  return NULL;
}

in3_ret_t in3_client_register_chain(in3_t* c, chain_id_t chain_id, in3_chain_type_t type, address_t contract, bytes32_t registry_id, uint8_t version, address_t wl_contract) {
  in3_chain_t* chain = in3_find_chain(c, chain_id);
  if (!chain) {
    c->chains = _realloc(c->chains, sizeof(in3_chain_t) * (c->chains_length + 1), sizeof(in3_chain_t) * c->chains_length);
    if (c->chains == NULL) return IN3_ENOMEM;
    chain                       = c->chains + c->chains_length;
    chain->nodelist             = NULL;
    chain->nodelist_length      = 0;
    chain->weights              = NULL;
    chain->init_addresses       = NULL;
    chain->whitelist            = NULL;
    chain->last_block           = 0;
    chain->nodelist_upd8_params = _calloc(1, sizeof(*(chain->nodelist_upd8_params)));
    chain->verified_hashes      = NULL;
    chain->avg_block_time       = avg_block_time_for_chain_id(chain_id);
    c->chains_length++;

  } else {
    if (chain->contract)
      b_free(chain->contract);
    if (chain->whitelist)
      whitelist_free(chain->whitelist);
  }

  chain->chain_id  = chain_id;
  chain->contract  = b_new((char*) contract, 20);
  chain->type      = type;
  chain->version   = version;
  chain->whitelist = NULL;
  memcpy(chain->registry_id, registry_id, 32);
  _free(chain->nodelist_upd8_params);
  chain->nodelist_upd8_params = NULL;

  if (wl_contract) {
    chain->whitelist                 = _malloc(sizeof(in3_whitelist_t));
    chain->whitelist->addresses.data = NULL;
    chain->whitelist->addresses.len  = 0;
    chain->whitelist->needs_update   = true;
    chain->whitelist->last_block     = 0;
    memcpy(chain->whitelist->contract, wl_contract, 20);
  }

  return chain->contract ? IN3_OK : IN3_ENOMEM;
}

in3_ret_t in3_client_add_node(in3_t* c, chain_id_t chain_id, char* url, in3_node_props_t props, address_t address) {
  in3_chain_t* chain = in3_find_chain(c, chain_id);
  if (!chain) return IN3_EFIND;
  in3_node_t* node       = NULL;
  int         node_index = chain->nodelist_length;
  for (int i = 0; i < chain->nodelist_length; i++) {
    if (memcmp(chain->nodelist[i].address->data, address, 20) == 0) {
      node       = chain->nodelist + i;
      node_index = i;
      break;
    }
  }
  if (!node) {
    chain->nodelist = chain->nodelist
                          ? _realloc(chain->nodelist, sizeof(in3_node_t) * (chain->nodelist_length + 1), sizeof(in3_node_t) * chain->nodelist_length)
                          : _calloc(chain->nodelist_length + 1, sizeof(in3_node_t));
    chain->weights = chain->weights
                         ? _realloc(chain->weights, sizeof(in3_node_weight_t) * (chain->nodelist_length + 1), sizeof(in3_node_weight_t) * chain->nodelist_length)
                         : _calloc(chain->nodelist_length + 1, sizeof(in3_node_weight_t));
    if (!chain->nodelist || !chain->weights) return IN3_ENOMEM;
    node           = chain->nodelist + chain->nodelist_length;
    node->address  = b_new((char*) address, 20);
    node->index    = chain->nodelist_length;
    node->capacity = 1;
    node->deposit  = 0;
    BIT_CLEAR(node->attrs, ATTR_WHITELISTED);
    chain->nodelist_length++;
  } else
    _free(node->url);

  node->props = props;
  node->url   = _malloc(strlen(url) + 1);
  memcpy(node->url, url, strlen(url) + 1);

  in3_node_weight_t* weight   = chain->weights + node_index;
  weight->blacklisted_until   = 0;
  weight->response_count      = 0;
  weight->total_response_time = 0;
  return IN3_OK;
}

in3_ret_t in3_client_remove_node(in3_t* c, chain_id_t chain_id, address_t address) {
  in3_chain_t* chain = in3_find_chain(c, chain_id);
  if (!chain) return IN3_EFIND;
  int node_index = -1;
  for (int i = 0; i < chain->nodelist_length; i++) {
    if (memcmp(chain->nodelist[i].address->data, address, 20) == 0) {
      node_index = i;
      break;
    }
  }
  if (node_index == -1) return IN3_EFIND;
  if (chain->nodelist[node_index].url)
    _free(chain->nodelist[node_index].url);
  if (chain->nodelist[node_index].address)
    b_free(chain->nodelist[node_index].address);

  if (node_index < chain->nodelist_length - 1) {
    memmove(chain->nodelist + node_index, chain->nodelist + node_index + 1, sizeof(in3_node_t) * (chain->nodelist_length - 1 - node_index));
    memmove(chain->weights + node_index, chain->weights + node_index + 1, sizeof(in3_node_weight_t) * (chain->nodelist_length - 1 - node_index));
  }
  chain->nodelist_length--;
  if (!chain->nodelist_length) {
    _free(chain->nodelist);
    _free(chain->weights);
    chain->nodelist = NULL;
    chain->weights  = NULL;
  }
  return IN3_OK;
}

in3_ret_t in3_client_clear_nodes(in3_t* c, chain_id_t chain_id) {
  in3_chain_t* chain = in3_find_chain(c, chain_id);
  if (!chain) return IN3_EFIND;
  in3_nodelist_clear(chain);
  chain->nodelist        = NULL;
  chain->weights         = NULL;
  chain->nodelist_length = 0;
  return IN3_OK;
}

/* frees the data */
void in3_free(in3_t* a) {
  if (!a) return;
  int i;
  for (i = 0; i < a->chains_length; i++) {
    if (a->chains[i].verified_hashes) _free(a->chains[i].verified_hashes);
    in3_nodelist_clear(a->chains + i);
    b_free(a->chains[i].contract);
    whitelist_free(a->chains[i].whitelist);
    _free(a->chains[i].nodelist_upd8_params);
  }
  if (a->signer) _free(a->signer);
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
  if (a->key) _free(a->key);
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

  if (default_transport) c->transport = default_transport;
  if (default_storage) c->cache = default_storage;
  if (default_signer) c->signer = default_signer;

#ifndef TEST
  in3_log_set_quiet(1);
#endif
  return c;
}

in3_t* in3_new() {
  return in3_for_chain(0);
}

static chain_id_t chain_id(d_token_t* t) {
  if (d_type(t) == T_STRING) {
    char* c = d_string(t);
    if (!strcmp(c, "mainnet")) return 1;
    if (!strcmp(c, "kovan")) return 0x2a;
    if (!strcmp(c, "goerli")) return 0x5;
    // 0 is allowed (as chain_id for local chain) if t is T_INT,
    // but for T_STRING it's an error
    return 0;
  }
  return d_long(t);
}

static inline char* config_err(const char* keyname, const char* err) {
  char* s = _malloc(strlen(keyname) + strlen(err) + 4);
  if (s)
    sprintf(s, "%s: %s!", keyname, err);
  return s;
}

static inline bool is_hex_str(const char* str) {
  if (str[0] == '0' && str[1] == 'x')
    str += 2;
  return str[strspn(str, "0123456789abcdefABCDEF")] == 0;
}

char* in3_configure(in3_t* c, const char* config) {
  d_track_keynames(1);
  d_clear_keynames();
  json_ctx_t* cnf = parse_json((char*) config);
  d_track_keynames(0);
  char* res = NULL;

  if (!cnf || !cnf->result) return config_err("in3_configure", "parse error");
  for (d_iterator_t iter = d_iter(cnf->result); iter.left; d_iter_next(&iter)) {
    d_token_t* token = iter.token;
    if (token->key == key("autoUpdateList")) {
      EXPECT_TOK_BOOL(token);
      BITMASK_SET_BOOL(c->flags, FLAGS_AUTO_UPDATE_LIST, (d_int(token) ? true : false));
    } else if (token->key == key("chainId")) {
      EXPECT_TOK(token, IS_D_UINT32(token) || (d_type(token) == T_STRING && chain_id(token) != 0), "expected uint32 or string value (mainnet/goerli/kovan)");
      c->chain_id = chain_id(token);
    } else if (token->key == key("signatureCount")) {
      EXPECT_TOK_U8(token);
      c->signature_count = (uint8_t) d_int(token);
    } else if (token->key == key("finality")) {
      EXPECT_TOK_U16(token);
#ifdef POA
      if (c->chain_id == ETH_CHAIN_ID_GOERLI || c->chain_id == ETH_CHAIN_ID_KOVAN)
        EXPECT_CFG(d_int(token) > 0 && d_int(token) <= 100, "expected % value");
#endif
      c->finality = (uint16_t) d_int(token);
    } else if (token->key == key("includeCode")) {
      EXPECT_TOK_BOOL(token);
      BITMASK_SET_BOOL(c->flags, FLAGS_INCLUDE_CODE, (d_int(token) ? true : false));
    } else if (token->key == key("maxAttempts")) {
      EXPECT_TOK_U16(token);
      c->max_attempts = d_int(token);
    } else if (token->key == key("keepIn3")) {
      EXPECT_TOK_BOOL(token);
      BITMASK_SET_BOOL(c->flags, FLAGS_KEEP_IN3, (d_int(token) ? true : false));
    } else if (token->key == key("stats")) {
      EXPECT_TOK_BOOL(token);
      BITMASK_SET_BOOL(c->flags, FLAGS_STATS, (d_int(token) ? true : false));
    } else if (token->key == key("useBinary")) {
      EXPECT_TOK_BOOL(token);
      BITMASK_SET_BOOL(c->flags, FLAGS_BINARY, (d_int(token) ? true : false));
    } else if (token->key == key("useHttp")) {
      EXPECT_TOK_BOOL(token);
      BITMASK_SET_BOOL(c->flags, FLAGS_HTTP, (d_int(token) ? true : false));
    } else if (token->key == key("maxBlockCache")) {
      EXPECT_TOK_U32(token);
      c->max_block_cache = d_long(token);
    } else if (token->key == key("maxCodeCache")) {
      EXPECT_TOK_U32(token);
      c->max_code_cache = d_long(token);
    } else if (token->key == key("key")) {
      EXPECT_TOK_B256(token);
      memcpy(c->key = _calloc(32, 1), token->data, token->len);
    } else if (token->key == key("maxVerifiedHashes")) {
      EXPECT_TOK_U16(token);
      in3_chain_t* chain = in3_find_chain(c, c->chain_id);
      if (c->max_verified_hashes < d_long(token)) {
        chain->verified_hashes = _realloc(chain->verified_hashes,
                                          sizeof(in3_verified_hash_t) * d_long(token),
                                          sizeof(in3_verified_hash_t) * c->max_verified_hashes);
        // clear newly allocated memory
        memset(chain->verified_hashes + c->max_verified_hashes, 0, (d_long(token) - c->max_verified_hashes) * sizeof(in3_verified_hash_t));
      }
      c->max_verified_hashes = d_long(token);
    } else if (token->key == key("timeout")) {
      EXPECT_TOK_U32(token);
      c->timeout = d_long(token);
    } else if (token->key == key("minDeposit")) {
      EXPECT_TOK_U64(token);
      c->min_deposit = d_long(token);
    } else if (token->key == key("nodeProps")) {
      EXPECT_TOK_U64(token);
      c->node_props = d_long(token);
    } else if (token->key == key("nodeLimit")) {
      EXPECT_TOK_U16(token);
      c->node_limit = (uint16_t) d_int(token);
    } else if (token->key == key("proof")) {
      EXPECT_TOK_STR(token);
      EXPECT_TOK(token, !strcmp(d_string(token), "full") || !strcmp(d_string(token), "standard") || !strcmp(d_string(token), "none"), "expected values - full/standard/none");
      c->proof = strcmp(d_string(token), "full") == 0
                     ? PROOF_FULL
                     : (strcmp(d_string(token), "standard") == 0 ? PROOF_STANDARD : PROOF_NONE);
    } else if (token->key == key("replaceLatestBlock")) {
      EXPECT_TOK_U8(token);
      c->replace_latest_block = (uint8_t) d_int(token);
      in3_node_props_set(&c->node_props, NODE_PROP_MIN_BLOCK_HEIGHT, d_int(token));
    } else if (token->key == key("requestCount")) {
      EXPECT_TOK_U8(token);
      c->request_count = (uint8_t) d_int(token);
    } else if (token->key == key("rpc")) {
      EXPECT_TOK_STR(token);
      c->proof           = PROOF_NONE;
      c->chain_id        = ETH_CHAIN_ID_LOCAL;
      c->request_count   = 1;
      in3_chain_t* chain = in3_find_chain(c, c->chain_id);
      in3_node_t*  n     = &chain->nodelist[0];
      if (n->url) _free(n->url);
      n->url = _malloc(d_len(token) + 1);
      if (!n->url) {
        res = config_err("in3_configure", "OOM");
        goto cleanup;
      }
      strcpy(n->url, d_string(token));
      _free(chain->nodelist_upd8_params);
      chain->nodelist_upd8_params = NULL;
    } else if (token->key == key("servers") || token->key == key("nodes")) {
      EXPECT_TOK_OBJ(token);
      for (d_iterator_t ct = d_iter(token); ct.left; d_iter_next(&ct)) {
        EXPECT_TOK_OBJ(ct.token);
        EXPECT_TOK_KEY_HEXSTR(ct.token);

        // register chain
        chain_id_t   chain_id    = char_to_long(d_get_keystr(ct.token->key), -1);
        bytes_t*     contract    = d_get_byteskl(ct.token, key("contract"), 20);
        bytes_t*     registry_id = d_get_byteskl(ct.token, key("registryId"), 32);
        bytes_t*     wl_contract = d_get_byteskl(ct.token, key("whiteListContract"), 20);
        in3_chain_t* chain       = in3_find_chain(c, chain_id);

        if (!chain) {
          EXPECT_CFG(contract && registry_id, "invalid contract/registry!");
          EXPECT_CFG((in3_client_register_chain(c, chain_id, chain ? chain->type : CHAIN_ETH, contract ? contract->data : chain->contract->data, registry_id ? registry_id->data : chain->registry_id, 2, wl_contract ? wl_contract->data : NULL)) == IN3_OK,
                     "register chain failed");
          chain = in3_find_chain(c, chain_id);
          EXPECT_CFG(chain != NULL, "invalid chain id!");
        }

        // chain_props
        bool has_wlc = false, has_man_wl = false;
        for (d_iterator_t cp = d_iter(ct.token); cp.left; d_iter_next(&cp)) {
          if (cp.token->key == key("contract")) {
            EXPECT_TOK_ADDR(cp.token);
            memcpy(chain->contract->data, cp.token->data, cp.token->len);
          } else if (cp.token->key == key("whiteListContract")) {
            EXPECT_TOK_ADDR(cp.token);
            EXPECT_CFG(!has_man_wl, "cannot specify manual whiteList and whiteListContract together!");
            has_wlc = true;
            whitelist_free(chain->whitelist);
            chain->whitelist               = _calloc(1, sizeof(in3_whitelist_t));
            chain->whitelist->needs_update = true;
            memcpy(chain->whitelist->contract, cp.token->data, 20);
          } else if (cp.token->key == key("whiteList")) {
            EXPECT_TOK_ARR(cp.token);
            EXPECT_CFG(!has_wlc, "cannot specify manual whiteList and whiteListContract together!");
            has_man_wl = true;
            int len = d_len(cp.token), i = 0;
            whitelist_free(chain->whitelist);
            chain->whitelist            = _calloc(1, sizeof(in3_whitelist_t));
            chain->whitelist->addresses = bytes(_calloc(1, len * 20), len * 20);
            for (d_iterator_t n = d_iter(cp.token); n.left; d_iter_next(&n), i += 20) {
              EXPECT_TOK_ADDR(n.token);
              const uint8_t* whitelist_address = d_bytes(n.token)->data;
              for (uint32_t j = 0; j < chain->whitelist->addresses.len; j += 20) {
                if (!memcmp(whitelist_address, chain->whitelist->addresses.data + j, 20)) {
                  whitelist_free(chain->whitelist);
                  chain->whitelist = NULL;
                  EXPECT_TOK(cp.token, false, "duplicate address!");
                }
              }
              d_bytes_to(n.token, chain->whitelist->addresses.data + i, 20);
            }
          } else if (cp.token->key == key("registryId")) {
            EXPECT_TOK_B256(cp.token);
            bytes_t data = d_to_bytes(cp.token);
            memcpy(chain->registry_id, data.data, 32);
          } else if (cp.token->key == key("needsUpdate")) {
            EXPECT_TOK_BOOL(cp.token);
            if (!d_int(cp.token)) {
              if (chain->nodelist_upd8_params) {
                _free(chain->nodelist_upd8_params);
                chain->nodelist_upd8_params = NULL;
              }
            } else if (!chain->nodelist_upd8_params)
              chain->nodelist_upd8_params = _calloc(1, sizeof(*(chain->nodelist_upd8_params)));
          } else if (cp.token->key == key("avgBlockTime")) {
            EXPECT_TOK_U16(cp.token);
            chain->avg_block_time = (uint16_t) d_int(cp.token);
          } else if (cp.token->key == key("verifiedHashes")) {
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
          } else if (cp.token->key == key("nodeList")) {
            EXPECT_TOK_ARR(cp.token);
            if (in3_client_clear_nodes(c, chain_id) < 0) goto cleanup;
            int i = 0;
            for (d_iterator_t n = d_iter(cp.token); n.left; d_iter_next(&n), i++) {
              EXPECT_CFG(d_get(n.token, key("url")) && d_get(n.token, key("address")), "expected URL & address");
              EXPECT_TOK_STR(d_get(n.token, key("url")));
              EXPECT_TOK_ADDR(d_get(n.token, key("address")));
              EXPECT_CFG(in3_client_add_node(c, chain_id, d_get_string(n.token, "url"),
                                             d_get_longkd(n.token, key("props"), 65535),
                                             d_get_byteskl(n.token, key("address"), 20)->data) == IN3_OK,
                         "add node failed");
#ifndef __clang_analyzer__
              BIT_SET(chain->nodelist[i].attrs, ATTR_BOOT_NODE);
#endif
            }
          } else {
            EXPECT_TOK(cp.token, false, "unsupported config option!");
          }
        }
        in3_client_run_chain_whitelisting(chain);
      }
    } else {
      EXPECT_TOK(token, false, "unsupported config option!");
    }
  }

  if (c->signature_count && c->chain_id != ETH_CHAIN_ID_LOCAL && !c->replace_latest_block) {
    in3_log_warn("signatureCount > 0 without replaceLatestBlock is bound to fail; using default (" STR(DEF_REPL_LATEST_BLK) ")\n");
    c->replace_latest_block = DEF_REPL_LATEST_BLK;
  }

  EXPECT_CFG(in3_find_chain(c, c->chain_id), "chain corresponding to chain id not initialized!");

cleanup:
  json_free(cnf);
  return res;
}
