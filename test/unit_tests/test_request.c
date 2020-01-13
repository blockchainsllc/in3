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

#include "../../src/api/eth1/eth_api.h"
#include "../../src/core/client/cache.h"
#include "../../src/core/client/context.h"
#include "../../src/core/client/keys.h"
#include "../../src/core/client/nodelist.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/utils.h"
#include "../../src/verifier/eth1/basic/eth_basic.h"
#include "../test_utils.h"
#include <stdio.h>
#include <unistd.h>

void test_configure_request() {
  in3_register_eth_basic();
  in3_register_eth_api();

  in3_t* c                = in3_for_chain(0);
  c->proof                = PROOF_FULL;
  c->signature_count      = 2;
  c->chains->needs_update = false;
  c->finality             = 10;
  c->include_code         = true;
  c->replace_latest_block = 6;
  c->use_binary           = true;
  c->use_http             = true;

  in3_ctx_t* ctx = ctx_new(c, "{\"method\":\"eth_getBlockByNumber\",\"params\":[\"latest\",false]}");
  TEST_ASSERT_EQUAL(IN3_WAITING, in3_ctx_execute(ctx));
  in3_request_t* request = in3_create_request(ctx);
  json_ctx_t*    json    = parse_json(request->payload);
  d_token_t*     in3     = d_get(d_get_at(json->result, 0), K_IN3);
  TEST_ASSERT_NOT_NULL(in3);
  TEST_ASSERT_EQUAL(1, d_get_int(in3, "useFullProof"));
  TEST_ASSERT_EQUAL(1, d_get_int(in3, "useBinary"));
  TEST_ASSERT_EQUAL(10, d_get_int(in3, "finality"));
  TEST_ASSERT_EQUAL(6, d_get_int(in3, "latestBlock"));
  d_token_t* signers = d_get(in3, key("signers"));
  TEST_ASSERT_NOT_NULL(signers);
  TEST_ASSERT_EQUAL(2, d_len(signers));
  request_free(request, ctx, false);
  json_free(json);
  ctx_free(ctx);

  in3_free(c);
}

void test_exec_req() {
  in3_register_eth_basic();
  in3_register_eth_api();

  in3_t* c = in3_for_chain(ETH_CHAIN_ID_MAINNET);

  char* result = in3_client_exec_req(c, "{\"method\":\"web3_sha3\",\"params\":[\"0x1234\"]}");
  TEST_ASSERT_EQUAL_STRING("{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"0x56570de287d73cd1cb6092bb8fdee6173974955fdef345ae579ee9f475ea7432\"}", result);
  _free(result);

  result = in3_client_exec_req(c, "\"method\":\"web3_sha3\",\"params\":[\"0x1234\"]}");
  TEST_ASSERT_EQUAL_STRING("{\"id\":0,\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32700,\"message\":\"The Request is not a valid structure!\"}}", result);
  _free(result);

  result = in3_client_exec_req(c, "{\"params\":[\"0x1234\"]}");
  TEST_ASSERT_EQUAL_STRING("{\"id\":0,\"jsonrpc\":\"2.0\",\"error\":{\"code\":-6,\"message\":\"No Method defined\"}}", result);
  _free(result);

  result = in3_client_exec_req(c, "{\"method\":\"in3_cacheClear\",\"params\":[]}");
  TEST_ASSERT_EQUAL_STRING("{\"id\":0,\"jsonrpc\":\"2.0\",\"error\":{\"code\":-6,\"message\":\"The request could not be handled\nNo storage set\"}}", result);
  _free(result);

  in3_free(c);
}
/*
 * Main
 */
int main() {
  _free(in3_create_signer(NULL, NULL, NULL));
  _free(in3_create_storage_handler(NULL, NULL, NULL, NULL));

  TESTS_BEGIN();
  RUN_TEST(test_configure_request);
  RUN_TEST(test_exec_req);
  return TESTS_END();
}
