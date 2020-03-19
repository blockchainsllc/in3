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
#include "../../src/verifier/eth1/basic/eth_basic.h"
#include "../../src/verifier/eth1/basic/filter.h"
#include "../test_utils.h"
#include "../util/transport.h"
#include <stdio.h>
#include <unistd.h>

#define TEST_ASSERT_FILTER_OPT_FROMBLK(opt_str, blk, out_str) \
  do {                                                        \
    char* opt = filter_opt_set_fromBlock(opt_str, blk, true); \
    TEST_ASSERT_EQUAL_STRING(out_str, opt);                   \
    _free(opt);                                               \
  } while (0)

static void test_filter() {
  in3_t* c           = in3_for_chain(ETH_CHAIN_ID_MAINNET);
  c->transport       = test_transport;
  c->flags           = FLAGS_STATS;
  c->proof           = PROOF_NONE;
  c->signature_count = 0;

  for (int i = 0; i < c->chains_length; i++) c->chains[i].nodelist_upd8_params = NULL;

  char *result = NULL, *error = NULL;
  add_response("eth_blockNumber", "[]", "\"0x84cf52\"", NULL, NULL);

  TEST_ASSERT_EQUAL(0, in3_client_rpc(c, "eth_newFilter", "[{\"fromBlock\":\"0x84cf51\",\"address\":\"0xF0AD5cAd05e10572EfcEB849f6Ff0c68f9700455\",\"topics\":[\"0xca6abbe9d7f11422cb6ca7629fbf6fe9efb1c621f71ce8f02b9f2a230097404f\"]}]", &result, &error));
  TEST_ASSERT_NULL(error);
  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("\"0x1\"", result);

  add_response("eth_blockNumber", "[]", "\"0x84cf52\"", NULL, NULL);

  free(result);

  TEST_ASSERT_EQUAL(0, in3_client_rpc(c, "eth_newFilter", "[{\"fromBlock\":\"0x84cf51\",\"address\":\"0xF0AD5cAd05e10572EfcEB849f6Ff0c68f9700455\",\"topics\":[\"0xca6abbe9d7f11422cb6ca7629fbf6fe9efb1c621f71ce8f02b9f2a230097404f\"]}]", &result, &error));
  TEST_ASSERT_NULL(error);
  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("\"0x2\"", result);

  free(result);

  // now we simulate a blocknumber ..55 which is higher then ..51 we registered
  add_response("eth_getLogs", "[{\"fromBlock\":\"0x84cf51\",\"address\":\"0xF0AD5cAd05e10572EfcEB849f6Ff0c68f9700455\",\"topics\":[\"0xca6abbe9d7f11422cb6ca7629fbf6fe9efb1c621f71ce8f02b9f2a230097404f\"]}]", "[]", NULL, NULL);
  add_response("eth_blockNumber", "[]", "\"0x84cf55\"", NULL, NULL);

  TEST_ASSERT_EQUAL(0, in3_client_rpc(c, "eth_getFilterChanges", "[\"0x1\"]", &result, &error));
  TEST_ASSERT_NULL(error);
  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("[]", result);

  free(result);
  // now we simulate a blocknumber ..59 which is higher then ..55 we registered
  add_response("eth_getLogs", "[{\"fromBlock\":\"0x84cf56\",\"address\":\"0xF0AD5cAd05e10572EfcEB849f6Ff0c68f9700455\",\"topics\":[\"0xca6abbe9d7f11422cb6ca7629fbf6fe9efb1c621f71ce8f02b9f2a230097404f\"]}]", "[]", NULL, NULL);
  add_response("eth_blockNumber", "[]", "\"0x84cf59\"", NULL, NULL);

  TEST_ASSERT_EQUAL(0, in3_client_rpc(c, "eth_getFilterChanges", "[\"0x1\"]", &result, &error));
  TEST_ASSERT_NULL(error);
  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("[]", result);

  free(result);
  // now we simulate a blocknumber ..59 which is higher then ..55 we registered
  add_response("eth_blockNumber", "[]", "\"0x84cf59\"", NULL, NULL);
  // because we give him the same block, we will send a second request.

  TEST_ASSERT_EQUAL(0, in3_client_rpc(c, "eth_getFilterChanges", "[\"0x1\"]", &result, &error));
  TEST_ASSERT_NULL(error);
  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("[]", result);

  free(result);

  //eth_getFilterChanges

  TEST_ASSERT_EQUAL(0, in3_client_rpc(c, "eth_uninstallFilter", "[\"0x1\"]", &result, &error));
  TEST_ASSERT_NULL(error);
  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("true", result);

  free(result);

  TEST_ASSERT_EQUAL(0, in3_client_rpc(c, "eth_uninstallFilter", "[\"0x1\"]", &result, &error));
  TEST_ASSERT_NULL(error);
  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("false", result);

  in3_free(c);
}

static inline bool is_filter_opt_json_valid(char* opt_json_str) {
  bool        is_valid  = false;
  json_ctx_t* tx_params = parse_json(opt_json_str);
  if (tx_params)
    is_valid = filter_opt_valid(tx_params->result);
  json_free(tx_params);
  return is_valid;
}

static void test_filter_opt_validation() {
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{}"));
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{\"fromBlock\":\"0x84cf56\"}"));
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{\"fromBlock\":\"latest\"}"));
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{\"fromBlock\":1234567}"));
  TEST_ASSERT_FALSE(is_filter_opt_json_valid("{\"fromBlock\":[]]}"));
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{\"toBlock\":\"0x84cf56\"}"));
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{\"toBlock\":\"latest\"}"));
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{\"toBlock\":1234567}"));
  TEST_ASSERT_FALSE(is_filter_opt_json_valid("{\"toBlock\":[]]}"));
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{\"blockHash\":\"0x01234567890123456789012345678901\"}"));
  TEST_ASSERT_FALSE(is_filter_opt_json_valid("{\"toBlock\":1234567,\"blockHash\":\"0x0123456789012345678901234567890101234567890123456789012345678901\"}"));
  TEST_ASSERT_FALSE(is_filter_opt_json_valid("{\"fromBlock\":1234567,\"blockHash\":\"0x0123456789012345678901234567890101234567890123456789012345678901\"}"));
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{\"address\":\"0x0123456789012345678901234567890123456789\"}"));
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{\"address\":[\"0x0123456789012345678901234567890123456789\",\"0x1234567890123456789012345678901234567890\"]}"));
  TEST_ASSERT_FALSE(is_filter_opt_json_valid("{\"address\":[[\"0x0123456789012345678901234567890123456789\"],\"0x1234567890123456789012345678901234567890\"]}"));
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{\"topics\":[\"0x0123456789012345678901234567890101234567890123456789012345678901\"]}"));
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{\"topics\":[\"0x0123456789012345678901234567890101234567890123456789012345678901\",\"0x1234567890123456789012345678901012345678901234567890123456789012\"]}"));
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{\"topics\":[\"0x0123456789012345678901234567890101234567890123456789012345678901\",null]}"));
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{\"topics\":[null,\"0x0123456789012345678901234567890101234567890123456789012345678901\"]}"));
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{\"topics\":[\"0x0123456789012345678901234567890101234567890123456789012345678901\",null,\"0x1234567890123456789012345678901012345678901234567890123456789012\"]}"));
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{\"topics\":[[\"0x0123456789012345678901234567890101234567890123456789012345678901\",\"0x1234567890123456789012345678901012345678901234567890123456789012\"],null]}"));
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{\"topics\":[null,[\"0x0123456789012345678901234567890101234567890123456789012345678901\",\"0x1234567890123456789012345678901012345678901234567890123456789012\"]]}"));
  TEST_ASSERT_TRUE(is_filter_opt_json_valid("{\"topics\":[[\"0x0123456789012345678901234567890101234567890123456789012345678901\",\"0x1234567890123456789012345678901012345678901234567890123456789012\"],null,\"0x1234567890123456789012345678901012345678901234567890123456789012\"]}"));
  TEST_ASSERT_FALSE(is_filter_opt_json_valid("{\"topics\":[[\"0x0123456789012345678901234567890101234567890123456789012345678901\",[\"0x0123456789012345678901234567890101234567890123456789012345678901\",\"0x1234567890123456789012345678901012345678901234567890123456789012\"]]]}"));
}

static void test_filter_from_block_manip() {
  TEST_ASSERT_FILTER_OPT_FROMBLK("{\"fromBlock\":\"0x84cf56\"}", 0x84cf57, "{\"fromBlock\":\"0x84cf57\"}");
  TEST_ASSERT_FILTER_OPT_FROMBLK("{\"fromBlock\":\"latest\"}", 0x84cf57, "{\"fromBlock\":\"0x84cf57\"}");
  // Todo: support integer fromBlock manipulation
  //  TEST_ASSERT_FILTER_OPT_FROMBLK("{\"fromBlock\":1234567}", 0x84cf57, "{\"fromBlock\":\"0x84cf57\"}");
  TEST_ASSERT_FILTER_OPT_FROMBLK("{}", 0x84cf57, "{\"fromBlock\":\"0x84cf57\"}");
}

static void test_filter_creation() {
  in3_t* c           = in3_for_chain(ETH_CHAIN_ID_MAINNET);
  c->transport       = test_transport;
  c->flags           = FLAGS_STATS;
  c->proof           = PROOF_NONE;
  c->signature_count = 0;

  for (int i = 0; i < c->chains_length; i++) c->chains[i].nodelist_upd8_params = NULL;

  TEST_ASSERT_FALSE(filter_remove(c, 1));
  TEST_ASSERT_EQUAL(IN3_EINVAL, filter_add(c, FILTER_EVENT, NULL));
  add_response("eth_blockNumber", "[]", "\"0x84cf59\"", NULL, NULL);
  TEST_ASSERT_GREATER_THAN(0, filter_add(c, FILTER_BLOCK, NULL));
  add_response("eth_blockNumber", "[]", "\"0x84cf5a\"", NULL, NULL);
  TEST_ASSERT_GREATER_THAN(0, filter_add(c, FILTER_BLOCK, NULL));
  TEST_ASSERT_TRUE(filter_remove(c, 1));
  add_response("eth_blockNumber", "[]", "\"0x84cf5f\"", NULL, NULL);
  TEST_ASSERT_GREATER_THAN(0, filter_add(c, FILTER_BLOCK, NULL));
  TEST_ASSERT_EQUAL(2, c->filters->count);
  TEST_ASSERT_FALSE(filter_remove(c, 10));
  TEST_ASSERT_FALSE(filter_remove(c, 0));
  TEST_ASSERT_FALSE(0);
  in3_free(c);
}

static void test_filter_changes() {
  in3_t* c           = in3_for_chain(ETH_CHAIN_ID_MAINNET);
  c->transport       = test_transport;
  c->flags           = FLAGS_STATS;
  c->proof           = PROOF_NONE;
  c->signature_count = 0;

  for (int i = 0; i < c->chains_length; i++) c->chains[i].nodelist_upd8_params = NULL;

  in3_ctx_t* ctx = ctx_new(c, "{\"method\":\"eth_getBlockByNumber\",\"params\":[\"latest\",false]}");
  TEST_ASSERT_EQUAL(IN3_EUNKNOWN, filter_get_changes(ctx, 1, NULL));
  add_response("eth_blockNumber", "[]", "\"0x84cf59\"", NULL, NULL);
  TEST_ASSERT_GREATER_THAN(0, filter_add(c, FILTER_BLOCK, NULL));
  TEST_ASSERT_EQUAL(IN3_EUNKNOWN, filter_get_changes(ctx, 10, NULL));
  TEST_ASSERT_EQUAL(IN3_EUNKNOWN, filter_get_changes(ctx, 0, NULL));
  TEST_ASSERT_TRUE(filter_remove(c, 1));

  add_response("eth_blockNumber", "[]", "\"0x84cf59\"", NULL, NULL);
  TEST_ASSERT_EQUAL(IN3_EUNKNOWN, filter_get_changes(ctx, 1, NULL));

  add_response("eth_blockNumber", "[]", "\"0x84cf58\"", NULL, NULL);
  TEST_ASSERT_EQUAL(1, filter_add(c, FILTER_BLOCK, NULL));
  ctx_free(ctx);

  add_response("eth_getBlockByNumber",
               "[\"0x84cf59\",false]",
               "{"
               "        \"author\": \"0x0000000000000000000000000000000000000000\","
               "        \"difficulty\": \"0x2\","
               "        \"extraData\": \"0x44505020417574686f7269747900000000000000000000000000000000000000d2d0c956dddf306aae94dd3c53c5e022418eb17a040a6e89674568686baff4576a1aa2b6d9434beb5bc971070ca5d54ca2c83ec50e47915235d05d5e1de22b4100\","
               "        \"gasLimit\": \"0x7a1200\","
               "        \"gasUsed\": \"0x94ae8\","
               "        \"hash\": \"0xf407f59e59f35659ebf92b7c51d7faab027b3217144dd5bce9fc5b42de1e1de9\","
               "        \"logsBloom\": \"0x00040000001080400000000000000000000000200000400000800000000000000040000000001008000000000002000000000000020000008008840000000000400000000000008000000000000000000002000000000000000000000042040008500000000000000000000004000000000000004000000010000000000000000000000000001000000040000009000000000041000000000000000000804000000000004000000000400000800004000000000040800000003000000008400000000004000000000000002000040000000000008000002000000000000000000000000000000000000400100000000000000008080000000000004000000000\","
               "        \"miner\": \"0x0000000000000000000000000000000000000000\","
               "        \"number\": \"0x19d45f\","
               "        \"parentHash\": \"0x47ef26f15caaab0a365071f5f9886374883581068e66227ced15b1724d09f090\","
               "        \"receiptsRoot\": \"0x8cbd134c7b5819b81a9f12ba34edc04d0908d11b886915cd4b9d7ac956c2f37d\","
               "        \"sealFields\": ["
               "          \"0xa00000000000000000000000000000000000000000000000000000000000000000\","
               "          \"0x880000000000000000\""
               "        ],"
               "        \"sha3Uncles\": \"0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347\","
               "        \"size\": \"0x72b\","
               "        \"stateRoot\": \"0x9d7f22d1b37a19f35f600c162bb2382488e26446dd5f60043161dec9ea169af2\","
               "        \"timestamp\": \"0x5dd79bd0\","
               "        \"totalDifficulty\": \"0x27c8e0\","
               "        \"transactions\": [],"
               "        \"transactionsRoot\": \"0xb7dd5015e1ef1ddd97dca5fc8447c8f497dbacb5d62bdd9e62b3925ce7885631\","
               "        \"uncles\": [ ]"
               "}",
               NULL,
               NULL);
  add_response("eth_blockNumber", "[]", "\"0x84cf59\"", NULL, NULL);

  ctx          = ctx_new(c, "{\"method\":\"eth_getBlockByNumber\",\"params\":[\"latest\",false]}");
  sb_t* result = sb_new("");
  TEST_ASSERT_EQUAL(IN3_OK, filter_get_changes(ctx, 1, result));
  TEST_ASSERT_EQUAL_STRING("[\"0xf407f59e59f35659ebf92b7c51d7faab027b3217144dd5bce9fc5b42de1e1de9\"]", result->data);
  ctx_free(ctx);

  add_response("eth_blockNumber", "[]", "\"0x84cf60\"", NULL, NULL);
  TEST_ASSERT_EQUAL(2, filter_add(c, FILTER_BLOCK, NULL));
  add_response("eth_blockNumber", "[]", "\"0x84cf60\"", NULL, NULL);
  ctx    = ctx_new(c, "{\"method\":\"eth_getBlockByNumber\",\"params\":[\"latest\",false]}");
  result = sb_new("");
  TEST_ASSERT_EQUAL(IN3_OK, filter_get_changes(ctx, 2, result));
  TEST_ASSERT_EQUAL_STRING("[]", result->data);
  ctx_free(ctx);
  in3_free(c);
}

/*
 * Main
 */
int main() {
  in3_log_set_quiet(true);
  TESTS_BEGIN();
  in3_register_eth_basic();
  RUN_TEST(test_filter);
  RUN_TEST(test_filter_opt_validation);
  RUN_TEST(test_filter_from_block_manip);
  RUN_TEST(test_filter_creation);
  RUN_TEST(test_filter_changes);
  return TESTS_END();
}
