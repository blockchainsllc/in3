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

static in3_ret_t test_transport(char** urls, int urls_len, char* payload, in3_response_t* result) {
  char* buffer = NULL;
  long  length;
  FILE* f = fopen("../test/testdata/requests/in3_nodeList.json", "r");
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
    printf("Error coudl not find the testdata 'test/testdata/requests/in3_nodeList.json'\n");
    exit(1);
  }

  // now parse the json
  json_ctx_t* res  = parse_json(buffer);
  str_range_t json = d_to_json(d_get_at(d_get(d_get_at(res->result, 0), key("response")), 0));
  sb_add_range(&result->result, json.data, 0, json.len);
  free_json(res);
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
  while (i < MAX_ENTRIES && cache->keys[i]) i++;
  cache->keys[i]   = strdup(key);
  cache->values[i] = bytes(malloc(value->len), value->len);
  memcpy(cache->values[i].data, value->data, value->len);
}

static void test_cache() {

  in3_register_eth_nano();
  cache_t* cache = calloc(1, sizeof(cache_t));

  in3_t* c                  = in3_new();
  c->transport              = test_transport;
  c->cacheStorage           = _malloc(sizeof(in3_storage_handler_t));
  c->cacheStorage->cptr     = cache;
  c->cacheStorage->get_item = cache_get_item;
  c->cacheStorage->set_item = cache_set_item;
  c->chainId                = 0x1;

  in3_chain_t* chain = NULL;
  for (int i = 0; i < c->chainsCount; i++) {
    if (c->chains[i].chainId == 0x1) chain = &c->chains[i];
  }

  TEST_ASSERT_TRUE(chain != NULL);
  TEST_ASSERT_EQUAL_INT32(2, chain->nodeListLength);

  // cache is empty so no changes
  in3_cache_init(c);
  TEST_ASSERT_EQUAL_INT32(2, chain->nodeListLength);

  // now we update the node (from testfiles)
  TEST_ASSERT_EQUAL(0, update_nodes(c, chain));

  // the nodeList should have 5 nodes now
  TEST_ASSERT_EQUAL_INT32(5, chain->nodeListLength);
  // ..and the cache one entry
  TEST_ASSERT_TRUE(*cache->keys != NULL);

  // create a second client...
  in3_t* c2        = in3_new();
  c2->cacheStorage = c->cacheStorage;
  c2->transport    = test_transport;
  c2->chainId      = c->chainId;
  in3_configure(c2, "{\"chainId\":\"0x1\"}");
  in3_chain_t* chain2 = NULL;
  for (int i = 0; i < c2->chainsCount; i++) {
    if (c2->chains[i].chainId == 0x1) chain2 = &c2->chains[i];
  }

  // the nodeList should have 2 nodes still
  TEST_ASSERT_EQUAL_INT32(2, chain2->nodeListLength);
  in3_cache_init(c2);
  // the nodeList should have 5 nodes now
  TEST_ASSERT_EQUAL_INT32(5, chain2->nodeListLength);

  // test request
  in3_ctx_t* ctx = in3_client_rpc_ctx(c2, "in3_nodeList", "[]");
  if (ctx->error) printf("ERROR : %s\n", ctx->error);
  TEST_ASSERT(ctx && ctx->error == NULL);
  free_ctx(ctx);
}

static void test_newchain() {

  in3_register_eth_nano();
  in3_set_default_transport(test_transport);

  cache_t* cache = _calloc(1, sizeof(cache_t));

  in3_t* c                  = in3_new();
  c->cacheStorage           = _malloc(sizeof(in3_storage_handler_t));
  c->cacheStorage->cptr     = cache;
  c->cacheStorage->get_item = cache_get_item;
  c->cacheStorage->set_item = cache_set_item;
  c->chainId                = 0x8;

  in3_set_default_storage(c->cacheStorage);

  in3_chain_t* chain = NULL;
  for (int i = 0; i < c->chainsCount; i++) {
    if (c->chains[i].chainId == 0x8) chain = &c->chains[i];
  }

  TEST_ASSERT_TRUE(chain == NULL);
  address_t contract;
  hex2byte_arr("0x64abe24afbba64cae47e3dc3ced0fcab95e4edd5", -1, contract, 20);
  bytes32_t registry_id;
  hex2byte_arr("0x423dd84f33a44f60e5d58090dcdcc1c047f57be895415822f211b8cd1fd692e3", -1, registry_id, 32);
  in3_client_register_chain(c, 0x8, CHAIN_ETH, contract, registry_id, 2, NULL);
  in3_client_add_node(c, 0x8, "http://test.com", 0xFF, contract);

  for (int i = 0; i < c->chainsCount; i++) {
    if (c->chains[i].chainId == 0x8) chain = &c->chains[i];
  }

  TEST_ASSERT_TRUE(chain != NULL);
  TEST_ASSERT_EQUAL(1, chain->nodeListLength);

  // cache is empty so no changes
  in3_cache_init(c);
  TEST_ASSERT_EQUAL_INT32(1, chain->nodeListLength);

  // now we update the node (from testfiles)
  TEST_ASSERT_EQUAL(0, update_nodes(c, chain));

  // the nodeList should have 5 nodes now
  TEST_ASSERT_EQUAL_INT32(5, chain->nodeListLength);
  // ..and the cache one entry
  TEST_ASSERT_TRUE(*cache->keys != NULL);

  // create a second client...
  in3_t* c2        = in3_new();
  c2->cacheStorage = c->cacheStorage;
  c2->chainId      = c->chainId;
  in3_client_register_chain(c2, 0x8, CHAIN_ETH, contract, registry_id, 2, NULL);
  in3_chain_t* chain2 = NULL;
  for (int i = 0; i < c2->chainsCount; i++) {
    if (c2->chains[i].chainId == c2->chainId) chain2 = &c2->chains[i];
  }

  // the nodeList should have 2 nodes still
  TEST_ASSERT_EQUAL_INT32(0, chain2->nodeListLength);
  in3_cache_init(c2);
  // the nodeList should have 5 nodes now
  TEST_ASSERT_EQUAL_INT32(5, chain2->nodeListLength);

  in3_client_remove_node(c2, c2->chainId, chain2->nodeList->address->data);
  TEST_ASSERT_EQUAL_INT32(4, chain2->nodeListLength);
  in3_client_clear_nodes(c2, c2->chainId);
  TEST_ASSERT_EQUAL_INT32(0, chain2->nodeListLength);
}

void test_scache() {
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
/*
 * Main
 */
int main() {
  TEST_ASSERT_EQUAL(0, mem_stack_size());
  memstack();
  in3_log_set_level(LOG_ERROR);

  // now run tests
  TESTS_BEGIN();
  RUN_TEST(test_scache);
  RUN_TEST(test_cache);
  RUN_TEST(test_newchain);
  return TESTS_END();
}
