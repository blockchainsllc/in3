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
#include "../../src/core/util/log.h"
#include "../../src/core/util/utils.h"
#include "../../src/verifier/eth1/basic/eth_basic.h"
#include "../test_utils.h"
#include <stdio.h>
#include <unistd.h>

#define TEST_ASSERT_CONFIGURE_FAIL(desc, in3, config, err_slice) \
  do {                                                           \
    char* _err_ = in3_configure(in3, config);                    \
    TEST_ASSERT_NOT_NULL(str_find(_err_, err_slice));            \
    free(_err_);                                                 \
  } while (0)

#define TEST_ASSERT_CONFIGURE_PASS(in3, config) \
  TEST_ASSERT_NULL(in3_configure(in3, config))

static void test_configure_request() {
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

static void test_exec_req() {
  in3_t* c      = in3_for_chain(ETH_CHAIN_ID_MAINNET);
  char*  result = in3_client_exec_req(c, "{\"method\":\"web3_sha3\",\"params\":[\"0x1234\"]}");
  TEST_ASSERT_EQUAL_STRING("{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"0x56570de287d73cd1cb6092bb8fdee6173974955fdef345ae579ee9f475ea7432\"}", result);
  _free(result);

  result = in3_client_exec_req(c, "\"method\":\"web3_sha3\",\"params\":[\"0x1234\"]}");
  TEST_ASSERT_EQUAL_STRING("{\"id\":0,\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32700,\"message\":\"The Request is not a valid structure!\"}}", result);
  _free(result);

  result = in3_client_exec_req(c, "{\"params\":[\"0x1234\"]}");
  TEST_ASSERT_EQUAL_STRING("{\"id\":0,\"jsonrpc\":\"2.0\",\"error\":{\"code\":-6,\"message\":\"No Method defined\"}}", result);
  _free(result);

  in3_free(c);
}

static void test_configure_validation() {
  in3_t* c = in3_for_chain(0);

  TEST_ASSERT_CONFIGURE_FAIL("invalid JSON in config", c, "{\"\"}", "parse error");

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: autoUpdateList", c, "{\"autoUpdateList\":1}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: autoUpdateList", c, "{\"autoUpdateList\":\"1\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: autoUpdateList", c, "{\"autoUpdateList\":\"0x00000\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"autoUpdateList\":true}");
  TEST_ASSERT_EQUAL(c->auto_update_list, true);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: chainId", c, "{\"chainId\":\"-1\"}", "expected uint32 or string");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: chainId", c, "{\"chainId\":\"\"}", "expected uint32 or string");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: chainId", c, "{\"chainId\":\"0\"}", "expected uint32 or string");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: chainId", c, "{\"chainId\":false}", "expected uint32 or string");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: chainId", c, "{\"chainId\":\"0x1203030230\"}", "expected uint32 or string");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"chainId\":\"mainnet\"}");
  TEST_ASSERT_EQUAL(c->chain_id, 1);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"chainId\":0}");
  TEST_ASSERT_EQUAL(c->chain_id, 0);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"chainId\":4294967295}"); // UINT32_MAX
  TEST_ASSERT_EQUAL(c->chain_id, 4294967295U);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"chainId\":\"0xffffffff\"}"); // UINT32_MAX
  TEST_ASSERT_EQUAL(c->chain_id, 0xffffffff);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: signatureCount", c, "{\"signatureCount\":\"-1\"}", "expected uint8");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: signatureCount", c, "{\"signatureCount\":\"0x1234\"}", "expected uint8");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: signatureCount", c, "{\"signatureCount\":\"value\"}", "expected uint8");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: signatureCount", c, "{\"signatureCount\":256}", "expected uint8");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"signatureCount\":0}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"signatureCount\":255}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"signatureCount\":\"0xff\"}");
  TEST_ASSERT_EQUAL(c->signature_count, 255);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: finality", c, "{\"finality\":\"-1\"}", "expected uint16");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: finality", c, "{\"finality\":\"0x123412341234\"}", "expected uint16");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: finality", c, "{\"finality\":\"value\"}", "expected uint16");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: finality", c, "{\"finality\":65536}", "expected uint16");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"finality\":0}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"finality\":65535}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"finality\":\"0xffff\"}");
  TEST_ASSERT_EQUAL(c->finality, 65535);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: includeCode", c, "{\"includeCode\":1}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: includeCode", c, "{\"includeCode\":\"1\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: includeCode", c, "{\"includeCode\":\"0x00000\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"includeCode\":true}");
  TEST_ASSERT_EQUAL(c->include_code, true);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxAttempts", c, "{\"maxAttempts\":\"-1\"}", "expected uint16");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxAttempts", c, "{\"maxAttempts\":\"0x123412341234\"}", "expected uint16");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxAttempts", c, "{\"maxAttempts\":\"value\"}", "expected uint16");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxAttempts", c, "{\"maxAttempts\":65536}", "expected uint16");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"maxAttempts\":0}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"maxAttempts\":65535}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"maxAttempts\":\"0xffff\"}");
  TEST_ASSERT_EQUAL(c->max_attempts, 65535);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: keepIn3", c, "{\"keepIn3\":1}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: keepIn3", c, "{\"keepIn3\":\"1\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: keepIn3", c, "{\"keepIn3\":\"0x00000\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"keepIn3\":true}");
  TEST_ASSERT_EQUAL(c->keep_in3, true);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxBlockCache", c, "{\"maxBlockCache\":\"-1\"}", "expected uint32");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxBlockCache", c, "{\"maxBlockCache\":\"\"}", "expected uint32");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxBlockCache", c, "{\"maxBlockCache\":\"0\"}", "expected uint32");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxBlockCache", c, "{\"maxBlockCache\":false}", "expected uint32");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxBlockCache", c, "{\"maxBlockCache\":\"0x1203030230\"}", "expected uint32");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"maxBlockCache\":1}");
  TEST_ASSERT_EQUAL(c->max_block_cache, 1);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"maxBlockCache\":0}");
  TEST_ASSERT_EQUAL(c->max_block_cache, 0);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"maxBlockCache\":4294967295}"); // UINT32_MAX
  TEST_ASSERT_EQUAL(c->max_block_cache, 4294967295U);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"maxBlockCache\":\"0xffffffff\"}"); // UINT32_MAX
  TEST_ASSERT_EQUAL(c->max_block_cache, 0xffffffff);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxCodeCache", c, "{\"maxCodeCache\":\"-1\"}", "expected uint32");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxCodeCache", c, "{\"maxCodeCache\":\"\"}", "expected uint32");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxCodeCache", c, "{\"maxCodeCache\":\"0\"}", "expected uint32");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxCodeCache", c, "{\"maxCodeCache\":false}", "expected uint32");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxCodeCache", c, "{\"maxCodeCache\":\"0x1203030230\"}", "expected uint32");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"maxCodeCache\":1}");
  TEST_ASSERT_EQUAL(c->max_code_cache, 1);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"maxCodeCache\":0}");
  TEST_ASSERT_EQUAL(c->max_code_cache, 0);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"maxCodeCache\":4294967295}"); // UINT32_MAX
  TEST_ASSERT_EQUAL(c->max_code_cache, 4294967295U);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"maxCodeCache\":\"0xffffffff\"}"); // UINT32_MAX
  TEST_ASSERT_EQUAL(c->max_code_cache, 0xffffffff);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: minDeposit", c, "{\"minDeposit\":\"-1\"}", "expected uint64");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: minDeposit", c, "{\"minDeposit\":\"\"}", "expected uint64");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: minDeposit", c, "{\"minDeposit\":\"0\"}", "expected uint64");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: minDeposit", c, "{\"minDeposit\":false}", "expected uint64");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: minDeposit", c, "{\"minDeposit\":\"0x01234567890123456789\"}", "expected uint64");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"minDeposit\":1}");
  TEST_ASSERT_EQUAL(c->min_deposit, 1);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"minDeposit\":0}");
  TEST_ASSERT_EQUAL(c->min_deposit, 0);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"minDeposit\":18446744073709551615}"); // UINT64_MAX
  TEST_ASSERT_EQUAL(c->min_deposit, 18446744073709551615ULL);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"minDeposit\":\"0xffffffff\"}"); // UINT64_MAX
  TEST_ASSERT_EQUAL(c->min_deposit, 0xffffffffffffffff);

  in3_free(c);
}

/*
 * Main
 */
int main() {
  _free(in3_create_signer(NULL, NULL, NULL));
  _free(in3_create_storeage_handler(NULL, NULL, NULL));

  in3_log_set_quiet(true);
  in3_register_eth_basic();
  in3_register_eth_api();

  TESTS_BEGIN();
  RUN_TEST(test_configure_request);
  RUN_TEST(test_exec_req);
  RUN_TEST(test_configure_validation);
  return TESTS_END();
}
