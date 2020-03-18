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

#ifndef TEST
#define TEST
#endif
#ifndef TEST
#define DEBUG
#endif

#include "../../src/core/client/cache.h"
#include "../../src/core/client/context.h"
#include "../../src/core/client/nodelist.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/log.h"
#include "../../src/core/util/scache.h"
#include "../../src/verifier/eth1/nano/eth_nano.h"
#include "../test_utils.h"
#include <stdio.h>
#include <unistd.h>

#define CONTRACT_ADDRS "0x5f51e413581dd76759e9eed51e63d14c8d1379c8"
#define REGISTRY_ID "0x67c02e5e272f9d6b4a33716614061dd298283f86351079ef903bf0d4410a44ea"
#define WHITELIST_CONTRACT_ADDRS "0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab"

static in3_ret_t test_transport(in3_request_t* req) {
  char* buffer = NULL;
  long  length;
  FILE* f = fopen("../c/test/testdata/requests/in3_nodeList.json", "r");
  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = _malloc(length + 1);
    if (buffer) {
      fread(buffer, 1, length, f);
      buffer[length] = 0;
    }
    fclose(f);
  } else {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      printf("Current working dir: %s\n", cwd);
    } else {
      perror("getcwd() error");
      return 1;
    }
    printf("Error coudl not find the testdata 'c/test/testdata/requests/in3_nodeList.json'\n");
    exit(1);
  }

  // now parse the json
  json_ctx_t* res  = parse_json(buffer);
  str_range_t json = d_to_json(d_get_at(d_get(d_get_at(res->result, 0), key("response")), 0));
  sb_add_range(&req->results->result, json.data, 0, json.len);
  json_free(res);
  return IN3_OK;
}
#define MAX_ENTRIES 10

typedef struct cache_s {
  bytes_t values[MAX_ENTRIES];
  char*   keys[MAX_ENTRIES];
} cache_t;
static bytes_t* cache_get_item(void* cptr, char* key) {
  cache_t* cache = (cache_t*) cptr;
  for (int i = 0; i < MAX_ENTRIES && cache->keys[i]; i++) {
    if (strcmp(cache->keys[i], key) == 0)
      return b_dup(cache->values + i);
  }
  return NULL;
}
static void cache_set_item(void* cptr, char* key, bytes_t* value) {
  cache_t* cache = (cache_t*) cptr;
  int      i     = 0;
  while (i < MAX_ENTRIES && cache->keys[i]) {
    if (strcmp(cache->keys[i], key) == 0) {
      _free(cache->keys[i]);
      _free(cache->values[i].data);
      break;
    }
    i++;
  }
  cache->keys[i]   = strdup(key);
  cache->values[i] = bytes(malloc(value->len), value->len);
  memcpy(cache->values[i].data, value->data, value->len);
}

void static setup_test_cache(in3_t* c) {
  cache_t* cache     = calloc(1, sizeof(cache_t));
  c->cache           = _malloc(sizeof(in3_storage_handler_t));
  c->cache->cptr     = cache;
  c->cache->get_item = cache_get_item;
  c->cache->set_item = cache_set_item;
}

static void test_cache() {
  in3_t* c     = in3_for_chain(ETH_CHAIN_ID_GOERLI);
  c->transport = test_transport;
  setup_test_cache(c);

  in3_chain_t* chain = in3_find_chain(c, ETH_CHAIN_ID_GOERLI);

  TEST_ASSERT_TRUE(chain != NULL);
  TEST_ASSERT_EQUAL_INT32(2, chain->nodelist_length);

  // cache is empty so no changes
  in3_cache_init(c);
  TEST_ASSERT_EQUAL_INT32(2, chain->nodelist_length);

  // now we update the node (from testfiles)
  TEST_ASSERT_EQUAL(0, update_nodes(c, chain));

  // the nodeList should have 5 nodes now
  TEST_ASSERT_EQUAL_INT32(7, chain->nodelist_length);
  // ..and the cache one entry
  TEST_ASSERT_TRUE(*((cache_t*) c->cache->cptr)->keys != NULL);

  // create a second client...
  in3_t* c2     = in3_for_chain(0);
  c2->cache     = c->cache;
  c2->transport = test_transport;
  c2->chain_id  = c->chain_id;
  c2->flags |= FLAGS_AUTO_UPDATE_LIST | FLAGS_NODE_LIST_NO_SIG;

  in3_configure(c2, "{\"chainId\":\"0x5\"}");
  in3_chain_t* chain2 = in3_find_chain(c2, c2->chain_id);

  // the nodeList should have 2 nodes still
  TEST_ASSERT_EQUAL_INT32(2, chain2->nodelist_length);
  in3_cache_init(c2);
  // the nodeList should have 5 nodes now
  TEST_ASSERT_EQUAL_INT32(7, chain2->nodelist_length);

  // test request
  in3_ctx_t* ctx = in3_client_rpc_ctx(c2, "in3_nodeList", "[]");
  if (ctx->error) printf("ERROR : %s\n", ctx->error);
  TEST_ASSERT(ctx && ctx->error == NULL);
  ctx_free(ctx);
}

static void test_newchain() {
  in3_set_default_transport(test_transport);

  in3_t* c    = in3_for_chain(0);
  c->chain_id = 0x8;
  c->flags |= FLAGS_AUTO_UPDATE_LIST | FLAGS_NODE_LIST_NO_SIG;
  setup_test_cache(c);

  in3_set_default_storage(c->cache);

  in3_chain_t* chain = NULL;
  for (int i = 0; i < c->chains_length; i++) {
    if (c->chains[i].chain_id == 0x8) chain = &c->chains[i];
  }

  TEST_ASSERT_TRUE(chain == NULL);
  address_t contract;
  hex_to_bytes(CONTRACT_ADDRS, -1, contract, 20);
  bytes32_t registry_id;
  hex_to_bytes(REGISTRY_ID, -1, registry_id, 32);
  in3_client_register_chain(c, 0x8, CHAIN_ETH, contract, registry_id, 2, NULL);
  in3_client_add_node(c, 0x8, "http://test.com", 0xFF, contract);

  for (int i = 0; i < c->chains_length; i++) {
    if (c->chains[i].chain_id == 0x8) chain = &c->chains[i];
  }

  TEST_ASSERT_TRUE(chain != NULL);
  TEST_ASSERT_EQUAL(1, chain->nodelist_length);

  // cache is empty so no changes
  in3_cache_init(c);
  TEST_ASSERT_EQUAL_INT32(1, chain->nodelist_length);

  // now we update the node (from testfiles)
  TEST_ASSERT_EQUAL(0, update_nodes(c, chain));

  // the nodeList should have 7 nodes now
  TEST_ASSERT_EQUAL_INT32(7, chain->nodelist_length);
  // ..and the cache one entry
  TEST_ASSERT_TRUE(*((cache_t*) c->cache->cptr)->keys != NULL);

  // create a second client...
  in3_t* c2    = in3_for_chain(0);
  c2->chain_id = c->chain_id;
  c2->cache    = c->cache;

  in3_client_register_chain(c2, 0x8, CHAIN_ETH, contract, registry_id, 2, NULL);
  in3_chain_t* chain2 = NULL;
  for (int i = 0; i < c2->chains_length; i++) {
    if (c2->chains[i].chain_id == c2->chain_id) chain2 = &c2->chains[i];
  }

  // the nodeList should have 2 nodes still
  TEST_ASSERT_EQUAL_INT32(0, chain2->nodelist_length);
  in3_cache_init(c2);
  // the nodeList should have 7 nodes now
  TEST_ASSERT_EQUAL_INT32(7, chain2->nodelist_length);

  in3_client_remove_node(c2, c2->chain_id, chain2->nodelist->address->data);
  TEST_ASSERT_EQUAL_INT32(6, chain2->nodelist_length);
  in3_client_clear_nodes(c2, c2->chain_id);
  TEST_ASSERT_EQUAL_INT32(0, chain2->nodelist_length);
}

static void test_scache() {
  char*          key   = "123";
  char*          value = "45678";
  bytes_t        k     = bytes((uint8_t*) key, 3);
  bytes_t        v     = bytes((uint8_t*) value, 3);
  cache_entry_t* cache = in3_cache_add_entry(NULL, bytes((uint8_t*) key, 3), bytes((uint8_t*) value, 5));

  bytes_t* val = in3_cache_get_entry(cache, &k);
  TEST_ASSERT_TRUE(val != NULL && val->len == 5);
  val = in3_cache_get_entry(cache, &v);
  TEST_ASSERT_NULL(val);
}

static void test_whitelist_cache() {
  address_t contract;
  hex_to_bytes(CONTRACT_ADDRS, -1, contract, 20);
  bytes32_t registry_id;
  hex_to_bytes(REGISTRY_ID, -1, registry_id, 32);

  in3_t* c    = in3_for_chain(0);
  c->chain_id = 0x8;
  setup_test_cache(c);
  in3_set_default_storage(c->cache);
  char* tmp = NULL;

  TEST_ASSERT_EQUAL_STRING("cannot specify manual whiteList and whiteListContract together!",
                           (tmp = in3_configure(c, "{"
                                                   "  \"nodes\": {"
                                                   "    \"0x7\": {"
                                                   "      \"contract\":\"" CONTRACT_ADDRS "\","
                                                   "      \"registryId\":\"" REGISTRY_ID "\","
                                                   "      \"whiteList\": [\"0x1234567890123456789012345678901234567890\", \"0x1234567890123456789000000000000000000000\"],"
                                                   "      \"whiteListContract\": \"" WHITELIST_CONTRACT_ADDRS "\""
                                                   "    }"
                                                   "  }"
                                                   "}")));

  free(tmp);
  TEST_ASSERT_NULL(in3_configure(c, "{"
                                    "  \"nodes\": {"
                                    "    \"0x8\": {"
                                    "      \"contract\":\"" CONTRACT_ADDRS "\","
                                    "      \"registryId\":\"" REGISTRY_ID "\","
                                    "      \"whiteListContract\": \"" WHITELIST_CONTRACT_ADDRS "\""
                                    "    }"
                                    "  }"
                                    "}"));

  address_t wlc;
  hex_to_bytes(WHITELIST_CONTRACT_ADDRS, -1, wlc, 20);
  TEST_ASSERT_EQUAL_MEMORY(in3_find_chain(c, 0x8)->whitelist->contract, wlc, 20);
  in3_ctx_t* ctx = ctx_new(c, "{\"method\":\"eth_getBlockByNumber\",\"params\":[\"latest\",false]}");
  TEST_ASSERT_EQUAL(IN3_OK, in3_cache_store_whitelist(ctx, in3_find_chain(c, 0x8)));

  in3_t* c2    = in3_for_chain(0);
  c2->chain_id = c->chain_id;
  c2->cache    = c->cache;
  in3_client_register_chain(c2, 0x8, CHAIN_ETH, contract, registry_id, 2, wlc);
  TEST_ASSERT_EQUAL(IN3_OK, in3_cache_update_whitelist(c2, in3_find_chain(c2, 0x8)));
  TEST_ASSERT_EQUAL_MEMORY(in3_find_chain(c2, 0x8)->whitelist->contract, wlc, 20);
  TEST_ASSERT_TRUE(b_cmp(&in3_find_chain(c, 0x8)->whitelist->addresses, &in3_find_chain(c2, 0x8)->whitelist->addresses));
  in3_free(c2);
  ctx_free(ctx);
  in3_free(c);
}

/*
 * Main
 */
int main() {
  TEST_ASSERT_EQUAL(0, mem_stack_size());
  memstack();
  in3_register_eth_nano();
  in3_log_set_udata_(NULL);
  in3_log_set_lock_(NULL);
  in3_log_set_fp_(NULL);
  in3_log_set_quiet_(false);
  in3_log_set_level(LOG_ERROR);

  // now run tests
  TESTS_BEGIN();
  RUN_TEST(test_scache);
  RUN_TEST(test_cache);
  RUN_TEST(test_newchain);
  RUN_TEST(test_whitelist_cache);
  return TESTS_END();
}
