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

#include "../../src/api/eth1/abi.h"
#include "../../src/api/eth1/eth_api.h"
#include "../../src/core/client/context_internal.h"
#include "../../src/core/client/keys.h"
#include "../../src/core/util/bitset.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/log.h"
#include "../../src/verifier/eth1/full/eth_full.h"
#include "../test_utils.h"
#include "../util/transport.h"
#include <stdio.h>
#include <unistd.h>

#define err_string(msg) ("Error:" msg)

static void test_in3_config() {

  in3_t* c           = in3_for_chain(ETH_CHAIN_ID_MAINNET);
  c->transport       = test_transport;
  c->flags           = FLAGS_STATS;
  c->proof           = PROOF_NONE;
  c->signature_count = 0;

  in3_ctx_t* ctx = in3_client_rpc_ctx(c, "in3_config", "[{\
     \"chainId\":7,\
     \"autoUpdateList\":true,\
     \"finality\":50,\
     \"includeCode\":true,\
     \"maxAttempts\":99,\
     \"maxBlockCache\":98,\
     \"maxCodeCache\":97,\
     \"minDeposit\":96,\
     \"keepIn3\":true,\
     \"nodeLimit\":95,\
     \"proof\":\"full\",\
     \"replaceLatestBlock\":94,\
     \"requestCount\":93,\
     \"signatureCount\":92,\
     \"nodes\":{\
        \"0x7\":{\
           \"contract\":\"0x1234567890123456789012345678901234567890\",\
           \"whiteListContract\":\"0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab\",\
           \"registryId\":\"0x3456789012345678901234567890123456789012345678901234567890ffff\",\
           \"needsUpdate\":false,\
           \"nodeList\":[{\
              \"url\":\"#1\",\
              \"props\":\"0xffff\",\
              \"address\":\"0x1234567890123456789012345678901234567890\"\
           }]\
       }\
     }\
   }]");

  TEST_ASSERT_NULL(ctx->error);
  TEST_ASSERT_EQUAL(1, d_get_intk(ctx->responses[0], K_RESULT));
  ctx_free(ctx);

  TEST_ASSERT_EQUAL(7, c->chain_id);
  TEST_ASSERT_EQUAL(FLAGS_AUTO_UPDATE_LIST, c->flags & FLAGS_AUTO_UPDATE_LIST);
  TEST_ASSERT_EQUAL(50, c->finality);
  TEST_ASSERT_EQUAL(FLAGS_INCLUDE_CODE, c->flags & FLAGS_INCLUDE_CODE);
  TEST_ASSERT_EQUAL(99, c->max_attempts);
  TEST_ASSERT_EQUAL(98, c->max_block_cache);
  TEST_ASSERT_EQUAL(97, c->max_code_cache);
  TEST_ASSERT_EQUAL(96, c->min_deposit);
  TEST_ASSERT_EQUAL(PROOF_FULL, c->proof);
  TEST_ASSERT_EQUAL(95, c->node_limit);
  TEST_ASSERT_EQUAL(94, c->replace_latest_block);
  TEST_ASSERT_EQUAL(93, c->request_count);
  TEST_ASSERT_EQUAL(92, c->signature_count);
  TEST_ASSERT_EQUAL(FLAGS_KEEP_IN3, c->flags & FLAGS_KEEP_IN3);

  in3_chain_t* chain = in3_find_chain(c, 7);
  TEST_ASSERT_NOT_NULL(chain);

  char tmp[64];
  bytes_to_hex(chain->contract->data, chain->contract->len, tmp);
  TEST_ASSERT_EQUAL_STRING("1234567890123456789012345678901234567890", tmp);
  bytes_to_hex(chain->registry_id, 32, tmp);
  TEST_ASSERT_EQUAL_STRING("003456789012345678901234567890123456789012345678901234567890ffff", tmp);
  TEST_ASSERT_NULL(chain->nodelist_upd8_params);
  TEST_ASSERT_EQUAL(1, chain->nodelist_length);

  bytes_to_hex(chain->whitelist->contract, 20, tmp);
  TEST_ASSERT_EQUAL_STRING("dd80249a0631cf0f1593c7a9c9f9b8545e6c88ab", tmp);
  bytes_to_hex(chain->nodelist->address->data, chain->nodelist->address->len, tmp);
  TEST_ASSERT_EQUAL_STRING("1234567890123456789012345678901234567890", tmp);
  TEST_ASSERT_EQUAL_STRING("#1", chain->nodelist->url);
  TEST_ASSERT_EQUAL(0xffff, chain->nodelist->props);

  in3_free(c);
}

static void test_in3_client_rpc() {
  char * result = NULL, *error = NULL;
  in3_t* c           = in3_for_chain(ETH_CHAIN_ID_MAINNET);
  c->transport       = test_transport;
  c->flags           = FLAGS_STATS;
  c->proof           = PROOF_NONE;
  c->signature_count = 0;
  c->max_attempts    = 1;
  for (int i = 0; i < c->chains_length; i++)
    c->chains[i].nodelist_upd8_params = NULL;

  // Error response string
  add_response("eth_blockNumber", "[]", NULL, "\"Error\"", NULL);
  TEST_ASSERT_EQUAL(IN3_ERPC, in3_client_rpc(c, "eth_blockNumber", "[]", &result, &error));
  free(result);
  free(error);

  // Error response obj with message
  add_response("eth_blockNumber", "[]", NULL, "{\"message\":\"Undefined\"}", NULL);
  TEST_ASSERT_EQUAL(IN3_ERPC, in3_client_rpc(c, "eth_blockNumber", "[]", &result, &error));
  free(result);
  free(error);

  // Error response obj without message
  add_response("eth_blockNumber", "[]", NULL, "{\"Failure\":\"Undefined\"}", NULL);
  TEST_ASSERT_EQUAL(IN3_ERPC, in3_client_rpc(c, "eth_blockNumber", "[]", &result, &error));
  free(result);
  free(error);

  // Invalid JSON request
  TEST_ASSERT_EQUAL(IN3_EINVAL, in3_client_rpc(c, "eth_blockNumber", "[\"]", &result, &error));
  free(result);
  free(error);

  // Invalid calls to in3_client_rpc()
  TEST_ASSERT_EQUAL(IN3_EINVAL, in3_client_rpc(c, "eth_blockNumber", "[]", &result, NULL));
  free(result);
  TEST_ASSERT_EQUAL(IN3_EINVAL, in3_client_rpc(c, "eth_blockNumber", "[]", NULL, NULL));

  // Invalid calls to in3_client_rpc_ctx()
  in3_ctx_t* ctx = in3_client_rpc_ctx(c, "eth_blockNumber", "[\"]");
  TEST_ASSERT_NOT_NULL(ctx->error);
  ctx_free(ctx);

  // No transport check
  c->transport = NULL;
  TEST_ASSERT_EQUAL(IN3_ECONFIG, in3_client_rpc(c, "eth_blockNumber", "[]", &result, &error));
  c->transport = test_transport;

  // test in3_client_exec_req() with keep_in3 set to true
  // TODO: also test with use_binary set to true
  c->flags |= FLAGS_KEEP_IN3;
  add_response("eth_blockNumber", "[]", NULL, "{\"message\":\"Undefined\"}", "{\"version\": \"2.1.0\",\"chainId\": \"0x5\",\"verification\": \"proof\"}");
  char* response = in3_client_exec_req(c, "{\"method\":\"eth_blockNumber\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":[]}");
  TEST_ASSERT_NOT_NULL(response);
  TEST_ASSERT_NOT_NULL(str_find(response, "\"in3\":{\"version\": \"2.1.0\",\"chainId\": \"0x5\",\"verification\": \"proof\"}"));
  free(response);

  // test in3_client_exec_req() with keep_in3 set to false
  BITMASK_CLEAR(c->flags, FLAGS_KEEP_IN3);
  add_response("eth_blockNumber", "[]", NULL, "{\"message\":\"Undefined\"}", "{\"version\": \"2.1.0\",\"chainId\": \"0x5\",\"verification\": \"proof\"}");
  response = in3_client_exec_req(c, "{\"method\":\"eth_blockNumber\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":[]}");
  TEST_ASSERT_NOT_NULL(response);
  free(response);

  //  // Invalid JSON result
  //  add_response("eth_blockNumber", "[]", "\"\"0x84cf52\"", NULL, NULL);
  //  TEST_ASSERT_EQUAL(IN3_EUNKNOWN, in3_client_rpc(c, "eth_blockNumber", "[]", &result, &error));
  //
  //  // No result and no error
  //  add_response("eth_blockNumber", "[]", NULL, NULL, NULL);
  //  TEST_ASSERT_EQUAL(IN3_EUNKNOWN, in3_client_rpc(c, "eth_blockNumber", "[]", &result, &error));
}

IN3_IMPORT_TEST void initChain(in3_chain_t* chain, uint64_t chainId, char* contract, char* registry_id, uint8_t version, int boot_node_count, in3_chain_type_t type, json_ctx_t* spec);

static void test_in3_client_chain() {
  // Leading zeros in registry id
  in3_chain_t chain;
  initChain(&chain, 0x01, "ac1b824795e1eb1f6e609fe0da9b9af8beaab60f", "23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845fa", 2, 2, CHAIN_ETH, NULL);
  uint8_t reg_id[32];
  hex_to_bytes("0023d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845fa", -1, reg_id, 32);
  TEST_ASSERT_EQUAL_MEMORY(chain.registry_id, reg_id, 32);

  // Reregister chains with same chain id
  in3_t*    c = in3_for_chain(ETH_CHAIN_ID_MULTICHAIN);
  address_t contract1, contract2;
  hex_to_bytes("0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f", -1, contract1, 20);
  hex_to_bytes("0x5f51e413581dd76759e9eed51e63d14c8d1379c8", -1, contract2, 20);
  bytes32_t registry_id;
  hex_to_bytes("0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb", -1, registry_id, 32);
  in3_client_register_chain(c, 0x8, CHAIN_ETH, contract1, registry_id, 2, NULL);
  in3_client_register_chain(c, 0x8, CHAIN_ETH, contract2, registry_id, 2, NULL);
  TEST_ASSERT_EQUAL_MEMORY(in3_find_chain(c, 0x8)->contract->data, contract2, 20);

  // Add node with same address
  in3_client_add_node(c, 0x8, "http://test1.com", 0xFF, contract1);
  in3_client_add_node(c, 0x8, "http://test2.com", 0xFF, contract1);
  TEST_ASSERT_EQUAL_STRING(in3_find_chain(c, 0x8)->nodelist[0].url, "http://test2.com");
  TEST_ASSERT_EQUAL_UINT64(in3_find_chain(c, 0x8)->nodelist[0].props, 0xFF);

  // remove last node
  TEST_ASSERT_EQUAL(IN3_OK, in3_client_remove_node(c, 0x8, contract1));

  in3_free(c);
}

static void checksum(d_token_t* params, chain_id_t chain, char* result) {
  bytes_t*  adr = d_get_bytes_at(params, 0);
  in3_ret_t res = to_checksum(adr->data, 0, result);
}

static void test_in3_checksum_rpc() {
  char*       param_test = "[\"0x0dE496AE79194D5F5b18eB66987B504A0FEB32f2\",false]";
  char *      result = NULL, *error = NULL;
  in3_t*      in3     = in3_for_chain(ETH_CHAIN_ID_MAINNET);
  json_ctx_t* json    = parse_json(param_test);
  d_token_t*  address = &json->result[0];
  char        ret_checksum[43];
  checksum(address, 0, ret_checksum);
  in3_ret_t ret = in3_client_rpc(in3, "in3_checksumAddress", param_test, &result, &error);
  // remove quotes from result
  char str_result[43];
  memcpy(str_result, &result[1], 42);
  str_result[42] = '\0';
  TEST_ASSERT_EQUAL(ret, IN3_OK);
  TEST_ASSERT_EQUAL(error, NULL);
  TEST_ASSERT_EQUAL_STRING(ret_checksum, str_result);
  free(result);
  free(error);
}

static void test_in3_client_context() {
  in3_t*     c   = in3_for_chain(ETH_CHAIN_ID_MULTICHAIN);
  in3_ctx_t* ctx = ctx_new(c, "[{\"id\":1,\"jsonrpc\":\"2.0\","
                              "\"method\":\"eth_getBlockByHash\","
                              "\"params\":[\"0xe670ec64341771606e55d6b4ca35a1a6b75ee3d5145a99d05921026d1527331\", false],"
                              "\"in3\":{\"version\": \"" IN3_PROTO_VER "\",\"chainId\":\"0x1\"}}]");
  TEST_ASSERT_EQUAL(IN3_EINVAL, ctx_get_error(ctx, 1));
  TEST_ASSERT_EQUAL(IN3_ERPCNRES, ctx_get_error(ctx, 0));

  // null maybe a valid result
  json_ctx_t* json  = parse_json("{\"result\":null}");
  ctx->responses    = malloc(sizeof(d_token_t*));
  ctx->responses[0] = json->result;
  TEST_ASSERT_EQUAL(IN3_OK, ctx_get_error(ctx, 0));
  json_free(json);

  // Test with error
  json              = parse_json("{\"error\":\"Unknown\"}");
  ctx->responses[0] = json->result;
  TEST_ASSERT_EQUAL(IN3_EINVALDT, ctx_get_error(ctx, 0));
  // Test ctx_check_response_error() which internally also calls ctx_set_error()
  TEST_ASSERT_EQUAL(IN3_ERPC, ctx_check_response_error(ctx, 0));
  TEST_ASSERT_EQUAL_STRING("Unknown", ctx->error);
  json_free(json);

  // Test with error obj
  json              = parse_json("{\"error\":{\"msg\":\"Unknown\",\"id\":\"0xf1\"}}");
  ctx->responses[0] = json->result;
  TEST_ASSERT_EQUAL(IN3_ERPC, ctx_check_response_error(ctx, 0));
  TEST_ASSERT_EQUAL_STRING("{\"msg\":\"Unknown\",\"id\":\"0xf1\"}:Unknown", ctx->error);
  json_free(json);
  free(ctx->responses);
  ctx->responses = NULL;

  // Test getter/setter
  TEST_ASSERT_EQUAL(IN3_ERPC, ctx_set_error(ctx, "RPC failure", IN3_ERPC));
  TEST_ASSERT_EQUAL(IN3_ERPC, ctx_get_error(ctx, 0));
  TEST_ASSERT_EQUAL_STRING("RPC failure:{\"msg\":\"Unknown\",\"id\":\"0xf1\"}:Unknown", ctx->error);

  ctx_free(ctx);
  in3_free(c);
}

/*
 * Main
 */
int main() {
  in3_register_eth_full();
  in3_register_eth_api();
  in3_log_set_quiet(true);

  // now run tests
  TESTS_BEGIN();
  RUN_TEST(test_in3_config);
  RUN_TEST(test_in3_client_rpc);
  RUN_TEST(test_in3_checksum_rpc);
  RUN_TEST(test_in3_client_chain);
  RUN_TEST(test_in3_client_context);
  return TESTS_END();
}
