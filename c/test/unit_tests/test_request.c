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

#include "../../src/api/eth1/eth_api.h"
#include "../../src/core/client/context_internal.h"
#include "../../src/core/client/keys.h"
#include "../../src/core/util/bitset.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/log.h"
#include "../../src/signer/pk-signer/signer.h"
#include "../../src/verifier/eth1/basic/eth_basic.h"
#include "../test_utils.h"
#include "../util/transport.h"
#include "nodeselect/cache.h"
#include "nodeselect/nodelist.h"
#include "nodeselect/nodeselect_def.h"

#define TEST_ASSERT_CONFIGURE_FAIL(desc, in3, config, err_slice) \
  do {                                                           \
    char* _err_ = in3_configure(in3, config);                    \
    TEST_ASSERT_NOT_NULL(str_find(_err_, err_slice));            \
    free(_err_);                                                 \
  } while (0)
#define TEST_ASSERT_CONFIGURE_PASS(in3, config) \
  TEST_ASSERT_NULL(in3_configure(in3, config))
#define CONTRACT_ADDRS           "0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f"
#define REGISTRY_ID              "0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb"
#define WHITELIST_CONTRACT_ADDRS "0xdd80249a0631cf0f1593c7a9c9f9b8545e6c88ab"
#define NODE_URL                 "rpc.node"
#define NODE_ADDRS               "0x8904b9813c9ada123f9fccb9123659088dacd477"

static void test_configure_request() {
  in3_t* c = in3_for_chain(CHAIN_ID_MAINNET);
  TEST_ASSERT_NULL(in3_configure(c, "{\"autoUpdateList\":false,\"requestCount\":1,\"maxAttempts\":1,\"nodeRegistry\":{\"needsUpdate\":false}}"));
  c->proof                = PROOF_FULL;
  c->signature_count      = 2;
  c->finality             = 10;
  c->flags                = FLAGS_INCLUDE_CODE | FLAGS_BINARY | FLAGS_HTTP;
  c->replace_latest_block = 6;

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
  request_free(request);
  json_free(json);
  //  ctx_free(ctx);
  ctx_free(ctx);

  in3_free(c);
}

static void test_bulk_response() {
  in3_t* c = in3_for_chain(CHAIN_ID_MAINNET);
  TEST_ASSERT_NULL(in3_configure(c, "{\"autoUpdateList\":false,\"requestCount\":2,\"maxAttempts\":1,\"nodeRegistry\":{\"needsUpdate\":false}}"));
  c->flags = 0;

  //  add_response("eth_blockNumber", "[]", "0x2", NULL, NULL);
  in3_ctx_t* ctx = ctx_new(c, "[{\"method\":\"eth_blockNumber\",\"params\":[]},{\"method\":\"eth_blockNumber\",\"params\":[]}]");
  TEST_ASSERT_EQUAL(CTX_WAITING_TO_SEND, in3_ctx_exec_state(ctx));
  in3_request_t* req = in3_create_request(ctx);

  // first response is an error we expect a waiting since the transport has not passed all responses yet
  in3_ctx_add_response(req->ctx, 0, true, "500 from server", -1, 0);
  TEST_ASSERT_EQUAL(CTX_WAITING_FOR_RESPONSE, in3_ctx_exec_state(ctx));
  in3_ctx_add_response(req->ctx, 1, false, "[{\"result\":\"0x1\"},{\"result\":\"0x2\",\"in3\":{\"currentBlock\":\"0x1\"}}]", -1, 0);
  request_free(req);
  TEST_ASSERT_EQUAL(CTX_SUCCESS, in3_ctx_exec_state(ctx));

  char* res = ctx_get_response_data(ctx);
  TEST_ASSERT_EQUAL_STRING("[{\"result\":\"0x1\"},{\"result\":\"0x2\"}]", res);
  _free(res);

  ctx_free(ctx);
  in3_free(c);
}

static void test_configure_signed_request() {
  in3_t* c = in3_for_chain(CHAIN_ID_LOCAL);
  TEST_ASSERT_NULL(in3_configure(c, "{\"autoUpdateList\":false,\"requestCount\":1,\"maxAttempts\":1,\"nodeRegistry\":{\"needsUpdate\":false}}"));
  eth_register_pk_signer(c);
  char* err = in3_configure(c, "{\"key\":\"0x1234567890123456789012345678901234567890123456789012345678901234\"}");
  TEST_ASSERT_NULL_MESSAGE(err, err);
  c->flags = FLAGS_INCLUDE_CODE;

  in3_ctx_t* ctx = ctx_new(c, "{\"id\":2,\"method\":\"eth_blockNumber\",\"params\":[]}");
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
  TEST_ASSERT_EQUAL_STRING("8e39d2066cf9d1898e6bc9fbbfaa8fd6b9e5a86515e643f537c831982718866d0903e91f5f8824363dd3754fe550b37aa1e6eeb3742f13ad36d3321972e959a71c", hex);
  request_free(request);
  json_free(json);
  ctx_free(ctx);
  in3_free(c);
}

static void test_exec_req() {
  in3_t* c      = in3_for_chain(CHAIN_ID_MAINNET);
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
  TEST_ASSERT_EQUAL_STRING("{\"id\":0,\"jsonrpc\":\"2.0\",\"error\":{\"code\":-21,\"message\":\"no plugin found that handled the cache_clear action\"}}", result);
  _free(result);

  in3_free(c);
}

static void test_partial_response() {
  in3_t* c = in3_for_chain(CHAIN_ID_MAINNET);
  TEST_ASSERT_NULL(in3_configure(c, "{\"autoUpdateList\":false,\"requestCount\":3,\"maxAttempts\":1,\"nodeRegistry\":{\"needsUpdate\":false}}"));
  c->flags = 0;

  //  add_response("eth_blockNumber", "[]", "0x2", NULL, NULL);
  in3_ctx_t* ctx = ctx_new(c, "{\"method\":\"eth_blockNumber\",\"params\":[]}");
  TEST_ASSERT_EQUAL(IN3_WAITING, in3_ctx_execute(ctx));
  in3_request_t* req = in3_create_request(ctx);

  // first response is an error we expect a waiting since the transport has not passed all responses yet
  in3_ctx_add_response(req->ctx, 0, true, "500 from server", -1, 0);
  TEST_ASSERT_EQUAL(IN3_WAITING, in3_ctx_execute(ctx));
  TEST_ASSERT_EQUAL(IN3_WAITING, in3_ctx_execute(ctx));                               // calling twice will give the same result
  TEST_ASSERT_TRUE(get_node(in3_nodeselect_def_data(c), ctx->nodes)->blocked);        // first node is blacklisted
  TEST_ASSERT_FALSE(get_node(in3_nodeselect_def_data(c), ctx->nodes->next)->blocked); // second node is not blacklisted

  // now we have a valid response and should get a accaptable response
  in3_ctx_add_response(req->ctx, 2, false, "{\"result\":\"0x100\"}", -1, 0);
  TEST_ASSERT_EQUAL(IN3_OK, in3_ctx_execute(ctx));

  request_free(req);
  ctx_free(ctx);
  in3_free(c);
}

static void test_retry_response() {
  in3_t* c = in3_for_chain(CHAIN_ID_MAINNET);
  TEST_ASSERT_NULL(in3_configure(c, "{\"autoUpdateList\":false,\"requestCount\":2,\"nodeRegistry\":{\"needsUpdate\":false}}"));
  c->flags = 0;

  //  add_response("eth_blockNumber", "[]", "0x2", NULL, NULL);
  in3_ctx_t* ctx = ctx_new(c, "{\"method\":\"eth_blockNumber\",\"params\":[]}");
  TEST_ASSERT_EQUAL(IN3_WAITING, in3_ctx_execute(ctx));
  in3_request_t* req = in3_create_request(ctx);

  // first response is an error we expect a waiting since the transport has not passed all responses yet
  in3_ctx_add_response(req->ctx, 0, true, "500 from server", -1, 0);
  TEST_ASSERT_EQUAL(IN3_WAITING, in3_ctx_execute(ctx));                               // calling twice will give the same result
  TEST_ASSERT_TRUE(get_node(in3_nodeselect_def_data(c), ctx->nodes)->blocked);        // first node is blacklisted
  TEST_ASSERT_FALSE(get_node(in3_nodeselect_def_data(c), ctx->nodes->next)->blocked); // second node is not blacklisted
  TEST_ASSERT_NOT_NULL(ctx->raw_response);                                            // we still keep the raw response

  in3_ctx_add_response(req->ctx, 1, false, "{\"error\":\"Error:no internet\"}", -1, 0);
  TEST_ASSERT_EQUAL(IN3_WAITING, in3_ctx_execute(ctx));

  TEST_ASSERT_NULL(ctx->raw_response);
  request_free(req);

  // we must create a new request since this is a reattempt
  req = in3_create_request(ctx);
  TEST_ASSERT_NOT_NULL(ctx->raw_response); // now the raw response is set

  in3_ctx_add_response(req->ctx, 0, false, "{\"result\":\"0x100\"}", -1, 0);
  TEST_ASSERT_EQUAL(IN3_OK, in3_ctx_execute(ctx));

  request_free(req);
  ctx_free(ctx);
  in3_free(c);
}

static void test_configure() {
  in3_t* c   = in3_for_chain(CHAIN_ID_MAINNET);
  char*  tmp = NULL;

  // proof
  tmp = in3_configure(c, "{\"proof\":\"standard\"}");
  TEST_ASSERT_EQUAL(PROOF_STANDARD, c->proof);
  free(tmp);

  // rpc
  tmp = in3_configure(c, "{\"rpc\":\"http://rpc.slock.it\"}");
  TEST_ASSERT_EQUAL(PROOF_NONE, c->proof);
  TEST_ASSERT_EQUAL(CHAIN_ID_LOCAL, c->chain.chain_id);
  TEST_ASSERT_EQUAL(1, in3_get_nodelist(c)->request_count);
  TEST_ASSERT_EQUAL_STRING("http://rpc.slock.it", in3_nodeselect_def_data(c)->nodelist->url);
  free(tmp);

  // missing registryId and contract
  TEST_ASSERT_NOT_NULL((tmp = in3_configure(c, "{\"chainId\":\"0x3\",\"nodeRegistry\":{}}")));
  free(tmp);

  // bad registryId
  TEST_ASSERT_NOT_NULL((tmp = in3_configure(c, "{\"nodeRegistry\":{\"registryId\":\"0x987\"}}")));
  free(tmp);

  in3_free(c);
}

static void test_configure_validation() {
  in3_t*                   c = in3_for_chain(CHAIN_ID_MAINNET);
  in3_nodeselect_config_t* w = in3_get_nodelist(c);
  eth_register_pk_signer(c);

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
  TEST_ASSERT_CONFIGURE_FAIL("uninitialized chain: chainId", c, "{\"chainId\":0}", "chain corresponding to chain id not initialized!");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"chainId\":\"mainnet\"}");
  TEST_ASSERT_EQUAL(c->chain.chain_id, 1);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"chainId\":5}");
  TEST_ASSERT_EQUAL(c->chain.chain_id, CHAIN_ID_GOERLI);

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
  c->chain_id = CHAIN_ID_GOERLI;
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

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: bootWeights", c, "{\"bootWeights\":1}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: bootWeights", c, "{\"bootWeights\":\"1\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: bootWeights", c, "{\"bootWeights\":\"0x00000\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"bootWeights\":true}");
  TEST_ASSERT_EQUAL(FLAGS_BOOT_WEIGHTS, c->flags & FLAGS_BOOT_WEIGHTS);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"bootWeights\":false}");
  TEST_ASSERT_EQUAL(0, c->flags & FLAGS_BOOT_WEIGHTS);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxAttempts", c, "{\"maxAttempts\":\"-1\"}", "expected uint16");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxAttempts", c, "{\"maxAttempts\":\"0x123412341234\"}", "expected uint16");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxAttempts", c, "{\"maxAttempts\":\"value\"}", "expected uint16");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxAttempts", c, "{\"maxAttempts\":65536}", "expected uint16");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: maxAttempts", c, "{\"maxAttempts\":0}", "maxAttempts must be at least 1");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"maxAttempts\":1}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"maxAttempts\":65535}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"maxAttempts\":\"0xffff\"}");
  TEST_ASSERT_EQUAL(c->max_attempts, 65535);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: keepIn3", c, "{\"keepIn3\":1}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: keepIn3", c, "{\"keepIn3\":\"1\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: keepIn3", c, "{\"keepIn3\":\"0x00000\"}", "expected boolean");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"keepIn3\":true}");
  TEST_ASSERT_EQUAL(FLAGS_KEEP_IN3, c->flags & FLAGS_KEEP_IN3);

  bytes32_t b256;
  hex_to_bytes("0x1234567890123456789012345678901234567890123456789012345678901234", -1, b256, 32);

  /*
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: key", c, "{\"key\":1}", "expected 256 bit data");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: key", c, "{\"key\":\"1\"}", "expected 256 bit data");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: key", c, "{\"key\":\"0x00000\"}", "expected 256 bit data");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"key\":\"0x1234567890123456789012345678901234567890123456789012345678901234\"}");
  TEST_ASSERT_EQUAL_MEMORY(c->key, b256, 32);
*/
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
  TEST_ASSERT_EQUAL(w->min_deposit, 1);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"minDeposit\":0}");
  TEST_ASSERT_EQUAL(w->min_deposit, 0);
  // fixme:
  // TEST_ASSERT_CONFIGURE_PASS(c, "{\"minDeposit\":18446744073709551615}"); // UINT64_MAX
  // TEST_ASSERT_EQUAL(c->min_deposit, 18446744073709551615ULL);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"minDeposit\":\"0xffffffffffffffff\"}"); // UINT64_MAX
  TEST_ASSERT_EQUAL(w->min_deposit, 0xffffffffffffffff);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodeProps", c, "{\"nodeProps\":\"-1\"}", "expected uint64");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodeProps", c, "{\"nodeProps\":\"\"}", "expected uint64");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodeProps", c, "{\"nodeProps\":\"0\"}", "expected uint64");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodeProps", c, "{\"nodeProps\":false}", "expected uint64");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodeProps", c, "{\"nodeProps\":\"0x012345678901234567\"}", "expected uint64");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"nodeProps\":1}");
  TEST_ASSERT_EQUAL(w->node_props, 1);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"nodeProps\":0}");
  TEST_ASSERT_EQUAL(w->node_props, 0);
  // fixme:
  // TEST_ASSERT_CONFIGURE_PASS(c, "{\"nodeProps\":18446744073709551615}"); // UINT64_MAX
  // TEST_ASSERT_EQUAL(c->node_props, 18446744073709551615ULL);
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"nodeProps\":\"0xffffffffffffffff\"}"); // UINT64_MAX
  TEST_ASSERT_EQUAL(w->node_props, 0xffffffffffffffff);

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
  TEST_ASSERT_EQUAL(in3_node_props_get(w->node_props, NODE_PROP_MIN_BLOCK_HEIGHT), c->replace_latest_block);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: requestCount", c, "{\"requestCount\":\"-1\"}", "expected uint8");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: requestCount", c, "{\"requestCount\":\"0x123412341234\"}", "expected uint8");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: requestCount", c, "{\"requestCount\":\"value\"}", "expected uint8");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: requestCount", c, "{\"requestCount\":65536}", "expected uint8");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: requestCount", c, "{\"requestCount\":0}", "requestCount must be at least 1");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"requestCount\":1}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"requestCount\":255}");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"requestCount\":\"0xff\"}");
  TEST_ASSERT_EQUAL(w->request_count, 255);

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: rpc", c, "{\"rpc\":false}", "expected string");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: rpc", c, "{\"rpc\":\"0x123412341234\"}", "expected string");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: rpc", c, "{\"rpc\":65536}", "expected string");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"rpc\":\"rpc.local\"}");
  TEST_ASSERT_EQUAL(c->proof, PROOF_NONE);
  TEST_ASSERT_EQUAL(c->chain.chain_id, CHAIN_ID_LOCAL);
  TEST_ASSERT_EQUAL(w->request_count, 1);
  TEST_ASSERT_EQUAL_STRING(w->data->nodelist[0].url, "rpc.local");

  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodeRegistry", c, "{\"nodeRegistry\":false}", "expected object");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodeRegistry", c, "{\"nodeRegistry\":\"0x123412341234\"}", "expected object");
  TEST_ASSERT_CONFIGURE_FAIL("mismatched type: nodeRegistry", c, "{\"nodeRegistry\":65536}", "expected object");
  TEST_ASSERT_CONFIGURE_PASS(c, "{\"chainId\":\"0x1\",\"nodeRegistry\":{}}");
  TEST_ASSERT_CONFIGURE_FAIL("whiteListContract with manual whiteList",
                             c, "{"
                                "  \"chainId\":\"0xdeaf\","
                                "  \"nodeRegistry\":{"
                                "      \"contract\":\"" CONTRACT_ADDRS "\","
                                "      \"registryId\":\"" REGISTRY_ID "\","
                                "      \"whiteListContract\":\"" WHITELIST_CONTRACT_ADDRS "\","
                                "      \"whiteList\":[\"0x0123456789012345678901234567890123456789\", \"0x1234567890123456789012345678901234567890\"],"
                                "  }"
                                "}",
                             "cannot specify manual whiteList and whiteListContract together!");
  in3_free(c);

  c = in3_for_chain(CHAIN_ID_LOCAL);
  TEST_ASSERT_CONFIGURE_PASS(c, "{"
                                "  \"nodeRegistry\":{"
                                "      \"contract\":\"" CONTRACT_ADDRS "\","
                                "      \"registryId\":\"" REGISTRY_ID "\","
                                "      \"whiteList\":[\"0x0123456789012345678901234567890123456789\", \"0x1234567890123456789012345678901234567890\"],"
                                "  }"
                                "}");
  uint8_t wl[40];
  hex_to_bytes("0x01234567890123456789012345678901234567891234567890123456789012345678901234567890", -1, wl, 40);
  TEST_ASSERT_EQUAL_MEMORY(in3_nodeselect_def_data(c)->whitelist->addresses.data, wl, 40);
  in3_free(c);

  c = in3_for_chain(0);
  TEST_ASSERT_CONFIGURE_FAIL("duplicate whiteList addresses",
                             c, "{"
                                "  \"nodeRegistry\":{"
                                "      \"contract\":\"" CONTRACT_ADDRS "\","
                                "      \"registryId\":\"" REGISTRY_ID "\","
                                "      \"whiteList\":[\"0x0123456789012345678901234567890123456789\", \"0x0123456789012345678901234567890123456789\"],"
                                "  }"
                                "}",
                             "duplicate address!");
  in3_free(c);

  c = in3_for_chain(0);
  TEST_ASSERT_CONFIGURE_PASS(c, "{"
                                "  \"chainId\":\"0xdeaf\","
                                "  \"verifiedHashes\":[{"
                                "    \"block\": \"0x234ad3\","
                                "    \"hash\": \"0x1230980495039470913820938019274231230980495039470913820938019274\""
                                "  },{"
                                "    \"block\": \"0x234a99\","
                                "    \"hash\": \"0xda879213bf9834ff2eade0921348dda879213bf9834ff2eade0921348d238130\""
                                "  }],"
                                "  \"nodeRegistry\":{"
                                "      \"contract\":\"" CONTRACT_ADDRS "\","
                                "      \"registryId\":\"" REGISTRY_ID "\","
                                "      \"whiteListContract\":\"" WHITELIST_CONTRACT_ADDRS "\","
                                "      \"nodeList\":[{"
                                "        \"url\":\"" NODE_URL "\","
                                "        \"props\":\"0xffff\","
                                "        \"address\":\"" NODE_ADDRS "\""
                                "      }],"
                                "      \"needsUpdate\":true,"
                                "      \"avgBlockTime\":7"
                                "  }"
                                "}");
  in3_nodeselect_def_t* nl = in3_nodeselect_def_data(c);
  TEST_ASSERT_NOT_NULL(nl);

  address_t addr;
  hex_to_bytes(WHITELIST_CONTRACT_ADDRS, -1, addr, 20);
  TEST_ASSERT_EQUAL_MEMORY(nl->whitelist->contract, addr, 20);
  hex_to_bytes(CONTRACT_ADDRS, -1, addr, 20);
  TEST_ASSERT_EQUAL_MEMORY(nl->contract, addr, 20);

  hex_to_bytes(REGISTRY_ID, -1, b256, 32);
  TEST_ASSERT_EQUAL_MEMORY(nl->registry_id, b256, 32);

  TEST_ASSERT_EQUAL(nl->nodelist_upd8_params != NULL, true);
  TEST_ASSERT_EQUAL_STRING(nl->nodelist[0].url, NODE_URL);
  TEST_ASSERT_EQUAL(nl->nodelist[0].props, 0xffff);
  hex_to_bytes(NODE_ADDRS, -1, addr, 20);
  TEST_ASSERT_EQUAL_MEMORY(nl->nodelist[0].address, addr, 20);

  TEST_ASSERT_EQUAL(0x234ad3, c->chain.verified_hashes[0].block_number);
  hex_to_bytes("0x1230980495039470913820938019274231230980495039470913820938019274", -1, b256, 32);
  TEST_ASSERT_EQUAL_MEMORY(c->chain.verified_hashes[0].hash, b256, 32);

  TEST_ASSERT_EQUAL(0x234a99, c->chain.verified_hashes[1].block_number);
  hex_to_bytes("0xda879213bf9834ff2eade0921348dda879213bf9834ff2eade0921348d238130", -1, b256, 32);
  TEST_ASSERT_EQUAL_MEMORY(c->chain.verified_hashes[1].hash, b256, 32);

  TEST_ASSERT_EQUAL(7, nl->avg_block_time);

  // test that all added nodes are marked as boot nodes
  for (int i = 0; i < nl->nodelist_length; ++i) {
    TEST_ASSERT_TRUE(!!(nl->nodelist[i].attrs & (1 << ATTR_BOOT_NODE)));
  }
  in3_free(c);
}

#define ADD_RESPONSE_SIGS(sigs) add_response("eth_getTransactionByHash",                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
                                             "[\"0x715ece6967d0dc6aa6e8e4ee83937d3d4a79fdc644b64f07aa72f877df156be7\"]",                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
                                             "{"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
                                             "    \"blockHash\":\"0x3b1d2d185af8856ae03743b632ce1ed2c949e5d857870b7dae15f5b0601efff7\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
                                             "    \"blockNumber\":\"0x37e13e\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
                                             "    \"chainId\":\"0x5\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
                                             "    \"creates\":null,"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
                                             "    \"from\":\"0xe6ed92d26573c67af5eca7fb2a49a807fb8f88db\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
                                             "    \"gas\":\"0x7a120\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
                                             "    \"gasPrice\":\"0xf695\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
                                             "    \"hash\":\"0x715ece6967d0dc6aa6e8e4ee83937d3d4a79fdc644b64f07aa72f877df156be7\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
                                             "    \"input\":\"0x205c287800000000000000000000000030666f5ef83df74980b30e36bc2f65478e9f78d9000000000000000000000000000000000000000000000001c9f78d2893e40000\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
                                             "    \"nonce\":\"0xdb68\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
                                             "    \"publicKey\":\"0xbc40a37d5ba777b3a681ff12fa77bd34d85a11f1e1bc5b3bd2bbced17d49fdfd7095d2344ae3a62dbe253c41ca1d6733c4d59b80d0b20eeb9717190d74390be3\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
                                             "    \"r\":\"0x59c2d85d4ca60d6d0c1c1d29da1d07b1ed7190a02be72f386262baf48c5050f2\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
                                             "    \"raw\":\"0xf8a982db6882f6958307a120947abc5f3976a239fd4addb73e2d43449038a7cf2c80b844205c287800000000000000000000000030666f5ef83df74980b30e36bc2f65478e9f78d9000000000000000000000000000000000000000000000001c9f78d2893e400002ea059c2d85d4ca60d6d0c1c1d29da1d07b1ed7190a02be72f386262baf48c5050f2a01d32a1bc5b07edfcc6a42a87841c8097cbed2ae76c90c2367396f8609582cdda\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
                                             "    \"s\":\"0x1d32a1bc5b07edfcc6a42a87841c8097cbed2ae76c90c2367396f8609582cdda\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
                                             "    \"standardV\":\"0x1\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
                                             "    \"to\":\"0x7abc5f3976a239fd4addb73e2d43449038a7cf2c\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
                                             "    \"transactionIndex\":\"0x1\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
                                             "    \"v\":\"0x2e\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
                                             "    \"value\":\"0x0\""                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
                                             "}",                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
                                             NULL,                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
                                             "{"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
                                             "    \"proof\":{"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
                                             "      \"type\":\"transactionProof\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
                                             "      \"block\":\"0xf9025ca028ec80e2ad66d03b158eb3fce8eed394bd90649551a0e2cca1d22d11e90d3655a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347940000000000000000000000000000000000000000a0ce1395a999dbe04c2090cd7e6440d7272d6f4dad2543989d164ae2a7e50e9391a0157c899d5554d0c3b513d16836cecd040a40c80c77e5a5fd8bfe961823bc97d2a0d61a06c53c3f935052b4dfa7f56b9a9cfc4356eb9abdd931c9a33a07c43b0652b9010000000000000000000000000000000080000040500000000001000000008080000000000001000220000000000000000000004000000040800000400000000002002000000000000000400808000000000004000000000000000000000040000000001000000000200080000000008000000000400000000000000010000000000000000000000000000000000400001000000000100000000000000000000100000000000000000000000000002100000000000000000001000000000000000000000002000000000000000000000008000000000040000000000000000000000000000000000000000000000000000000000000001000030002000000002000018337e13e837a1200834bf487845f9b2a24b861476f65726c6920496e697469617469766520417574686f726974790000000000227e77a2cddac26649a6b765e817ce6275a104da4c76215f15498edc0247679129cde07d0da98f595bc0aa8c10db10c427d74cc8c828a236e2755362fd80844800a00000000000000000000000000000000000000000000000000000000000000000880000000000000000\"," \
                                             "      \"merkleProof\":[\"0xf851a0cc1732b8ace8b4d42b392e17ddd5f4a110a03de206a85882d796dc32cfaf299e80808080808080a096eb62167a629b538b74ba420941f9ce6e928412beee3f241c2982bb12afd7e18080808080808080\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
                                             "      \"0xf85180a0a3147f39b9ae6fff129b790c82ad9513606c8bc6f947e1acf2279960b536d323a0fedd07567a67ab98651732388ac714f89981dfcb6689e915909ebd1d157ee7558080808080808080808080808080\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
                                             "      \"0xf8ae20b8abf8a982db6882f6958307a120947abc5f3976a239fd4addb73e2d43449038a7cf2c80b844205c287800000000000000000000000030666f5ef83df74980b30e36bc2f65478e9f78d9000000000000000000000000000000000000000000000001c9f78d2893e400002ea059c2d85d4ca60d6d0c1c1d29da1d07b1ed7190a02be72f386262baf48c5050f2a01d32a1bc5b07edfcc6a42a87841c8097cbed2ae76c90c2367396f8609582cdda\"],"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
                                             "      \"txIndex\":1,"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
                                             "      \"signatures\":" sigs ""                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
                                             "    },"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
                                             "    \"version\":\"2.1.0\","                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
                                             "    \"currentBlock\":3662181,"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
                                             "    \"lastValidatorChange\":0,"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
                                             "    \"lastNodeList\":3489472,"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
                                             "    \"execTime\":50,"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
                                             "    \"rpcTime\":45,"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
                                             "    \"rpcCount\":2"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
                                             "}")

static void test_parallel_signatures() {
  in3_t* in3 = in3_for_chain(0x5);
  // note: maxVerifiedHashes must be zero for this test to work, since we would like to reuse the above response.
  in3_configure(in3, "{\"chainId\":\"0x34ff\",\"chainType\":0,\"autoUpdateList\":false,\"signatureCount\":3,\"requestCount\":1,\"maxAttempts\":1,\"maxVerifiedHashes\":0,"
                     "\"nodeRegistry\":{"
                     "   \"needsUpdate\":false,"
                     "   \"contract\": \"0x5f51e413581dd76759e9eed51e63d14c8d1379c8\","
                     "   \"registryId\": \"0x67c02e5e272f9d6b4a33716614061dd298283f86351079ef903bf0d4410a44ea\","
                     "   \"nodeList\": [{"
                     "      \"url\":\"https://in3-v2.slock.it/goerli/nd-1\","
                     "      \"address\":\"0x45d45e6ff99e6c34a235d263965910298985fcfe\","
                     "      \"props\":\"0x1dd\""
                     "    },"
                     "    {"
                     "      \"url\":\"https://in3-v2.slock.it/goerli/nd-2\","
                     "      \"address\":\"0x1fe2e9bf29aa1938859af64c413361227d04059a\","
                     "      \"props\":\"0x1dd\""
                     "    },"
                     "    {"
                     "      \"url\":\"https://in3-v2.slock.it/goerli/nd-3\","
                     "      \"address\":\"0x945f75c0408c0026a3cd204d36f5e47745182fd4\","
                     "      \"props\":\"0x1dd\""
                     "    },"
                     "    {"
                     "      \"url\":\"https://in3-v2.slock.it/goerli/nd-4\","
                     "      \"address\":\"0xc513a534de5a9d3f413152c41b09bd8116237fc8\","
                     "      \"props\":\"0x1dd\""
                     "    },"
                     "    {"
                     "      \"url\":\"https://in3-v2.slock.it/goerli/nd-5\","
                     "      \"address\":\"0xbcdf4e3e90cc7288b578329efd7bcc90655148d2\","
                     "      \"props\":\"0x1dd\""
                     "    },"
                     "    {"
                     "      \"url\":\"https://tincubeth.komputing.org/\","
                     "      \"address\":\"0xf944d416ebdf7f6e22eaf79a5a53ad1a487ddd9a\","
                     "      \"props\":\"0x1d7e0000000a\""
                     "    },"
                     "    {"
                     "      \"url\":\"https://h5l45fkzz7oc3gmb.onion/\","
                     "      \"address\":\"0x56d986deb3b5d14cb230d0f39247cc32416020b6\","
                     "      \"props\":\"0x21660000000a\""
                     "    },"
                     "    {"
                     "      \"url\":\"https://in3node.com\","
                     "      \"address\":\"0x1821354870a09e3c4d2ed1a5c4b481e38e3d6ba1\","
                     "      \"props\":\"0xa000001d1\""
                     "    }]"
                     "}}");
  register_transport(in3, test_transport);

  ADD_RESPONSE_SIGS("[{"
                    "  \"blockHash\": \"0x3b1d2d185af8856ae03743b632ce1ed2c949e5d857870b7dae15f5b0601efff7\","
                    "  \"block\": 3662142,"
                    "  \"r\": \"0x389656b8924ab3b0f05b5d618e14a6b561cc85023bde9a96f1b78487eb1872a4\","
                    "  \"s\": \"0x49c00342564b30e8b17488941aa3afde8bc85728d2a2814094c0afb7ce84c926\","
                    "  \"v\": 28,"
                    "  \"msgHash\": \"0x2367ad2a1ff16e8af634f6e1062b0954f6898b5340d849b8b5b79bc936b57950\""
                    "}, {"
                    "  \"blockHash\": \"0x3b1d2d185af8856ae03743b632ce1ed2c949e5d857870b7dae15f5b0601efff7\","
                    "  \"block\": 3662142,"
                    "  \"r\": \"0xdf49a129f1186ad491d66ef8100384799cbd583d59558092375c9e181ceddcec\","
                    "  \"s\": \"0x158e543243a2e4718cc253d389759b9382c040f5e77ae360531d7ff701537802\","
                    "  \"v\": 28,"
                    "  \"msgHash\": \"0x2367ad2a1ff16e8af634f6e1062b0954f6898b5340d849b8b5b79bc936b57950\""
                    "}, {"
                    "  \"blockHash\": \"0x3b1d2d185af8856ae03743b632ce1ed2c949e5d857870b7dae15f5b0601efff7\","
                    "  \"block\": 3662142,"
                    "  \"r\": \"0xe6ba447671e5f2c705d757111f0576e3109ec0b4b24540c74174dc101c6da2da\","
                    "  \"s\": \"0x61bdefd67dad9d6c2d07903b9a45072cd7a967b96139b9187307f05acbe0872a\","
                    "  \"v\": 27,"
                    "  \"msgHash\": \"0x2367ad2a1ff16e8af634f6e1062b0954f6898b5340d849b8b5b79bc936b57950\""
                    "}]");

  ADD_RESPONSE_SIGS("[{"
                    "  \"blockHash\": \"0x3b1d2d185af8856ae03743b632ce1ed2c949e5d857870b7dae15f5b0601efff7\","
                    "  \"block\": 3662142,"
                    "  \"r\": \"0x389656b8924ab3b0f05b5d618e14a6b561cc85023bde9a96f1b78487eb1872a4\","
                    "  \"s\": \"0x49c00342564b30e8b17488941aa3afde8bc85728d2a2814094c0afb7ce84c926\","
                    "  \"v\": 28,"
                    "  \"msgHash\": \"0x2367ad2a1ff16e8af634f6e1062b0954f6898b5340d849b8b5b79bc936b57950\""
                    "}, {"
                    "  \"blockHash\": \"0x3b1d2d185af8856ae03743b632ce1ed2c949e5d857870b7dae15f5b0601efff7\","
                    "  \"block\": 3662142,"
                    "  \"r\": \"0xe6ba447671e5f2c705d757111f0576e3109ec0b4b24540c74174dc101c6da2da\","
                    "  \"s\": \"0x61bdefd67dad9d6c2d07903b9a45072cd7a967b96139b9187307f05acbe0872a\","
                    "  \"v\": 27,"
                    "  \"msgHash\": \"0x2367ad2a1ff16e8af634f6e1062b0954f6898b5340d849b8b5b79bc936b57950\""
                    "}]");

  // we ask nd-1 and nd-5 for signatures of 3 nodes - nd-2, nd-3 & nd-4.
  // nd-1's response is missing a signature from nd-4, therefore we mark nd-4 as offline.
  // nd-5's response however has all 3 signatures, so we accept this response.
  in3_ctx_t* ctx = in3_client_rpc_ctx_raw(in3, "{\"jsonrpc\":\"2.0\","
                                               "\"method\":\"eth_getTransactionByHash\","
                                               "\"params\":[\"0x715ece6967d0dc6aa6e8e4ee83937d3d4a79fdc644b64f07aa72f877df156be7\"],"
                                               "\"in3\":{\"dataNodes\":[\"0x45d45e6ff99e6c34a235d263965910298985fcfe\", \"0xbcdf4e3e90cc7288b578329efd7bcc90655148d2\"],"
                                               "\"signerNodes\":[\"0x1fe2e9bf29aa1938859af64c413361227d04059a\",\"0x945f75c0408c0026a3cd204d36f5e47745182fd4\",\"0xc513a534de5a9d3f413152c41b09bd8116237fc8\"]}}");

  in3_nodeselect_def_t* nl = in3_nodeselect_def_data(in3);
  TEST_ASSERT_FALSE(is_blacklisted(&nl->nodelist[3]));

  bytes_t* address = hex_to_new_bytes("45d45e6ff99e6c34a235d263965910298985fcfe", 40);
  TEST_ASSERT_EQUAL_MEMORY(nl->offlines->reporter, address->data, 20);
  b_free(address);

  address = hex_to_new_bytes("c513a534de5a9d3f413152c41b09bd8116237fc8", 40);
  TEST_ASSERT_EQUAL_MEMORY(nl->offlines->offline->address, address->data, 20);
  b_free(address);
  ctx_free(ctx);

  ADD_RESPONSE_SIGS("[{"
                    "  \"blockHash\": \"0x3b1d2d185af8856ae03743b632ce1ed2c949e5d857870b7dae15f5b0601efff7\","
                    "  \"block\": 3662142,"
                    "  \"r\": \"0x389656b8924ab3b0f05b5d618e14a6b561cc85023bde9a96f1b78487eb1872a4\","
                    "  \"s\": \"0x49c00342564b30e8b17488941aa3afde8bc85728d2a2814094c0afb7ce84c926\","
                    "  \"v\": 28,"
                    "  \"msgHash\": \"0x2367ad2a1ff16e8af634f6e1062b0954f6898b5340d849b8b5b79bc936b57950\""
                    "}, {"
                    "  \"blockHash\": \"0x3b1d2d185af8856ae03743b632ce1ed2c949e5d857870b7dae15f5b0601efff7\","
                    "  \"block\": 3662142,"
                    "  \"r\": \"0xe6ba447671e5f2c705d757111f0576e3109ec0b4b24540c74174dc101c6da2da\","
                    "  \"s\": \"0x61bdefd67dad9d6c2d07903b9a45072cd7a967b96139b9187307f05acbe0872a\","
                    "  \"v\": 27,"
                    "  \"msgHash\": \"0x2367ad2a1ff16e8af634f6e1062b0954f6898b5340d849b8b5b79bc936b57950\""
                    "}]");

  // now, we ask tincubeth node for the signatures from the same 3 nodes.
  // again, we are missing a signature from nd-4, therefore we blacklist it.
  ctx = in3_client_rpc_ctx_raw(in3, "{\"jsonrpc\":\"2.0\","
                                    "\"method\":\"eth_getTransactionByHash\","
                                    "\"params\":[\"0x715ece6967d0dc6aa6e8e4ee83937d3d4a79fdc644b64f07aa72f877df156be7\"],"
                                    "\"in3\":{\"dataNodes\":[\"0xf944d416ebdf7f6e22eaf79a5a53ad1a487ddd9a\"],"
                                    "\"signerNodes\":[\"0x1fe2e9bf29aa1938859af64c413361227d04059a\",\"0x945f75c0408c0026a3cd204d36f5e47745182fd4\",\"0xc513a534de5a9d3f413152c41b09bd8116237fc8\"]}}");

  nl = in3_nodeselect_def_data(in3);
  TEST_ASSERT_TRUE(is_blacklisted(&nl->nodelist[3]));
  TEST_ASSERT_NULL(nl->offlines);
  ctx_free(ctx);

  in3_free(in3);
}

static void test_sigs() {
  in3_t* in3 = in3_for_chain(0x5);
  // note: maxVerifiedHashes must be zero for this test to work, since we would like to reuse the above response.
  in3_configure(in3, "{\"chainId\":\"0x34ff\",\"chainType\":0,\"autoUpdateList\":false,\"signatureCount\":3,\"requestCount\":1,\"maxAttempts\":1,\"maxVerifiedHashes\":0,"
                     "\"nodeRegistry\":{"
                     "   \"needsUpdate\":false,"
                     "   \"contract\": \"0x5f51e413581dd76759e9eed51e63d14c8d1379c8\","
                     "   \"registryId\": \"0x67c02e5e272f9d6b4a33716614061dd298283f86351079ef903bf0d4410a44ea\","
                     "   \"nodeList\": [{"
                     "      \"url\":\"https://in3-v2.slock.it/goerli/nd-1\","
                     "      \"address\":\"0x45d45e6ff99e6c34a235d263965910298985fcfe\","
                     "      \"props\":\"0x1dd\""
                     "    },"
                     "    {"
                     "      \"url\":\"https://in3-v2.slock.it/goerli/nd-2\","
                     "      \"address\":\"0x1fe2e9bf29aa1938859af64c413361227d04059a\","
                     "      \"props\":\"0x1dd\""
                     "    },"
                     "    {"
                     "      \"url\":\"https://in3-v2.slock.it/goerli/nd-3\","
                     "      \"address\":\"0x945f75c0408c0026a3cd204d36f5e47745182fd4\","
                     "      \"props\":\"0x1dd\""
                     "    },"
                     "    {"
                     "      \"url\":\"https://in3-v2.slock.it/goerli/nd-4\","
                     "      \"address\":\"0xc513a534de5a9d3f413152c41b09bd8116237fc8\","
                     "      \"props\":\"0x1dd\""
                     "    },"
                     "    {"
                     "      \"url\":\"https://in3-v2.slock.it/goerli/nd-5\","
                     "      \"address\":\"0xbcdf4e3e90cc7288b578329efd7bcc90655148d2\","
                     "      \"props\":\"0x1dd\""
                     "    },"
                     "    {"
                     "      \"url\":\"https://tincubeth.komputing.org/\","
                     "      \"address\":\"0xf944d416ebdf7f6e22eaf79a5a53ad1a487ddd9a\","
                     "      \"props\":\"0x1d7e0000000a\""
                     "    },"
                     "    {"
                     "      \"url\":\"https://h5l45fkzz7oc3gmb.onion/\","
                     "      \"address\":\"0x56d986deb3b5d14cb230d0f39247cc32416020b6\","
                     "      \"props\":\"0x21660000000a\""
                     "    },"
                     "    {"
                     "      \"url\":\"https://in3node.com\","
                     "      \"address\":\"0xd4f40cb00b0620fbf3a546feb68a4496482d89a4\","
                     "      \"props\":\"0xa000001d1\""
                     "    }]"
                     "}}");
  register_transport(in3, test_transport);

  ADD_RESPONSE_SIGS("[{"
                    "  \"blockHash\": \"0x3b1d2d185af8856ae03743b632ce1ed2c949e5d857870b7dae15f5b0601efff7\","
                    "  \"block\": 3662142,"
                    "  \"r\": \"0x389656b8924ab3b0f05b5d618e14a6b561cc85023bde9a96f1b78487eb1872a4\","
                    "  \"s\": \"0x49c00342564b30e8b17488941aa3afde8bc85728d2a2814094c0afb7ce84c926\","
                    "  \"v\": 28,"
                    "  \"msgHash\": \"0x2367ad2a1ff16e8af634f6e1062b0954f6898b5340d849b8b5b79bc936b57950\""
                    "}, {"
                    "  \"blockHash\": \"0x3b1d2d185af8856ae03743b632ce1ed2c949e5d857870b7dae15f5b0601efff7\","
                    "  \"block\": 3662142,"
                    "  \"r\": \"0xdf49a129f1186ad491d66ef8100384799cbd583d59558092375c9e181ceddcec\","
                    "  \"s\": \"0x158e543243a2e4718cc253d389759b9382c040f5e77ae360531d7ff701537802\","
                    "  \"v\": 28,"
                    "  \"msgHash\": \"0x2367ad2a1ff16e8af634f6e1062b0954f6898b5340d849b8b5b79bc936b57950\""
                    "}, {"
                    "  \"blockHash\": \"0x3b1d2d185af8856ae03743b632ce1ed2c949e5d857870b7dae15f5b0601efff7\","
                    "  \"block\": 3662142,"
                    "  \"r\": \"0xe6ba447671e5f2c705d757111f0576e3109ec0b4b24540c74174dc101c6da2da\","
                    "  \"s\": \"0x61bdefd67dad9d6c2d07903b9a45072cd7a967b96139b9187307f05acbe0872a\","
                    "  \"v\": 27,"
                    "  \"msgHash\": \"0x2367ad2a1ff16e8af634f6e1062b0954f6898b5340d849b8b5b79bc936b57950\""
                    "}]");

  ADD_RESPONSE_SIGS("[{"
                    "  \"blockHash\": \"0x3b1d2d185af8856ae03743b632ce1ed2c949e5d857870b7dae15f5b0601efff7\","
                    "  \"block\": 3662142,"
                    "  \"r\": \"0x389656b8924ab3b0f05b5d618e14a6b561cc85023bde9a96f1b78487eb1872a4\","
                    "  \"s\": \"0x49c00342564b30e8b17488941aa3afde8bc85728d2a2814094c0afb7ce84c926\","
                    "  \"v\": 28,"
                    "  \"msgHash\": \"0x2367ad2a1ff16e8af634f6e1062b0954f6898b5340d849b8b5b79bc936b57950\""
                    "}, {"
                    "  \"error\": {"
                    "    \"message\": \"Internal Error\","
                    "    \"code\": -32603,"
                    "    \"data\": { \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\" }"
                    "  }"
                    "}, {"
                    "  \"blockHash\": \"0x3b1d2d185af8856ae03743b632ce1ed2c949e5d857870b7dae15f5b0601efff7\","
                    "  \"block\": 3662142,"
                    "  \"r\": \"0xe6ba447671e5f2c705d757111f0576e3109ec0b4b24540c74174dc101c6da2da\","
                    "  \"s\": \"0x61bdefd67dad9d6c2d07903b9a45072cd7a967b96139b9187307f05acbe0872a\","
                    "  \"v\": 27,"
                    "  \"msgHash\": \"0x2367ad2a1ff16e8af634f6e1062b0954f6898b5340d849b8b5b79bc936b57950\""
                    "}]");

  // we ask nd-1 and nd-5 for signatures of 3 nodes - nd-2, nd-3 & nd-4.
  // nd-1's response is missing a signature from nd-4, therefore we mark nd-4 as offline.
  // nd-5's response however has all 3 signatures, so we accept this response.
  in3_ctx_t* ctx = in3_client_rpc_ctx_raw(in3, "{\"jsonrpc\":\"2.0\","
                                               "\"method\":\"eth_getTransactionByHash\","
                                               "\"params\":[\"0x715ece6967d0dc6aa6e8e4ee83937d3d4a79fdc644b64f07aa72f877df156be7\"],"
                                               "\"in3\":{\"dataNodes\":[\"0x45d45e6ff99e6c34a235d263965910298985fcfe\", \"0xbcdf4e3e90cc7288b578329efd7bcc90655148d2\"],"
                                               "\"signerNodes\":[\"0x1fe2e9bf29aa1938859af64c413361227d04059a\",\"0x945f75c0408c0026a3cd204d36f5e47745182fd4\",\"0xc513a534de5a9d3f413152c41b09bd8116237fc8\"]}}");

  in3_nodeselect_def_t* nl = in3_nodeselect_def_data(in3);
  TEST_ASSERT_FALSE(is_blacklisted(&nl->nodelist[3]));

  bytes_t* address = hex_to_new_bytes("45d45e6ff99e6c34a235d263965910298985fcfe", 40);
  TEST_ASSERT_EQUAL_MEMORY(nl->offlines->reporter, address->data, 20);
  b_free(address);

  address = hex_to_new_bytes("c513a534de5a9d3f413152c41b09bd8116237fc8", 40);
  TEST_ASSERT_EQUAL_MEMORY(nl->offlines->offline->address, address->data, 20);
  b_free(address);
  ctx_free(ctx);

  ADD_RESPONSE_SIGS("[{"
                    "  \"blockHash\": \"0x3b1d2d185af8856ae03743b632ce1ed2c949e5d857870b7dae15f5b0601efff7\","
                    "  \"block\": 3662142,"
                    "  \"r\": \"0x389656b8924ab3b0f05b5d618e14a6b561cc85023bde9a96f1b78487eb1872a4\","
                    "  \"s\": \"0x49c00342564b30e8b17488941aa3afde8bc85728d2a2814094c0afb7ce84c926\","
                    "  \"v\": 28,"
                    "  \"msgHash\": \"0x2367ad2a1ff16e8af634f6e1062b0954f6898b5340d849b8b5b79bc936b57950\""
                    "}, {"
                    "  \"error\": {"
                    "    \"code\": -16001,"
                    "    \"message\": \"block is not final\","
                    "    \"data\": {"
                    "      \"signedError\": {"
                    "        \"r\": \"0x7edfd61f03d06cc65c08fb410435c7093ded2b3a0c4256e4d2993d930eac58d1\","
                    "        \"s\": \"0x6862b882c0bd3ed4d942c40f5efae692c4bdb27417651531f4cc1feab7fa2f9c\","
                    "        \"v\": 27,"
                    "        \"block\": 3662142,"
                    "        \"currentBlock\": 3662148,"
                    "        \"timestamp\": 1606156121,"
                    "        \"msgHash\": \"0x7ed5cdcb7790a78baa5f3aa5df1632f18c8ffb8ef926ebaa7a1394666f69e452\""
                    "      }"
                    "    }"
                    "  }"
                    "}, {"
                    "  \"blockHash\": \"0x3b1d2d185af8856ae03743b632ce1ed2c949e5d857870b7dae15f5b0601efff7\","
                    "  \"block\": 3662142,"
                    "  \"r\": \"0xe6ba447671e5f2c705d757111f0576e3109ec0b4b24540c74174dc101c6da2da\","
                    "  \"s\": \"0x61bdefd67dad9d6c2d07903b9a45072cd7a967b96139b9187307f05acbe0872a\","
                    "  \"v\": 27,"
                    "  \"msgHash\": \"0x2367ad2a1ff16e8af634f6e1062b0954f6898b5340d849b8b5b79bc936b57950\""
                    "}]");

  // now, we ask tincubeth for the signatures from nd-2, nd-3 and in3node.com.
  // in3node.com responds with a signed error saying that it doesn't consider the requested block final.
  // but, since we know that the diff b/w requested block and current block is less than in3node.com's
  // min block-height, we blacklist it
  ctx = in3_client_rpc_ctx_raw(in3, "{\"jsonrpc\":\"2.0\","
                                    "\"method\":\"eth_getTransactionByHash\","
                                    "\"params\":[\"0x715ece6967d0dc6aa6e8e4ee83937d3d4a79fdc644b64f07aa72f877df156be7\"],"
                                    "\"in3\":{\"dataNodes\":[\"0xf944d416ebdf7f6e22eaf79a5a53ad1a487ddd9a\"],"
                                    "\"signerNodes\":[\"0x1fe2e9bf29aa1938859af64c413361227d04059a\",\"0x945f75c0408c0026a3cd204d36f5e47745182fd4\",\"0xd4f40cb00b0620fbf3a546feb68a4496482d89a4\"]}}");

  nl = in3_nodeselect_def_data(in3);
  TEST_ASSERT_TRUE(is_blacklisted(&nl->nodelist[7]));
  ctx_free(ctx);

  in3_free(in3);
}

/*
 * Main
 */
int main() {
  //  in3_log_set_quiet(false);
  //  in3_log_set_level(LOG_TRACE);
  in3_register_default(in3_register_eth_basic);
  in3_register_default(in3_register_eth_api);
  in3_register_default(in3_register_nodeselect_def);

  TESTS_BEGIN();
  RUN_TEST(test_configure_request);
  RUN_TEST(test_configure_signed_request);
  RUN_TEST(test_bulk_response);
  RUN_TEST(test_partial_response);
  RUN_TEST(test_retry_response);
  RUN_TEST(test_exec_req);
  RUN_TEST(test_configure);
  RUN_TEST(test_configure_validation);
  RUN_TEST(test_parallel_signatures);
  RUN_TEST(test_sigs);
  return TESTS_END();
}
