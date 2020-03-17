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

#include "../../src/api/eth1/eth_api.h"
#include "../../src/core/client/cache.h"
#include "../../src/core/client/context_internal.h"
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
#define CONTRACT_ADDRS "0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f"
#define REGISTRY_ID "0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb"
#define WHITELIST_CONTRACT_ADDRS "0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab"
#define NODE_URL "rpc.node"
#define NODE_ADDRS "0x8904b9813c9ada123f9fccb9123659088dacd477"

static void test_configure_request() {
  in3_t* c                = in3_for_chain(0);
  c->proof                = PROOF_FULL;
  c->signature_count      = 2;
  c->finality             = 10;
  c->flags                = FLAGS_INCLUDE_CODE | FLAGS_BINARY | FLAGS_HTTP | FLAGS_AUTO_UPDATE_LIST;
  c->replace_latest_block = 6;

  for (int i = 0; i < c->chains_length; i++) c->chains[i].nodelist_upd8_params = NULL;

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

static void test_configure_signed_request() {
  in3_t* c = in3_for_chain(ETH_CHAIN_ID_LOCAL);
  TEST_ASSERT_NULL(in3_configure(c, "{\"key\":\"0x1234567890123456789012345678901234567890123456789012345678901234\"}"));
  c->flags = FLAGS_INCLUDE_CODE;
  for (int i = 0; i < c->chains_length; i++) c->chains[i].nodelist_upd8_params = NULL;

  in3_ctx_t* ctx = ctx_new(c, "{\"method\":\"eth_blockNumber\",\"params\":[]}");
  TEST_ASSERT_EQUAL(IN3_WAITING, in3_ctx_execute(ctx));
  in3_request_t* request = in3_create_request(ctx);
  json_ctx_t*    json    = parse_json(request->payload);
  d_token_t*     in3     = d_get(d_get_at(json->result, 0), K_IN3);
  TEST_ASSERT_NOT_NULL(in3);
  bytes_t* sig = d_get_bytes(in3, "sig");
  TEST_ASSERT_NOT_NULL(sig);
  TEST_ASSERT_EQUAL(65, sig->len);
  char hex[150];
  TEST_ASSERT_EQUAL(65 * 2, bytes_to_hex(sig->data, sig->len, hex)); // 65bytes *2
  TEST_ASSERT_EQUAL_STRING("8e39d2066cf9d1898e6bc9fbbfaa8fd6b9e5a86515e643f537c831982718866d0903e91f5f8824363dd3754fe550b37aa1e6eeb3742f13ad36d3321972e959a701", hex);
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

  result = in3_client_exec_req(c, "{\"method\":\"in3_cacheClear\",\"params\":[]}");
  TEST_ASSERT_EQUAL_STRING("{\"id\":0,\"jsonrpc\":\"2.0\",\"error\":{\"code\":-6,\"message\":\"The request could not be handled:No storage set\"}}", result);
  _free(result);

  in3_free(c);
}

static void test_configure() {
  in3_t* c   = in3_for_chain(ETH_CHAIN_ID_MULTICHAIN);
  char*  tmp = NULL;

  // proof
  tmp = in3_configure(c, "{\"proof\":\"standard\"}");
  TEST_ASSERT_EQUAL(PROOF_STANDARD, c->proof);
  free(tmp);

  // rpc
  tmp = in3_configure(c, "{\"rpc\":\"http://rpc.slock.it\"}");
  TEST_ASSERT_EQUAL(PROOF_NONE, c->proof);
  TEST_ASSERT_EQUAL(ETH_CHAIN_ID_LOCAL, c->chain_id);
  TEST_ASSERT_EQUAL(1, c->request_count);
  TEST_ASSERT_EQUAL_STRING("http://rpc.slock.it", in3_find_chain(c, ETH_CHAIN_ID_LOCAL)->nodelist->url);
  free(tmp);

  // missing registryId and contract
  TEST_ASSERT_NOT_NULL((tmp = in3_configure(c, "{\"nodes\":{\"0x8\":{}}}")));
  free(tmp);

  // bad registryId
  TEST_ASSERT_NOT_NULL((tmp = in3_configure(c, "{\"nodes\":{\"0x8\":{\"registryId\":\"0x987\"}}}")));
  free(tmp);

  in3_free(c);
}

static void test_configure_validation() {
  in3_t* c = in3_for_chain(0);

  TEST_ASSERT_CONFIGURE_FAIL("invalid JSON in config", c, "{\"\"}", "parse error");

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: autoUpdateList", c, "{\"autoUpdateList\":1}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: autoUpdateList", c, "{\"autoUpdateList\":\"1\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: autoUpdateList", c, "{\"autoUpdateList\":\"0x00000\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"autoUpdateList\":true}");
  TEST_ASSERT_EQUAL(c->flags & FLAGS_AUTO_UPDATE_LIST, FLAGS_AUTO_UPDATE_LIST);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: chainId", c, "{\"chainId\":\"-1\"}", "expected uint32 or string");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: chainId", c, "{\"chainId\":\"\"}", "expected uint32 or string");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: chainId", c, "{\"chainId\":\"0\"}", "expected uint32 or string");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: chainId", c, "{\"chainId\":false}", "expected uint32 or string");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: chainId", c, "{\"chainId\":\"0x1203030230\"}", "expected uint32 or string");
  TEST_ASSERT_CONFIGURE_FAIL("uinitialized chain: chainId", c, "{\"chainId\":0}", "chain corresponding to chain id not initialized!");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"chainId\":\"mainnet\"}");
  TEST_ASSERT_EQUAL(c->chain_id, 1);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"chainId\":5}");
  TEST_ASSERT_EQUAL(c->chain_id, ETH_CHAIN_ID_GOERLI);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"chainId\":\"0x2a\"}");
  TEST_ASSERT_EQUAL(c->chain_id, ETH_CHAIN_ID_KOVAN);

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
#ifndef POA
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"finality\":0}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"finality\":65535}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"finality\":\"0xffff\"}");
  TEST_ASSERT_EQUAL(c->finality, 65535);
#else
  c->chain_id = ETH_CHAIN_ID_GOERLI;
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: finality", c, "{\"finality\":101}", "expected %");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: finality", c, "{\"finality\":0}", "expected %");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"finality\":1}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"finality\":100}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"finality\":\"0x64\"}");
  TEST_ASSERT_EQUAL(c->finality, 100);
#endif

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: includeCode", c, "{\"includeCode\":1}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: includeCode", c, "{\"includeCode\":\"1\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: includeCode", c, "{\"includeCode\":\"0x00000\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"includeCode\":true}");
  TEST_ASSERT_EQUAL(FLAGS_INCLUDE_CODE, c->flags & FLAGS_INCLUDE_CODE);

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
  TEST_ASSERT_EQUAL(FLAGS_KEEP_IN3, c->flags & FLAGS_KEEP_IN3);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: key", c, "{\"key\":1}", "expected 256 bit data");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: key", c, "{\"key\":\"1\"}", "expected 256 bit data");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: key", c, "{\"key\":\"0x00000\"}", "expected 256 bit data");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"key\":\"0x1234567890123456789012345678901234567890123456789012345678901234\"}");
  bytes32_t b256;
  hex_to_bytes("0x1234567890123456789012345678901234567890123456789012345678901234", -1, b256, 32);
  TEST_ASSERT_EQUAL_MEMORY(c->key, b256, 32);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: useBinary", c, "{\"useBinary\":1}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: useBinary", c, "{\"useBinary\":\"1\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: useBinary", c, "{\"useBinary\":\"0x00000\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"useBinary\":true}");
  TEST_ASSERT_EQUAL(FLAGS_BINARY, c->flags & FLAGS_BINARY);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: useHttp", c, "{\"useHttp\":1}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: useHttp", c, "{\"useHttp\":\"1\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: useHttp", c, "{\"useHttp\":\"0x00000\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"useHttp\":true}");
  TEST_ASSERT_EQUAL(FLAGS_HTTP, c->flags & FLAGS_HTTP);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: stats", c, "{\"stats\":1}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: stats", c, "{\"stats\":\"1\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: stats", c, "{\"stats\":\"0x00000\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"stats\":false}");
  TEST_ASSERT_EQUAL(0, c->flags & FLAGS_STATS);

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

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: timeout", c, "{\"timeout\":\"-1\"}", "expected uint32");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: timeout", c, "{\"timeout\":\"\"}", "expected uint32");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: timeout", c, "{\"timeout\":\"0\"}", "expected uint32");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: timeout", c, "{\"timeout\":false}", "expected uint32");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: timeout", c, "{\"timeout\":\"0x1203030230\"}", "expected uint32");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"timeout\":1}");
  TEST_ASSERT_EQUAL(c->timeout, 1);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"timeout\":0}");
  TEST_ASSERT_EQUAL(c->timeout, 0);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"timeout\":4294967295}"); // UINT32_MAX
  TEST_ASSERT_EQUAL(c->timeout, 4294967295U);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"timeout\":\"0xffffffff\"}"); // UINT32_MAX
  TEST_ASSERT_EQUAL(c->timeout, 0xffffffff);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: minDeposit", c, "{\"minDeposit\":\"-1\"}", "expected uint64");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: minDeposit", c, "{\"minDeposit\":\"\"}", "expected uint64");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: minDeposit", c, "{\"minDeposit\":\"0\"}", "expected uint64");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: minDeposit", c, "{\"minDeposit\":false}", "expected uint64");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: minDeposit", c, "{\"minDeposit\":\"0x012345678901234567\"}", "expected uint64");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"minDeposit\":1}");
  TEST_ASSERT_EQUAL(c->min_deposit, 1);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"minDeposit\":0}");
  TEST_ASSERT_EQUAL(c->min_deposit, 0);
  // fixme:
  // TEST_ASSERT_CONFIGURE_PASS(c, "{\"minDeposit\":18446744073709551615}"); // UINT64_MAX
  // TEST_ASSERT_EQUAL(c->min_deposit, 18446744073709551615ULL);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"minDeposit\":\"0xffffffffffffffff\"}"); // UINT64_MAX
  TEST_ASSERT_EQUAL(c->min_deposit, 0xffffffffffffffff);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodeProps", c, "{\"nodeProps\":\"-1\"}", "expected uint64");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodeProps", c, "{\"nodeProps\":\"\"}", "expected uint64");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodeProps", c, "{\"nodeProps\":\"0\"}", "expected uint64");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodeProps", c, "{\"nodeProps\":false}", "expected uint64");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodeProps", c, "{\"nodeProps\":\"0x012345678901234567\"}", "expected uint64");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"nodeProps\":1}");
  TEST_ASSERT_EQUAL(c->node_props, 1);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"nodeProps\":0}");
  TEST_ASSERT_EQUAL(c->node_props, 0);
  // fixme:
  // TEST_ASSERT_CONFIGURE_PASS(c, "{\"nodeProps\":18446744073709551615}"); // UINT64_MAX
  // TEST_ASSERT_EQUAL(c->node_props, 18446744073709551615ULL);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"nodeProps\":\"0xffffffffffffffff\"}"); // UINT64_MAX
  TEST_ASSERT_EQUAL(c->node_props, 0xffffffffffffffff);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodeLimit", c, "{\"nodeLimit\":\"-1\"}", "expected uint16");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodeLimit", c, "{\"nodeLimit\":\"0x123412341234\"}", "expected uint16");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodeLimit", c, "{\"nodeLimit\":\"value\"}", "expected uint16");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodeLimit", c, "{\"nodeLimit\":65536}", "expected uint16");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"nodeLimit\":0}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"nodeLimit\":65535}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"nodeLimit\":\"0xffff\"}");
  TEST_ASSERT_EQUAL(c->max_attempts, 65535);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: proof", c, "{\"proof\":\"-1\"}", "expected values - full/standard/none");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: proof", c, "{\"proof\":\"0x123412341234\"}", "expected string");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched value: proof", c, "{\"proof\":\"fully\"}", "expected values - full/standard/none");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched value: proof", c, "{\"proof\":\"non\"}", "expected values - full/standard/none");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: proof", c, "{\"proof\":65536}", "expected string");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"proof\":\"standard\"}");
  TEST_ASSERT_EQUAL(c->proof, PROOF_STANDARD);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"proof\":\"none\"}");
  TEST_ASSERT_EQUAL(c->proof, PROOF_NONE);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"proof\":\"full\"}");
  TEST_ASSERT_EQUAL(c->proof, PROOF_FULL);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: replaceLatestBlock", c, "{\"replaceLatestBlock\":\"-1\"}", "expected uint8");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: replaceLatestBlock", c, "{\"replaceLatestBlock\":\"0x123412341234\"}", "expected uint8");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: replaceLatestBlock", c, "{\"replaceLatestBlock\":\"value\"}", "expected uint8");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: replaceLatestBlock", c, "{\"replaceLatestBlock\":65536}", "expected uint8");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"replaceLatestBlock\":0}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"replaceLatestBlock\":255}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"replaceLatestBlock\":\"0xff\"}");
  TEST_ASSERT_EQUAL(c->replace_latest_block, 255);
  TEST_ASSERT_EQUAL(in3_node_props_get(c->node_props, NODE_PROP_MIN_BLOCK_HEIGHT), c->replace_latest_block);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: requestCount", c, "{\"requestCount\":\"-1\"}", "expected uint8");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: requestCount", c, "{\"requestCount\":\"0x123412341234\"}", "expected uint8");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: requestCount", c, "{\"requestCount\":\"value\"}", "expected uint8");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: requestCount", c, "{\"requestCount\":65536}", "expected uint8");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"requestCount\":0}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"requestCount\":255}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"requestCount\":\"0xff\"}");
  TEST_ASSERT_EQUAL(c->request_count, 255);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: rpc", c, "{\"rpc\":false}", "expected string");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: rpc", c, "{\"rpc\":\"0x123412341234\"}", "expected string");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: rpc", c, "{\"rpc\":65536}", "expected string");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"rpc\":\"rpc.local\"}");
  TEST_ASSERT_EQUAL(c->proof, PROOF_NONE);
  TEST_ASSERT_EQUAL(c->chain_id, ETH_CHAIN_ID_LOCAL);
  TEST_ASSERT_EQUAL(c->request_count, 1);
  TEST_ASSERT_EQUAL_STRING(in3_find_chain(c, ETH_CHAIN_ID_LOCAL)->nodelist[0].url, "rpc.local");

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodes", c, "{\"nodes\":false}", "expected object");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodes", c, "{\"nodes\":\"0x123412341234\"}", "expected object");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodes", c, "{\"nodes\":65536}", "expected object");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"nodes\":{}}");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodes entry", c, "{\"nodes\":{\"n1\":{}}}", "expected hex str");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodes entry", c, "{\"nodes\":{\"0x1\":null}}", "expected object");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodes entry", c, "{\"nodes\":{\"0x1\":[]}}", "expected object");
  TEST_ASSERT_CONFIGURE_FAIL("empty node obj", c, "{\"nodes\":{\"0xf1\":{}}}", "invalid contract/registry!");
  TEST_ASSERT_CONFIGURE_FAIL("missing registry id", c, "{\"nodes\":{\"0xf1\":{\"contract\":\"0x0123456789012345678901234567890123456789\"}}}", "invalid contract/registry!");
  TEST_ASSERT_CONFIGURE_FAIL("missing contract", c, "{\"nodes\":{\"0xf1\":{\"registryId\":\"0x0123456789012345678901234567890123456789012345678901234567890123\"}}}", "invalid contract/registry!");
  TEST_ASSERT_CONFIGURE_FAIL("whiteListContract with manual whiteList",
                             c, "{"
                                "  \"nodes\":{"
                                "    \"0xdeaf\":{"
                                "      \"contract\":\"" CONTRACT_ADDRS "\","
                                "      \"registryId\":\"" REGISTRY_ID "\","
                                "      \"whiteListContract\":\"" WHITELIST_CONTRACT_ADDRS "\","
                                "      \"whiteList\":[\"0x0123456789012345678901234567890123456789\", \"0x1234567890123456789012345678901234567890\"],"
                                "    }"
                                "  }"
                                "}",
                             "cannot specify manual whiteList and whiteListContract together!");
  in3_free(c);

  c = in3_for_chain(0);
  TEST_ASSERT_CONFIGURE_PASS(c, "{"
                                "  \"servers\":{"
                                "    \"0xdeaf\":{"
                                "      \"contract\":\"" CONTRACT_ADDRS "\","
                                "      \"registryId\":\"" REGISTRY_ID "\","
                                "      \"whiteList\":[\"0x0123456789012345678901234567890123456789\", \"0x1234567890123456789012345678901234567890\"],"
                                "    }"
                                "  }"
                                "}");
  uint8_t wl[40];
  hex_to_bytes("0x01234567890123456789012345678901234567891234567890123456789012345678901234567890", -1, wl, 40);
  TEST_ASSERT_EQUAL_MEMORY(in3_find_chain(c, 0xdeaf)->whitelist->addresses.data, wl, 40);
  in3_free(c);

  c = in3_for_chain(0);
  TEST_ASSERT_CONFIGURE_FAIL("duplicate whiteList addresses",
                             c, "{"
                                "  \"servers\":{"
                                "    \"0xdeaf\":{"
                                "      \"contract\":\"" CONTRACT_ADDRS "\","
                                "      \"registryId\":\"" REGISTRY_ID "\","
                                "      \"whiteList\":[\"0x0123456789012345678901234567890123456789\", \"0x0123456789012345678901234567890123456789\"],"
                                "    }"
                                "  }"
                                "}",
                             "duplicate address!");
  in3_free(c);

  c = in3_for_chain(0);
  TEST_ASSERT_CONFIGURE_PASS(c, "{"
                                "  \"nodes\":{"
                                "    \"0xdeaf\":{"
                                "      \"contract\":\"" CONTRACT_ADDRS "\","
                                "      \"registryId\":\"" REGISTRY_ID "\","
                                "      \"whiteListContract\":\"" WHITELIST_CONTRACT_ADDRS "\","
                                "      \"nodeList\":[{"
                                "        \"url\":\"" NODE_URL "\","
                                "        \"props\":\"0xffff\","
                                "        \"address\":\"" NODE_ADDRS "\""
                                "      }],"
                                "      \"needsUpdate\":true,"
                                "      \"avgBlockTime\":7,"
                                "      \"verifiedHashes\":[{"
                                "        \"block\": \"0x234ad3\","
                                "        \"hash\": \"0x1230980495039470913820938019274231230980495039470913820938019274\""
                                "      },{"
                                "        \"block\": \"0x234a99\","
                                "        \"hash\": \"0xda879213bf9834ff2eade0921348dda879213bf9834ff2eade0921348d238130\""
                                "      }]"
                                "    }"
                                "  }"
                                "}");
  in3_chain_t* chain = in3_find_chain(c, 0xdeaf);
  TEST_ASSERT_NOT_NULL(chain);

  address_t addr;
  hex_to_bytes(WHITELIST_CONTRACT_ADDRS, -1, addr, 20);
  TEST_ASSERT_EQUAL_MEMORY(chain->whitelist->contract, addr, 20);
  hex_to_bytes(CONTRACT_ADDRS, -1, addr, 20);
  TEST_ASSERT_EQUAL_MEMORY(chain->contract->data, addr, 20);

  hex_to_bytes(REGISTRY_ID, -1, b256, 32);
  TEST_ASSERT_EQUAL_MEMORY(chain->registry_id, b256, 32);

  TEST_ASSERT_EQUAL(chain->nodelist_upd8_params != NULL, true);
  TEST_ASSERT_EQUAL_STRING(chain->nodelist[0].url, NODE_URL);
  TEST_ASSERT_EQUAL(chain->nodelist[0].props, 0xffff);
  hex_to_bytes(NODE_ADDRS, -1, addr, 20);
  TEST_ASSERT_EQUAL_MEMORY(chain->nodelist[0].address->data, addr, 20);

  TEST_ASSERT_EQUAL(0x234ad3, chain->verified_hashes[0].block_number);
  hex_to_bytes("0x1230980495039470913820938019274231230980495039470913820938019274", -1, b256, 32);
  TEST_ASSERT_EQUAL_MEMORY(chain->verified_hashes[0].hash, b256, 32);

  TEST_ASSERT_EQUAL(0x234a99, chain->verified_hashes[1].block_number);
  hex_to_bytes("0xda879213bf9834ff2eade0921348dda879213bf9834ff2eade0921348d238130", -1, b256, 32);
  TEST_ASSERT_EQUAL_MEMORY(chain->verified_hashes[1].hash, b256, 32);

  TEST_ASSERT_EQUAL(7, chain->avg_block_time);

  // test that all added nodes are marked as boot nodes
  for (int i = 0; i < chain->nodelist_length; ++i) {
    TEST_ASSERT_TRUE(!!(chain->nodelist[i].attrs & (1 << ATTR_BOOT_NODE)));
  }
  in3_free(c);
}

/*
 * Main
 */
int main() {
  _free(in3_create_signer(NULL, NULL, NULL));
  _free(in3_create_storage_handler(NULL, NULL, NULL, NULL));

  in3_log_set_quiet(true);
  in3_register_eth_basic();
  in3_register_eth_api();

  TESTS_BEGIN();
  RUN_TEST(test_configure_request);
  RUN_TEST(test_exec_req);
  RUN_TEST(test_configure);
  RUN_TEST(test_configure_validation);
  RUN_TEST(test_configure_signed_request);
  return TESTS_END();
}
