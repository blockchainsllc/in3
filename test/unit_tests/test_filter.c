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
#include "../../src/verifier/eth1/basic/signer.h"
#include "../test_utils.h"
#include "../util/transport.h"
#include <stdio.h>
#include <unistd.h>

static void test_filter() {

  in3_register_eth_basic();

  in3_t* c = in3_for_chain(ETH_CHAIN_ID_MAINNET);
  ;
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
}

/*
 * Main
 */
int main() {
  in3_log_set_level(LOG_ERROR);
  TESTS_BEGIN();
  RUN_TEST(test_filter);
  return TESTS_END();
}
