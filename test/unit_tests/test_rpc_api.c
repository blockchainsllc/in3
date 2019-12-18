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

#include "../../src/api/eth1/abi.h"
#include "../../src/api/eth1/eth_api.h"
#include "../../src/core/client/context.h"
#include "../../src/core/client/keys.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/log.h"
#include "../../src/verifier/eth1/full/eth_full.h"
#include "../test_utils.h"
#include "../util/transport.h"
#include <stdio.h>
#include <unistd.h>

#define err_string(msg) ("Error:" msg)

static void test_in3_config() {

  in3_t* c          = in3_new();
  c->transport      = test_transport;
  c->chainId        = 0x1;
  c->autoUpdateList = false;
  c->proof          = PROOF_NONE;
  c->signatureCount = 0;

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
  free_ctx(ctx);

  TEST_ASSERT_EQUAL(7, c->chainId);
  TEST_ASSERT_EQUAL(true, c->autoUpdateList);
  TEST_ASSERT_EQUAL(50, c->finality);
  TEST_ASSERT_EQUAL(true, c->includeCode);
  TEST_ASSERT_EQUAL(99, c->max_attempts);
  TEST_ASSERT_EQUAL(98, c->maxBlockCache);
  TEST_ASSERT_EQUAL(97, c->maxCodeCache);
  TEST_ASSERT_EQUAL(96, c->minDeposit);
  TEST_ASSERT_EQUAL(PROOF_FULL, c->proof);
  TEST_ASSERT_EQUAL(95, c->nodeLimit);
  TEST_ASSERT_EQUAL(94, c->replaceLatestBlock);
  TEST_ASSERT_EQUAL(93, c->requestCount);
  TEST_ASSERT_EQUAL(92, c->signatureCount);
  TEST_ASSERT_EQUAL(1, c->keep_in3);

  in3_chain_t* chain = in3_find_chain(c, 7);
  TEST_ASSERT_NOT_NULL(chain);

  char tmp[64];
  bytes_to_hex(chain->contract->data, chain->contract->len, tmp);
  TEST_ASSERT_EQUAL_STRING("1234567890123456789012345678901234567890", tmp);
  bytes_to_hex(chain->registry_id, 32, tmp);
  TEST_ASSERT_EQUAL_STRING("003456789012345678901234567890123456789012345678901234567890ffff", tmp);
  TEST_ASSERT_EQUAL(false, chain->needsUpdate);
  TEST_ASSERT_EQUAL(1, chain->nodeListLength);

  bytes_to_hex(chain->nodeList->address->data, chain->nodeList->address->len, tmp);
  TEST_ASSERT_EQUAL_STRING("1234567890123456789012345678901234567890", tmp);
  TEST_ASSERT_EQUAL_STRING("#1", chain->nodeList->url);
  TEST_ASSERT_EQUAL(0xffff, chain->nodeList->props);

  in3_free(c);
}

static void test_in3_client_rpc() {
  char * result = NULL, *error = NULL;
  in3_t* c          = in3_new();
  c->transport      = test_transport;
  c->chainId        = 0x1;
  c->autoUpdateList = false;
  c->proof          = PROOF_NONE;
  c->signatureCount = 0;
  for (int i = 0; i < c->chainsCount; i++)
    c->chains[i].needsUpdate = false;

  // Error response string
  add_response("eth_blockNumber", "[]", NULL, "\"Error\"", NULL);
  TEST_ASSERT_EQUAL(IN3_EUNKNOWN, in3_client_rpc(c, "eth_blockNumber", "[]", &result, &error));
  free(result);
  free(error);

  // Error response obj with message
  add_response("eth_blockNumber", "[]", NULL, "{\"message\":\"Undefined\"}", NULL);
  TEST_ASSERT_EQUAL(IN3_EUNKNOWN, in3_client_rpc(c, "eth_blockNumber", "[]", &result, &error));
  free(result);
  free(error);

  // Error response obj without message
  add_response("eth_blockNumber", "[]", NULL, "{\"Failure\":\"Undefined\"}", NULL);
  TEST_ASSERT_EQUAL(IN3_EUNKNOWN, in3_client_rpc(c, "eth_blockNumber", "[]", &result, &error));
  free(result);
  free(error);

  // Invalid JSON request
  TEST_ASSERT_EQUAL(IN3_EUNKNOWN, in3_client_rpc(c, "eth_blockNumber", "[\"]", &result, &error));
  free(result);
  free(error);

  // Invalid calls to in3_client_rpc()
  TEST_ASSERT_EQUAL(IN3_EINVAL, in3_client_rpc(c, "eth_blockNumber", "[]", &result, NULL));
  free(result);
  TEST_ASSERT_EQUAL(IN3_EINVAL, in3_client_rpc(c, "eth_blockNumber", "[]", NULL, NULL));

  // Invalid calls to in3_client_rpc_ctx()
  in3_ctx_t* ctx = in3_client_rpc_ctx(c, "eth_blockNumber", "[\"]");
  TEST_ASSERT_NOT_NULL(ctx->error);
  free_ctx(ctx);

  // No transport check
  c->transport = NULL;
  TEST_ASSERT_EQUAL(IN3_EUNKNOWN, in3_client_rpc(c, "eth_blockNumber", "[]", &result, &error));
  c->transport = test_transport;

  // test in3_client_exec_req() with keep_in3 set to true
  // TODO: also test with use_binary set to true
  c->keep_in3 = true;
  add_response("eth_blockNumber", "[]", NULL, "{\"message\":\"Undefined\"}", NULL);
  char* response = in3_client_exec_req(c, "{\"method\":\"eth_blockNumber\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":[]}");
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
/*
 * Main
 */
int main() {
  in3_register_eth_full();
  in3_register_eth_api();

  // now run tests
  TESTS_BEGIN();
  RUN_TEST(test_in3_config);
  RUN_TEST(test_in3_client_rpc);
  return TESTS_END();
}
