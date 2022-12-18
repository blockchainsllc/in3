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

#ifndef TEST
#define TEST
#endif
#ifndef TEST
#define DEBUG
#endif

#include "../../src/nodeselect/full/rpcs.h"
#include "../../src/core/client/plugin.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/log.h"
#include "../../src/core/util/scache.h"
#include "../../src/verifier/eth1/nano/eth_nano.h"
#include "../test_utils.h"
#include "nodeselect/full/cache.h"
#include "nodeselect/full/nodelist.h"
#include <api/eth1/eth_api.h>
#include <nodeselect/full/nodeselect_def.h>
#include <stdio.h>
#include <unistd.h>

#define CONTRACT_ADDRS           "0x5f51e413581dd76759e9eed51e63d14c8d1379c8"
#define REGISTRY_ID              "0x67c02e5e272f9d6b4a33716614061dd298283f86351079ef903bf0d4410a44ea"
#define WHITELIST_CONTRACT_ADDRS "0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab"

static in3_ret_t test_transport(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  in3_http_request_t* req    = plugin_ctx;
  char*               buffer = NULL;
  long                length;
  FILE*               f = fopen("../test/testdata/requests/in3_nodeList.json", "r");
  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = _malloc(length + 1);
    fread(buffer, 1, length, f);
    buffer[length] = 0;
    fclose(f);
  }
  else {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      printf("Current working dir: %s\n", cwd);
    }
    else {
      perror("getcwd() error");
      return 1;
    }
    printf("Error coudl not find the testdata 'test/testdata/requests/in3_nodeList.json'\n");
    exit(1);
  }

  // now parse the json
  json_ctx_t* res  = parse_json(buffer);
  str_range_t json = d_to_json(d_get_at(d_get(d_get_at(res->result, 0), key("response")), 0));
  in3_ctx_add_response(req->req, 0, false, json.data, json.len, 0);
  json_free(res);
  if (buffer) _free(buffer);
  return IN3_OK;
}
#define MAX_ENTRIES 10

typedef struct cache_s {
  bytes_t values[MAX_ENTRIES];
  char*   keys[MAX_ENTRIES];
} cache_t;
static cache_t cache;

static bytes_t* cache_get_item(void* cptr, const char* key) {
  cache_t* cache = (cache_t*) cptr;
  for (int i = 0; i < MAX_ENTRIES && cache->keys[i]; i++) {
    if (strcmp(cache->keys[i], key) == 0)
      return b_dup(cache->values + i);
  }
  return NULL;
}
static void cache_set_item(void* cptr, const char* key, bytes_t* value) {
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
  if (!memiszero((uint8_t*) &cache, sizeof(cache))) {
    for (int i = 0; i < MAX_ENTRIES; ++i) {
      free(cache.keys[i]);
      free(cache.values[i].data);
    }
    memset(&cache, 0, sizeof(cache));
  }
  in3_set_storage_handler(c, cache_get_item, cache_set_item, NULL, &cache);
}

static void test_cache() {
  in3_t* c = in3_for_chain(0);
  TEST_ASSERT_NULL(in3_configure(c, "{\"chainId\":\"0x5\",\"signatureCount\":0}"));
  in3_plugin_register(c, PLGN_ACT_TRANSPORT, test_transport, NULL, true);
  setup_test_cache(c);

  in3_nodeselect_def_t* nl = in3_nodeselect_def_data(c);

  // cache is empty so no changes
  in3_cache_init(c, nl);
  TEST_ASSERT_EQUAL_INT32(2, nl->nodelist_length);

  // now we update the node (from testfiles)r
  update_nodes(c, nl);

  // the nodeList should have 5 nodes now
  TEST_ASSERT_EQUAL_INT32(7, nl->nodelist_length);
  // ..and the cache one entry
  TEST_ASSERT_TRUE(cache.keys != NULL);

  // create a second client...
  in3_t* c2 = in3_for_chain(0);
  TEST_ASSERT_NULL(in3_configure(c2, "{\"chainId\":\"0x5\",\"signatureCount\":0,\"maxAttempts\":1}"));

  c2->flags |= FLAGS_AUTO_UPDATE_LIST | FLAGS_NODE_LIST_NO_SIG;
  in3_plugin_register(c2, PLGN_ACT_TRANSPORT, test_transport, NULL, true);

  in3_nodeselect_def_t* nl2 = in3_nodeselect_def_data(c2);

  // the nodeList should have 2 nodes still
  TEST_ASSERT_EQUAL_INT32(2, nl2->nodelist_length);

  in3_set_storage_handler(c2, cache_get_item, cache_set_item, NULL, &cache);
  in3_cache_init(c2, nl2);

  // the nodeList should have 5 nodes now
  TEST_ASSERT_EQUAL_INT32(7, nl2->nodelist_length);

  // test request
  in3_req_t* ctx = in3_client_rpc_ctx(c2, FN_IN3_NODELIST, "[]");
  if (ctx->error) printf("ERROR : %s\n", ctx->error);
  TEST_ASSERT(ctx && ctx->error == NULL);
  req_free(ctx);
  in3_free(c);
  in3_free(c2);
}

static in3_ret_t in3_register_test_transport(in3_t* c) {
  return in3_plugin_register(c, PLGN_ACT_TRANSPORT, test_transport, NULL, true);
}

static void test_newchain() {

  in3_register_default(in3_register_test_transport);

  in3_t* c = in3_for_chain(0);
  TEST_ASSERT_NULL(in3_configure(c, "{\"chainId\":\"0x8\",\"signatureCount\":0,\"maxAttempts\":1}"));
  c->flags |= FLAGS_AUTO_UPDATE_LIST | FLAGS_NODE_LIST_NO_SIG;
  setup_test_cache(c);

  address_t contract;
  hex_to_bytes(CONTRACT_ADDRS, -1, contract, 20);
  bytes32_t registry_id;
  hex_to_bytes(REGISTRY_ID, -1, registry_id, 32);
  in3_client_register_chain(c, 0x8, CHAIN_ETH, 2);
  TEST_ASSERT_NULL(in3_configure(c, "{"
                                    "  \"nodeRegistry\": {"
                                    "      \"contract\":\"" CONTRACT_ADDRS "\","
                                    "      \"registryId\":\"" REGISTRY_ID "\","
                                    "      \"nodeList\": [{"
                                    "       \"address\": \"" CONTRACT_ADDRS "\","
                                    "       \"url\": \"http://test.com\","
                                    "       \"props\": \"0xFF\""
                                    "      }]"
                                    "  }"
                                    "}"));

  in3_nodeselect_def_t* nl = in3_nodeselect_def_data(c);
  TEST_ASSERT_EQUAL(1, nl->nodelist_length);

  // cache is empty so no changes
  in3_cache_init(c, nl);
  TEST_ASSERT_EQUAL_INT32(1, nl->nodelist_length);

  // now we update the node (from testfiles)
  TEST_ASSERT_EQUAL(0, update_nodes(c, nl));

  // the nodeList should have 7 nodes now
  TEST_ASSERT_EQUAL_INT32(7, nl->nodelist_length);
  // ..and the cache one entry
  TEST_ASSERT_TRUE(cache.keys != NULL);

  // create a second client...
  in3_t* c2 = in3_for_chain(0);
  TEST_ASSERT_NULL(in3_configure(c2, "{\"chainId\":\"0x8\",\"signatureCount\":0,\"maxAttempts\":1}"));
  in3_set_storage_handler(c2, cache_get_item, cache_set_item, NULL, &cache);

  in3_client_register_chain(c2, 0x8, CHAIN_ETH, 2);
  TEST_ASSERT_NULL(in3_configure(c2, "{"
                                     "  \"nodeRegistry\": {"
                                     "      \"contract\":\"" CONTRACT_ADDRS "\","
                                     "      \"registryId\":\"" REGISTRY_ID "\""
                                     "  }"
                                     "}"));

  in3_nodeselect_def_t* nl2 = in3_nodeselect_def_data(c2);

  // the nodeList should have 2 nodes still
  TEST_ASSERT_EQUAL_INT32(0, nl2->nodelist_length);
  in3_cache_init(c2, nl2);
  // the nodeList should have 7 nodes now
  TEST_ASSERT_EQUAL_INT32(7, nl2->nodelist_length);

  // fixme: nl_sep
  //  in3_client_remove_node(c2, c2->chain_id, nl2->nodelist->address);
  //  TEST_ASSERT_EQUAL_INT32(6, nl2->nodelist_length);
  //  in3_client_clear_nodes(c2, c2->chain_id);
  //  TEST_ASSERT_EQUAL_INT32(0, nl2->nodelist_length);

  in3_free(c);
  in3_free(c2);
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
  _free(cache);
}

static void test_whitelist_cache() {
  address_t contract;
  hex_to_bytes(CONTRACT_ADDRS, -1, contract, 20);
  bytes32_t registry_id;
  hex_to_bytes(REGISTRY_ID, -1, registry_id, 32);

  in3_t* c   = in3_for_chain(0);
  char*  err = in3_configure(c, "{\"chainId\":\"0x8\",\"signatureCount\":0,\"maxAttempts\":1}");
  TEST_ASSERT_NULL(err);

  setup_test_cache(c);
  char* tmp = NULL;

  TEST_ASSERT_EQUAL_STRING("cannot specify manual whiteList and whiteListContract together!",
                           (tmp = in3_configure(c, "{"
                                                   "  \"nodeRegistry\": {"
                                                   "      \"contract\":\"" CONTRACT_ADDRS "\","
                                                   "      \"registryId\":\"" REGISTRY_ID "\","
                                                   "      \"whiteList\": [\"0x1234567890123456789012345678901234567890\", \"0x1234567890123456789000000000000000000000\"],"
                                                   "      \"whiteListContract\": \"" WHITELIST_CONTRACT_ADDRS "\""
                                                   "  }"
                                                   "}")));

  free(tmp);
  TEST_ASSERT_NULL(in3_configure(c, "{"
                                    "  \"nodeRegistry\": {"
                                    "      \"contract\":\"" CONTRACT_ADDRS "\","
                                    "      \"registryId\":\"" REGISTRY_ID "\","
                                    "      \"whiteListContract\": \"" WHITELIST_CONTRACT_ADDRS "\""
                                    "  }"
                                    "}"));

  address_t             wlc;
  in3_nodeselect_def_t* nl = in3_nodeselect_def_data(c);
  hex_to_bytes(WHITELIST_CONTRACT_ADDRS, -1, wlc, 20);
  TEST_ASSERT_EQUAL_MEMORY(nl->whitelist->contract, wlc, 20);
  in3_req_t* ctx = req_new(c, "{\"method\":\"eth_getBlockByNumber\",\"params\":[\"latest\",false]}");
  TEST_ASSERT_EQUAL(IN3_OK, in3_cache_store_whitelist(ctx->client, nl));

  // fixme: nl_sep
  //  in3_t* c2 = in3_for_chain(0);
  //  TEST_ASSERT_NULL(in3_configure(c2, "{\"chainId\":\"0x8\",\"signatureCount\":0,\"maxAttempts\":1}"));
  //  in3_nodeselect_def_t* nl2 = in3_nodeselect_def_data(c2);
  //  in3_set_storage_handler(c2, cache_get_item, cache_set_item, NULL, &cache);
  //  in3_cache_init(c2, nl2);
  //  TEST_ASSERT_EQUAL(IN3_OK, in3_cache_update_whitelist(c2, nl2));
  //  TEST_ASSERT_EQUAL_MEMORY(nl2->whitelist->contract, wlc, 20);
  //  TEST_ASSERT_TRUE(b_cmp(&nl2->whitelist->addresses, &nl2->whitelist->addresses));
  //  in3_free(c2);

  req_free(ctx);
  in3_free(c);
}

/*
 * Main
 */
int main() {
  in3_register_default(in3_register_eth_nano);
  in3_register_default(in3_register_nodeselect_def);

  in3_log_set_udata_(NULL);
  in3_log_set_lock_(NULL);
  in3_log_set_fp_(NULL);

  // now run tests
  TESTS_BEGIN();
  RUN_TEST(test_scache);
  //  RUN_TEST(test_cache);
  //  RUN_TEST(test_newchain);
  RUN_TEST(test_whitelist_cache);
  return TESTS_END();
}
