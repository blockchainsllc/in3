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
#include "../../src/verifier/eth1/basic/eth_basic.h"
#include "../../src/verifier/eth1/basic/filter.h"
#include "../test_utils.h"
#include "../util/transport.h"
#include <stdio.h>
#include <unistd.h>

#define TEST_ASSERT_FILTER_OPT_FROMBLK(opt_str, blk, out_str) \
  do {                                                        \
    char* opt = filter_opt_set_fromBlock(opt_str, blk);       \
    TEST_ASSERT_EQUAL_STRING(out_str, opt);                   \
    _free(opt);                                               \
  } while (0)

static void test_filter() {

  in3_register_eth_basic();

  in3_t* c            = in3_for_chain(ETH_CHAIN_ID_MAINNET);
  c->transport        = test_transport;
  c->auto_update_list = false;
  c->proof            = PROOF_NONE;
  c->signature_count  = 0;

  for (int i = 0; i < c->chains_length; i++) c->chains[i].needs_update = false;

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

  // now we simulate a blocknumber ..55 which is higher then ..52 we registered
  add_response("eth_blockNumber", "[]", "\"0x84cf55\"", NULL, NULL);
  add_response("eth_getLogs", "[{\"fromBlock\":\"0x84cf52\",\"address\":\"0xF0AD5cAd05e10572EfcEB849f6Ff0c68f9700455\",\"topics\":[\"0xca6abbe9d7f11422cb6ca7629fbf6fe9efb1c621f71ce8f02b9f2a230097404f\"]}]", "[]", NULL, NULL);

  TEST_ASSERT_EQUAL(0, in3_client_rpc(c, "eth_getFilterChanges", "[\"0x1\"]", &result, &error));
  TEST_ASSERT_NULL(error);
  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("[]", result);

  free(result);
  // now we simulate a blocknumber ..59 which is higher then ..55 we registered
  add_response("eth_blockNumber", "[]", "\"0x84cf59\"", NULL, NULL);
  add_response("eth_getLogs", "[{\"fromBlock\":\"0x84cf56\",\"address\":\"0xF0AD5cAd05e10572EfcEB849f6Ff0c68f9700455\",\"topics\":[\"0xca6abbe9d7f11422cb6ca7629fbf6fe9efb1c621f71ce8f02b9f2a230097404f\"]}]", "[]", NULL, NULL);

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
/*
 * Main
 */
int main() {
  in3_log_set_quiet(true);
  TESTS_BEGIN();
  RUN_TEST(test_filter);
  RUN_TEST(test_filter_opt_validation);
  RUN_TEST(test_filter_from_block_manip);
  return TESTS_END();
}
