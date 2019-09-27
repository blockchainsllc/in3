/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2019 slock.it GmbH, Blockchains LLC
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

#include "../util/data.h"
#include "../util/log.h"
#include "../util/mem.h"
#include "cache.h"
#include "client.h"
#include "nodelist.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define KOVAN_SPEC parse_json("[{\"block\":0,\"engine\":\"authorityRound\",\"list\":[\"0x00D6Cc1BA9cf89BD2e58009741f4F7325BAdc0ED\",\"0x00427feae2419c15b89d1c21af10d1b6650a4d3d\",\"0x4Ed9B08e6354C70fE6F8CB0411b0d3246b424d6c\",\"0x0020ee4Be0e2027d76603cB751eE069519bA81A1\",\"0x0010f94b296a852aaac52ea6c5ac72e03afd032d\",\"0x007733a1FE69CF3f2CF989F81C7b4cAc1693387A\",\"0x00E6d2b931F55a3f1701c7389d592a7778897879\",\"0x00e4a10650e5a6D6001C38ff8E64F97016a1645c\",\"0x00a0a24b9f0e5ec7aa4c7389b8302fd0123194de\"]},{\"block\":10960440,\"engine\":\"authorityRound\",\"list\":[\"0x00D6Cc1BA9cf89BD2e58009741f4F7325BAdc0ED\",\"0x0010f94b296a852aaac52ea6c5ac72e03afd032d\",\"0x00a0a24b9f0e5ec7aa4c7389b8302fd0123194de\"]},{\"block\":10960500,\"engine\":\"authorityRound\",\"contract\":\"0xaE71807C1B0a093cB1547b682DC78316D945c9B8\",\"list\":[\"0xfaadface3fbd81ce37b0e19c0b65ff4234148132\",\"0x596e8221a30bfe6e7eff67fee664a01c73ba3c56\",\"0xa4df255ecf08bbf2c28055c65225c9a9847abd94\",\"0x03801efb0efe2a25ede5dd3a003ae880c0292e4d\",\"0xd05f7478c6aa10781258c5cc8b4f385fc8fa989c\"],\"requiresFinality\":true,\"bypassFinality\":10960502}]")
#define TOBALABA_SPEC parse_json("[{\"block\":0,\"engine\":\"authorityRound\",\"validatorContract\":\"0x1000000000000000000000000000000000000005\",\"list\":[\"0x4ba15b56452521c0826a35a6f2022e1210fc519b\"]}]")

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

static void initChain(in3_chain_t* chain, uint64_t chainId, char* contract, char* registry_id, uint8_t version, int boot_node_count, in3_chain_type_t type, json_ctx_t* spec) {
  chain->chainId        = chainId;
  chain->initAddresses  = NULL;
  chain->lastBlock      = 0;
  chain->contract       = hex2byte_new_bytes(contract, 40);
  chain->needsUpdate    = chainId == 0xffff ? 0 : 1;
  chain->nodeList       = _malloc(sizeof(in3_node_t) * boot_node_count);
  chain->nodeListLength = boot_node_count;
  chain->weights        = _malloc(sizeof(in3_node_weight_t) * boot_node_count);
  chain->type           = type;
  chain->spec           = spec;
  chain->version        = version;
  memset(chain->registry_id, 0, 32);
  if (version > 1) {
    int l = hex2byte_arr(registry_id, -1, chain->registry_id, 32);
    if (l < 32) {
      memmove(chain->registry_id + 32 - l, chain->registry_id, l);
      memset(chain->registry_id, 0, 32 - l);
    }
  }
}

static void initNode(in3_chain_t* chain, int node_index, char* address, char* url) {
  in3_node_t* node = chain->nodeList + node_index;
  node->address    = hex2byte_new_bytes(address, 40);
  node->index      = node_index;
  node->capacity   = 1;
  node->deposit    = 0;
  node->props      = chain->chainId == 0xFFFF ? 0x0 : 0xFF;
  node->url        = _malloc(strlen(url) + 1);
  memcpy(node->url, url, strlen(url) + 1);

  in3_node_weight_t* weight   = chain->weights + node_index;
  weight->blacklistedUntil    = 0;
  weight->response_count      = 0;
  weight->total_response_time = 0;
  weight->weight              = 1;
}

static void in3_client_init(in3_t* c) {
  c->autoUpdateList     = 1;
  c->cacheStorage       = NULL;
  c->signer             = NULL;
  c->cacheTimeout       = 0;
  c->use_binary         = 0;
  c->use_http           = 0;
  c->includeCode        = 0;
  c->chainId            = 0x01; // mainnet
  c->key                = NULL;
  c->finality           = 0;
  c->max_attempts       = 3;
  c->maxBlockCache      = 0;
  c->maxCodeCache       = 0;
  c->minDeposit         = 0;
  c->nodeLimit          = 0;
  c->proof              = PROOF_STANDARD;
  c->replaceLatestBlock = 0;
  c->requestCount       = 1;
  c->chainsCount        = 5;
  c->chains             = _malloc(sizeof(in3_chain_t) * c->chainsCount);
  c->filters            = NULL;

  // mainnet
  initChain(c->chains, 0x01, "40f1929b349107a65e705cdbe13c496840e12d51", "48667ae295b704e285fdbb4ad61e02af6040a1bc73bca8db450864fc44c549f1", 2, 2, CHAIN_ETH, NULL);
  initNode(c->chains, 0, "45d45e6ff99e6c34a235d263965910298985fcfe", "https://in3-v2.slock.it/mainnet/nd-1");
  initNode(c->chains, 1, "1fe2e9bf29aa1938859af64c413361227d04059a", "https://in3-v2.slock.it/mainnet/nd-2");

#ifdef IN3_STAGING
  // kovan
  initChain(c->chains + 1, 0x2a, "0604014f2a5fdfafce3f2ec10c77c31d8e15ce6f", "d440f01322c8529892c204d3705ae871c514bafbb2f35907832a07322e0dc868", 2, 2, CHAIN_ETH, NULL);
  initNode(c->chains + 1, 0, "784bfa9eb182c3a02dbeb5285e3dba92d717e07a", "https://in3.stage.slock.it/kovan/nd-1");
  initNode(c->chains + 1, 1, "17cdf9ec6dcae05c5686265638647e54b14b41a2", "https://in3.stage.slock.it/kovan/nd-2");
#else
  // kovan
  initChain(c->chains + 1, 0x2a, "102561587b165a9cb4cbb41d4f65981e66a86aed", "8fcf242e4ac27a3bc35ba07c1d3ca460ce863ac9bcf8349083c44810278caff9", 2, 2, CHAIN_ETH, NULL);
  initNode(c->chains + 1, 0, "45d45e6ff99e6c34a235d263965910298985fcfe", "https://in3-v2.slock.it/kovan/nd-1");
  initNode(c->chains + 1, 1, "1fe2e9bf29aa1938859af64c413361227d04059a", "https://in3-v2.slock.it/kovan/nd-2");
#endif

  // ipfs
  initChain(c->chains + 2, 0x7d0, "f0fb87f4757c77ea3416afe87f36acaa0496c7e9", NULL, 1, 2, CHAIN_IPFS, NULL);
  initNode(c->chains + 2, 0, "784bfa9eb182c3a02dbeb5285e3dba92d717e07a", "https://in3.slock.it/ipfs/nd-1");
  initNode(c->chains + 2, 1, "243D5BB48A47bEd0F6A89B61E4660540E856A33D", "https://in3.slock.it/ipfs/nd-5");

  // local
  initChain(c->chains + 3, 0xFFFF, "f0fb87f4757c77ea3416afe87f36acaa0496c7e9", NULL, 1, 1, CHAIN_ETH, NULL);
  initNode(c->chains + 3, 0, "784bfa9eb182c3a02dbeb5285e3dba92d717e07a", "http://localhost:8545");

#ifdef IN3_STAGING
  // goerli
  initChain(c->chains + 4, 0x05, "d7a42d93eab96fabb9a481ea36fa2f72df8741cb", "19d65866bf52970ec1679c0d70d9ffd75358f78db8235e38063b1b08e74a055f", 2, 2, CHAIN_ETH, NULL);
  initNode(c->chains + 4, 0, "784bfa9eb182c3a02dbeb5285e3dba92d717e07a", "https://in3.stage.slock.it/goerli/nd-1");
  initNode(c->chains + 4, 1, "17cdf9ec6dcae05c5686265638647e54b14b41a2", "https://in3.stage.slock.it/goerli/nd-2");
#else
  // goerli
  initChain(c->chains + 4, 0x05, "c5fb30ebfabc3171c0f6406d3fbacdc78dea102c", "fd9cfed117a4db071756bae3878597b3c16b189a8bdb8ce3f35846e691631f8d", 2, 2, CHAIN_ETH, NULL);
  initNode(c->chains + 4, 0, "45d45e6ff99e6c34a235d263965910298985fcfe", "https://in3-v2.slock.it/goerli/nd-1");
  initNode(c->chains + 4, 1, "1fe2e9bf29aa1938859af64c413361227d04059a", "https://in3-v2.slock.it/foerli/nd-2");
#endif
}

static in3_chain_t* find_chain(in3_t* c, uint64_t chain_id) {
  for (int i = 0; i < c->chainsCount; i++) {
    if (c->chains[i].chainId == chain_id) return &c->chains[i];
  }
  return NULL;
}

in3_ret_t in3_client_register_chain(in3_t* c, uint64_t chain_id, in3_chain_type_t type, address_t contract, bytes32_t registry_id, uint8_t version, json_ctx_t* spec) {
  in3_chain_t* chain = find_chain(c, chain_id);
  if (!chain) {
    c->chains = _realloc(c->chains, sizeof(in3_chain_t) * (c->chainsCount + 1), sizeof(in3_chain_t) * c->chainsCount);
    if (c->chains == NULL) return IN3_ENOMEM;
    chain                 = c->chains + c->chainsCount;
    chain->nodeList       = NULL;
    chain->nodeListLength = 0;
    chain->weights        = NULL;
    chain->initAddresses  = NULL;
    chain->lastBlock      = 0;
    c->chainsCount++;

  } else if (chain->contract)
    b_free(chain->contract);

  chain->chainId     = chain_id;
  chain->contract    = b_new((char*) contract, 20);
  chain->needsUpdate = 0;
  chain->type        = type;
  chain->spec        = spec;
  chain->version     = version;
  memcpy(chain->registry_id, registry_id, 32);
  return chain->contract ? IN3_OK : IN3_ENOMEM;
}

in3_ret_t in3_client_add_node(in3_t* c, uint64_t chain_id, char* url, uint64_t props, address_t address) {
  in3_chain_t* chain = find_chain(c, chain_id);
  if (!chain) return IN3_EFIND;
  in3_node_t* node       = NULL;
  int         node_index = chain->nodeListLength;
  for (int i = 0; i < chain->nodeListLength; i++) {
    if (memcmp(chain->nodeList[i].address->data, address, 20) == 0) {
      node       = chain->nodeList + i;
      node_index = i;
      break;
    }
  }
  if (!node) {
    chain->nodeList = chain->nodeList
                          ? _realloc(chain->nodeList, sizeof(in3_node_t) * (chain->nodeListLength + 1), sizeof(in3_node_t) * chain->nodeListLength)
                          : _calloc(chain->nodeListLength + 1, sizeof(in3_node_t));
    chain->weights = chain->weights
                         ? _realloc(chain->weights, sizeof(in3_node_weight_t) * (chain->nodeListLength + 1), sizeof(in3_node_weight_t) * chain->nodeListLength)
                         : _calloc(chain->nodeListLength + 1, sizeof(in3_node_weight_t));
    if (!chain->nodeList || !chain->weights) return IN3_ENOMEM;
    node           = chain->nodeList + chain->nodeListLength;
    node->address  = b_new((char*) address, 20);
    node->index    = chain->nodeListLength;
    node->capacity = 1;
    node->deposit  = 0;
    chain->nodeListLength++;
  } else
    _free(node->url);

  node->props = props;
  node->url   = _malloc(strlen(url) + 1);
  memcpy(node->url, url, strlen(url) + 1);

  in3_node_weight_t* weight   = chain->weights + node_index;
  weight->blacklistedUntil    = 0;
  weight->response_count      = 0;
  weight->total_response_time = 0;
  weight->weight              = 1;
  return IN3_OK;
}
in3_ret_t in3_client_remove_node(in3_t* c, uint64_t chain_id, address_t address) {
  in3_chain_t* chain = find_chain(c, chain_id);
  if (!chain) return IN3_EFIND;
  int node_index = -1;
  for (int i = 0; i < chain->nodeListLength; i++) {
    if (memcmp(chain->nodeList[i].address->data, address, 20) == 0) {
      node_index = i;
      break;
    }
  }
  if (node_index == -1) return IN3_EFIND;
  if (chain->nodeList[node_index].url)
    _free(chain->nodeList[node_index].url);
  if (chain->nodeList[node_index].address)
    b_free(chain->nodeList[node_index].address);

  if (node_index < chain->nodeListLength - 1) {
    memmove(chain->nodeList + node_index, chain->nodeList + node_index + 1, sizeof(in3_node_t) * (chain->nodeListLength - 1 - node_index));
    memmove(chain->weights + node_index, chain->weights + node_index + 1, sizeof(in3_node_weight_t) * (chain->nodeListLength - 1 - node_index));
  }
  chain->nodeListLength--;
  if (!chain->nodeListLength) {
    _free(chain->nodeList);
    _free(chain->weights);
    chain->nodeList = NULL;
    chain->weights  = NULL;
  }
  return IN3_OK;
}
in3_ret_t in3_client_clear_nodes(in3_t* c, uint64_t chain_id) {
  in3_chain_t* chain = find_chain(c, chain_id);
  if (!chain) return IN3_EFIND;
  in3_nodelist_clear(chain);
  chain->nodeList       = NULL;
  chain->weights        = NULL;
  chain->nodeListLength = 0;
  return IN3_OK;
}

/* frees the data */
void in3_free(in3_t* a) {
  int i;
  for (i = 0; i < a->chainsCount; i++) {
    in3_nodelist_clear(a->chains + i);
    b_free(a->chains[i].contract);
    free_json(a->chains[i].spec);
  }
  if (a->signer) _free(a->signer);
  _free(a->chains);

  if (a->filters != NULL) {
    in3_filter_t* f = NULL;
    for (size_t j = 0; j < a->filters->count; j++) {
      f = a->filters->array[j];
      if (f) f->release(f);
    }
    _free(a->filters->array);
    _free(a->filters);
  }
  _free(a);
}

in3_t* in3_new() {
  // initialize random with the timestamp as seed
  _srand(_time());

  // create new client
  in3_t* c = _calloc(1, sizeof(in3_t));
  in3_client_init(c);

  if (default_transport) c->transport = default_transport;
  if (default_storage) c->cacheStorage = default_storage;
  if (default_signer) c->signer = default_signer;

#ifndef TEST
  in3_log_set_quiet(1);
#endif
  return c;
}

in3_ret_t in3_configure(in3_t* c, char* config) {
  d_track_keynames(1);
  d_clear_keynames();
  json_ctx_t* cnf = parse_json(config);
  d_track_keynames(0);
  in3_ret_t res = IN3_OK;

  if (!cnf || !cnf->result) return IN3_EINVAL;
  for (d_iterator_t iter = d_iter(cnf->result); iter.left; d_iter_next(&iter)) {
    if (iter.token->key == key("autoUpdateList"))
      c->autoUpdateList = d_int(iter.token) ? true : false;
    else if (iter.token->key == key("chainId"))
      c->chainId = d_long(iter.token);
    else if (iter.token->key == key("finality"))
      c->finality = (uint_fast16_t) d_int(iter.token);
    else if (iter.token->key == key("includeCode"))
      c->includeCode = d_int(iter.token) ? true : false;
    else if (iter.token->key == key("maxAttempts"))
      c->max_attempts = d_int(iter.token);
    else if (iter.token->key == key("maxBlockCache"))
      c->maxBlockCache = d_int(iter.token);
    else if (iter.token->key == key("maxCodeCache"))
      c->maxCodeCache = d_int(iter.token);
    else if (iter.token->key == key("minDeposit"))
      c->minDeposit = d_long(iter.token);
    else if (iter.token->key == key("nodeLimit"))
      c->nodeLimit = (uint16_t) d_int(iter.token);
    else if (iter.token->key == key("proof"))
      c->proof = strcmp(d_string(iter.token), "full") == 0
                     ? PROOF_FULL
                     : (strcmp(d_string(iter.token), "standard") == 0 ? PROOF_STANDARD : PROOF_NONE);
    else if (iter.token->key == key("replaceLatestBlock"))
      c->replaceLatestBlock = (uint16_t) d_int(iter.token);
    else if (iter.token->key == key("requestCount"))
      c->requestCount = (uint8_t) d_int(iter.token);
    else if (iter.token->key == key("rpc")) {
      c->proof        = PROOF_NONE;
      c->chainId      = 0xFFFF;
      c->requestCount = 1;
      in3_node_t* n   = find_chain(c, c->chainId)->nodeList;
      if (n->url) _free(n);
      n->url = malloc(d_len(iter.token) + 1);
      if (!n->url) {
        res = IN3_ENOMEM;
        goto cleanup;
      }
      strcpy(n->url, d_string(iter.token));
    } else if (iter.token->key == key("servers") || iter.token->key == key("nodes"))
      for (d_iterator_t ct = d_iter(iter.token); ct.left; d_iter_next(&ct)) {
        // register chain
        uint64_t     chain_id = hex2long(d_get_keystr(ct.token->key));
        in3_chain_t* chain    = find_chain(c, chain_id);
        if (!chain) {
          bytes_t* contract_t  = d_get_byteskl(ct.token, key("contract"), 20);
          bytes_t* registry_id = d_get_byteskl(ct.token, key("regiistryId"), 32);
          if (!contract_t || !registry_id) {
            res = IN3_EINVAL;
            goto cleanup;
          }
          if ((res = in3_client_register_chain(c, chain_id, CHAIN_ETH, contract_t->data, registry_id->data, 2, NULL)) != IN3_OK) goto cleanup;
        }

        // chain_props
        for (d_iterator_t cp = d_iter(ct.token); cp.left; d_iter_next(&cp)) {
          if (cp.token->key == key("contract"))
            memcpy(chain->contract->data, cp.token->data, cp.token->len);
          else if (cp.token->key == key("registryId"))
            memcpy(chain->registry_id, cp.token->data, cp.token->len);
          else if (cp.token->key == key("needsUpdate"))
            chain->needsUpdate = d_int(cp.token) ? true : false;
          else if (cp.token->key == key("nodeList")) {
            if (in3_client_clear_nodes(c, chain_id) < 0) goto cleanup;
            for (d_iterator_t n = d_iter(cp.token); n.left; d_iter_next(&n)) {
              if ((res = in3_client_add_node(c, chain_id, d_get_string(n.token, "url"),
                                             d_get_longkd(n.token, key("props"), 65535),
                                             d_get_byteskl(n.token, key("address"), 20)->data)) != IN3_OK) goto cleanup;
            }
          }
        }
      }
  }

cleanup:
  free_json(cnf);
  return res;
}
