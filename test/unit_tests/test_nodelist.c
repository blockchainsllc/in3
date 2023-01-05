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
#define DEBUG
#endif

#include "../../src/verifier/eth1/nano/rpcs.h"
#include "../../src/nodeselect/full/rpcs.h"
#include "../../src/api/eth1/eth_api.h"
#include "../../src/verifier/eth1/full/eth_full.h"
#include "../src/core/util/log.h"
#include "../test_utils.h"
#include "../util/transport.h"
#include "nodeselect/full/nodelist.h"
#include "nodeselect/full/nodeselect_def.h"

#define ADD_RESPONSE_NODELIST_3(last_block) add_response(FN_IN3_NODELIST,                                                                            \
                                                         "[0,\"0x0000000100000002000000030000000400000005000000060000000700000008\",[]]",           \
                                                         "{"                                                                                        \
                                                         " \"nodes\": [{"                                                                           \
                                                         "   \"url\": \"https://in3-v2.slock.it/mainnet/nd-1\","                                    \
                                                         "   \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\","                          \
                                                         "   \"index\": 0,"                                                                         \
                                                         "   \"deposit\": \"0x2386f26fc10000\","                                                    \
                                                         "   \"props\": \"0x6000001dd\","                                                           \
                                                         "   \"timeout\": 3456000,"                                                                 \
                                                         "   \"registerTime\": 1576224418,"                                                         \
                                                         "   \"weight\": 2000"                                                                      \
                                                         "  },"                                                                                     \
                                                         "  {"                                                                                      \
                                                         "   \"url\": \"https://in3-v2.slock.it/mainnet/nd-2\","                                    \
                                                         "   \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\","                          \
                                                         "   \"index\": 1,"                                                                         \
                                                         "   \"deposit\": \"0x2386f26fc10000\","                                                    \
                                                         "   \"props\": \"0x6000001dd\","                                                           \
                                                         "   \"timeout\": 3456000,"                                                                 \
                                                         "   \"registerTime\": 1576224531,"                                                         \
                                                         "   \"weight\": 2000"                                                                      \
                                                         "  },"                                                                                     \
                                                         "  {"                                                                                      \
                                                         "   \"url\": \"https://in3-v2.slock.it/mainnet/nd-3\","                                    \
                                                         "   \"address\": \"0x945f75c0408c0026a3cd204d36f5e47745182fd4\","                          \
                                                         "   \"index\": 2,"                                                                         \
                                                         "   \"deposit\": \"0x2386f26fc10000\","                                                    \
                                                         "   \"props\": \"0x6000001dd\","                                                           \
                                                         "   \"timeout\": 3456000,"                                                                 \
                                                         "   \"registerTime\": 1576224604,"                                                         \
                                                         "   \"weight\": 2000"                                                                      \
                                                         " }],"                                                                                     \
                                                         " \"contract\": \"0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f\","                           \
                                                         " \"registryId\": \"0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb\"," \
                                                         " \"lastBlockNumber\": " last_block ","                                                    \
                                                         " \"totalServers\": 3"                                                                     \
                                                         "}",                                                                                       \
                                                         NULL,                                                                                      \
                                                         NULL)
#define ADD_RESPONSE_NODELIST_2(last_block) add_response(FN_IN3_NODELIST,                                                                            \
                                                         "[0,\"0x0000000100000002000000030000000400000005000000060000000700000008\",[]]",           \
                                                         "{"                                                                                        \
                                                         " \"nodes\": [{"                                                                           \
                                                         "   \"url\": \"https://in3-v2.slock.it/mainnet/nd-1\","                                    \
                                                         "   \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\","                          \
                                                         "   \"index\": 0,"                                                                         \
                                                         "   \"deposit\": \"0x2386f26fc10000\","                                                    \
                                                         "   \"props\": \"0x6000001dd\","                                                           \
                                                         "   \"timeout\": 3456000,"                                                                 \
                                                         "   \"registerTime\": 1576224418,"                                                         \
                                                         "   \"weight\": 2000"                                                                      \
                                                         "  },"                                                                                     \
                                                         "  {"                                                                                      \
                                                         "   \"url\": \"https://in3-v2.slock.it/mainnet/nd-2\","                                    \
                                                         "   \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\","                          \
                                                         "   \"index\": 1,"                                                                         \
                                                         "   \"deposit\": \"0x2386f26fc10000\","                                                    \
                                                         "   \"props\": \"0x6000001dd\","                                                           \
                                                         "   \"timeout\": 3456000,"                                                                 \
                                                         "   \"registerTime\": 1576224531,"                                                         \
                                                         "   \"weight\": 2000"                                                                      \
                                                         "  }],"                                                                                    \
                                                         " \"contract\": \"0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f\","                           \
                                                         " \"registryId\": \"0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb\"," \
                                                         " \"lastBlockNumber\": " last_block ","                                                    \
                                                         " \"totalServers\": 2"                                                                     \
                                                         "}",                                                                                       \
                                                         NULL,                                                                                      \
                                                         NULL);
#define ADD_RESPONSE_BLOCK_NUMBER(nl, blk, blk_hex) add_response(FN_ETH_BLOCKNUMBER,              \
                                                                 "[]",                           \
                                                                 "\"" blk_hex "\"",              \
                                                                 NULL,                           \
                                                                 "{"                             \
                                                                 "  \"lastValidatorChange\": 0," \
                                                                 "  \"lastNodeList\": " nl ","   \
                                                                 "  \"execTime\": 59,"           \
                                                                 "  \"rpcTime\": 59,"            \
                                                                 "  \"rpcCount\": 1,"            \
                                                                 "  \"currentBlock\": " blk ","  \
                                                                 "  \"version\": \"2.0.0\""      \
                                                                 "}")

IN3_IMPORT_TEST bool in3_node_props_match(in3_node_props_t np_config, in3_node_props_t np);

static void test_capabilities(void) {
  in3_node_props_t npclient, npserver;

  in3_node_props_init(&npclient);
  in3_node_props_init(&npserver);
  TEST_ASSERT(in3_node_props_match(npclient, npserver));
  TEST_ASSERT_EQUAL(in3_node_props_get(npclient, NODE_PROP_MIN_BLOCK_HEIGHT), 0);
  TEST_ASSERT_FALSE(in3_node_props_get(npclient, NODE_PROP_PROOF));
  TEST_ASSERT_FALSE(in3_node_props_matches(npclient, NODE_PROP_HTTP));

  in3_node_props_init(&npclient);
  in3_node_props_set(&npclient, NODE_PROP_MIN_BLOCK_HEIGHT, 10);
  in3_node_props_init(&npserver);
  in3_node_props_set(&npserver, NODE_PROP_PROOF, true);
  in3_node_props_set(&npserver, NODE_PROP_MULTICHAIN, true);
  in3_node_props_set(&npserver, NODE_PROP_ARCHIVE, true);
  in3_node_props_set(&npserver, NODE_PROP_HTTP, true);
  in3_node_props_set(&npserver, NODE_PROP_BINARY, true);
  in3_node_props_set(&npserver, NODE_PROP_ONION, true);
  in3_node_props_set(&npserver, NODE_PROP_MIN_BLOCK_HEIGHT, 6);
  TEST_ASSERT_TRUE(in3_node_props_match(npclient, npserver));

  in3_node_props_init(&npclient);
  in3_node_props_set(&npclient, NODE_PROP_PROOF, true);
  in3_node_props_set(&npclient, NODE_PROP_ARCHIVE, true);
  in3_node_props_set(&npclient, NODE_PROP_BINARY, true);
  in3_node_props_set(&npclient, NODE_PROP_SIGNER, true);
  in3_node_props_init(&npserver);
  in3_node_props_set(&npserver, NODE_PROP_PROOF, true);
  in3_node_props_set(&npserver, NODE_PROP_MULTICHAIN, true);
  in3_node_props_set(&npserver, NODE_PROP_ARCHIVE, true);
  in3_node_props_set(&npserver, NODE_PROP_HTTP, true);
  in3_node_props_set(&npserver, NODE_PROP_BINARY, true);
  in3_node_props_set(&npserver, NODE_PROP_ONION, true);
  in3_node_props_set(&npserver, NODE_PROP_SIGNER, true);
  in3_node_props_set(&npserver, NODE_PROP_DATA, true);
  in3_node_props_set(&npserver, NODE_PROP_STATS, true);
  TEST_ASSERT(in3_node_props_match(npclient, npserver));

  in3_node_props_init(&npclient);
  in3_node_props_init(&npserver);
  in3_node_props_set(&npserver, NODE_PROP_PROOF, true);
  in3_node_props_set(&npserver, NODE_PROP_MULTICHAIN, true);
  in3_node_props_set(&npserver, NODE_PROP_ARCHIVE, true);
  in3_node_props_set(&npserver, NODE_PROP_HTTP, true);
  in3_node_props_set(&npserver, NODE_PROP_BINARY, true);
  in3_node_props_set(&npserver, NODE_PROP_ONION, true);
  TEST_ASSERT(in3_node_props_match(npclient, npserver));

  in3_node_props_init(&npclient);
  in3_node_props_set(&npclient, NODE_PROP_PROOF, true);
  in3_node_props_init(&npserver);
  in3_node_props_set(&npserver, NODE_PROP_HTTP, true);
  TEST_ASSERT_FALSE(in3_node_props_match(npclient, npserver));

  in3_node_props_init(&npclient);
  in3_node_props_set(&npclient, NODE_PROP_MIN_BLOCK_HEIGHT, 5);
  in3_node_props_init(&npserver);
  in3_node_props_set(&npserver, NODE_PROP_MIN_BLOCK_HEIGHT, 6);
  TEST_ASSERT_FALSE(in3_node_props_match(npclient, npserver));

  in3_node_props_init(&npclient);
  in3_node_props_set(&npclient, NODE_PROP_MIN_BLOCK_HEIGHT, 1);
  in3_node_props_init(&npserver);
  in3_node_props_set(&npserver, NODE_PROP_MIN_BLOCK_HEIGHT, 1);
  TEST_ASSERT_TRUE(in3_node_props_match(npclient, npserver));

  in3_node_props_init(&npclient);
  in3_node_props_set(&npclient, NODE_PROP_MIN_BLOCK_HEIGHT, 0);
  in3_node_props_init(&npserver);
  in3_node_props_set(&npserver, NODE_PROP_MIN_BLOCK_HEIGHT, 6);
  TEST_ASSERT_TRUE(in3_node_props_match(npclient, npserver));

  in3_node_props_init(&npclient);
  in3_node_props_set(&npclient, NODE_PROP_PROOF, true);
  in3_node_props_set(&npclient, NODE_PROP_HTTP, true);
  in3_node_props_set(&npclient, NODE_PROP_BINARY, true);
  in3_node_props_set(&npclient, NODE_PROP_MIN_BLOCK_HEIGHT, 5);
  in3_node_props_init(&npserver);
  npserver = 1U | (1U << 3U) | (1U << 4U) | (5UL << 32U);
  TEST_ASSERT_EQUAL_MEMORY(&npclient, &npserver, sizeof(in3_node_props_t));

  in3_node_props_init(&npclient);
  in3_node_props_set(&npclient, NODE_PROP_MIN_BLOCK_HEIGHT, 5);
  TEST_ASSERT_EQUAL(in3_node_props_get(npclient, NODE_PROP_MIN_BLOCK_HEIGHT), 5);
  in3_node_props_set(&npclient, NODE_PROP_MIN_BLOCK_HEIGHT, 0);
  TEST_ASSERT_EQUAL(in3_node_props_get(npclient, NODE_PROP_MIN_BLOCK_HEIGHT), 0);
  in3_node_props_set(&npclient, NODE_PROP_MIN_BLOCK_HEIGHT, 255);
  TEST_ASSERT_EQUAL(in3_node_props_get(npclient, NODE_PROP_MIN_BLOCK_HEIGHT), 255);
}

static in3_t* in3_init_test(chain_id_t chain) {
  in3_t* in3 = in3_for_chain(chain);
  in3_register_nodeselect_def(in3);
  in3->flags = FLAGS_AUTO_UPDATE_LIST | FLAGS_NODE_LIST_NO_SIG;
  register_transport(in3, test_transport);
  if (chain == CHAIN_ID_MAINNET) {
    // use a predefined nodelist
    /*
      initChain(chain, 0x01, "ac1b824795e1eb1f6e609fe0da9b9af8beaab60f", "23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb", 2, 5, CHAIN_ETH, NULL);
  initNode(chain, 0, "45d45e6ff99e6c34a235d263965910298985fcfe", "https://in3-v2.slock.it/mainnet/nd-1");
  initNode(chain, 1, "1fe2e9bf29aa1938859af64c413361227d04059a", "https://in3-v2.slock.it/mainnet/nd-2");
  initNode(chain, 2, "0cea2ff03adcfa047e8f54f98d41d9147c3ccd4d", "https://in3-g.open-dna.de");
  initNode(chain, 3, "ccd12a2222995e62eca64426989c2688d828aa47", "https://chaind.de/eth/mainnet1");
  initNode(chain, 4, "510ee7f6f198e018e3529164da2473a96eeb3dc8", "https://0001.mainnet.in3.anyblock.tools");

*/
    char* err = in3_configure(in3, "{\"nodeRegistry\": {"
                                   "    \"nodeList\": [{"
                                   "                        \"address\": \"0x45d45e6ff99e6c34a235d263965910298985fcfe\","
                                   "                       \"url\": \"https://in3-v2.slock.it/mainnet/nd-1\","
                                   "                       \"props\": \"0xFFFF\""
                                   "  },{"
                                   "                        \"address\": \"0x1fe2e9bf29aa1938859af64c413361227d04059a\","
                                   "                       \"url\": \"https://in3-v2.slock.it/mainnet/nd-2\","
                                   "                       \"props\": \"0xFFFF\""
                                   "  },{"
                                   "                        \"address\": \"0x0cea2ff03adcfa047e8f54f98d41d9147c3ccd4d\","
                                   "                       \"url\": \"https://in3-v2.slock.it/mainnet/nd-3\","
                                   "                       \"props\": \"0xFFFF\""
                                   "  },{"
                                   "                        \"address\": \"0xccd12a2222995e62eca64426989c2688d828aa47\","
                                   "                       \"url\": \"https://in3-v2.slock.it/mainnet/nd-4\","
                                   "                       \"props\": \"0xFFFF\""
                                   "  },{"
                                   "                        \"address\": \"0x510ee7f6f198e018e3529164da2473a96eeb3dc8\","
                                   "                       \"url\": \"https://in3-v2.slock.it/mainnet/nd-5\","
                                   "                       \"props\": \"0xFFFF\""
                                   "  }]}}");
    if (err) printf("ERR: %s\n", err);
  }
  return in3;
}

// Scenario 1: nodelist updated in a very recent block
// Begin with 3 nodes in first nodeList update (always taken from a boot node) which happened at block 87989012 (i.e. `lastBlockNumber`).
// After 200 secs, `eth_blockNumber` returns 87989050 and reports a nodeList update happened at 87989048 (i.e. `lastNodeList`).
// Since, we have set `replace_latest_block` to default (i.e. 6), we postpone the update for `avg_block_time * 6 - (87989050 - 87989048)) = 60` seconds.
// A request now shouldn't trigger a nodeList update. We then go to the future (i.e. to expected update time) and run `eth_blockNumber`
// again; this should trigger a nodeList update.
static void test_nodelist_update_1() {
  in3_t* c                = in3_init_test(CHAIN_ID_MAINNET);
  c->proof                = PROOF_NONE;
  c->replace_latest_block = DEF_REPL_LATEST_BLK;

  // start time
  uint64_t t = 1;
  in3_time(&t);

  // begin with 3 nodes, i.e. one node more than the usual boot nodes
  ADD_RESPONSE_BLOCK_NUMBER("87989048", "87989050", "0x53E9B3A");
  ADD_RESPONSE_NODELIST_3("87989012");

  // fast forward to future, after a recent nodeList update (i.e. 2 blocks back)
  t = 200;
  in3_time(&t);

  uint64_t blk = eth_blockNumber(c);
  TEST_ASSERT_NOT_EQUAL(0, blk);

  in3_nodeselect_def_t* nl = in3_nodeselect_def_data(c);
  TEST_ASSERT_EQUAL(nl->nodelist_length, 3);
  TEST_ASSERT_NOT_NULL(nl->nodelist_upd8_params);
  TEST_ASSERT_EQUAL(nl->nodelist_upd8_params->exp_last_block, 87989048);

  // update must be postponed until block is at least replace_latest_block old
  t = 0;
  t = in3_time(&t) + (nl->avg_block_time * (c->replace_latest_block - 2)); // store expected update time
  TEST_ASSERT_EQUAL(nl->nodelist_upd8_params->timestamp, t);
  ADD_RESPONSE_BLOCK_NUMBER("87989048", "87989053", "0x53E9B3D");

  // another request must not trigger the nodeList update just yet
  blk = eth_blockNumber(c);
  TEST_ASSERT_NOT_EQUAL(0, blk);
  TEST_ASSERT_NOT_NULL(nl->nodelist_upd8_params);

  // fast forward to expected update time
  in3_time(&t);

  // reset rand to be deterministic
  int s = 0;
  in3_rand(&s);

  // any request should now trigger a nodeList update
  // in new update node 3 was removed
  ADD_RESPONSE_BLOCK_NUMBER("87989048", "87989050", "0x53E9B3A");
  ADD_RESPONSE_NODELIST_2("87989048");
  blk = eth_blockNumber(c);
  TEST_ASSERT_NOT_EQUAL(0, blk);
  TEST_ASSERT_EQUAL(nl->nodelist_length, 2);
  TEST_ASSERT_NULL(nl->nodelist_upd8_params);

  in3_free(c);
}

// Scenario 2: nodelist updated again while prev nodelist update was postponed
// Begin with 3 nodes in first nodeList update (always taken from a boot node) which happened at block 87989012 (i.e. `lastBlockNumber`).
// After 200 secs, `eth_blockNumber` returns 87989050 and reports a nodeList update happened at 87989049 (i.e. `lastNodeList`).
// Since, we have set `replace_latest_block` to default (i.e. 10), we postpone the update for `avg_block_time * 10 - (87989050 - 87989049)) = 135` seconds.
// `eth_blockNumber` now does not trigger a nodeList update. We move 60 secs ahead in time and do another `eth_blockNumber` which returns 87989054, this again
// does not trigger a nodeList update but reports a newer nodeList update happened at 87989052 (i.e. `lastNodeList`). So, we again postpone
// update. We again move ahead in time just before expected update time and call `eth_blockNumber` to check that nodeList isn't updated.
// We then go to the future (i.e. to expected update time) and run `eth_blockNumber` again; this should trigger a nodeList update.
static void test_nodelist_update_2() {
  in3_t* c                = in3_init_test(CHAIN_ID_MAINNET);
  c->proof                = PROOF_NONE;
  c->replace_latest_block = 10;

  // start time
  uint64_t t = 1;
  in3_time(&t);

  // reset rand to be deterministic
  int s = 0;
  in3_rand(&s);

  // begin with 3 nodes, i.e. one node more than the usual boot nodes
  ADD_RESPONSE_BLOCK_NUMBER("87989049", "87989050", "0x53E9B3A");
  ADD_RESPONSE_NODELIST_3("87989012");

  // fast forward to future, after a recent nodeList update (i.e. 2 blocks back)
  t = 200;
  in3_time(&t);

  uint64_t blk = eth_blockNumber(c);
  TEST_ASSERT_NOT_EQUAL(0, blk);

  in3_nodeselect_def_t* nl = in3_nodeselect_def_data(c);
  TEST_ASSERT_EQUAL(nl->nodelist_length, 3);
  TEST_ASSERT_NOT_NULL(nl->nodelist_upd8_params);
  TEST_ASSERT_EQUAL(nl->nodelist_upd8_params->exp_last_block, 87989049);

  // update must be postponed until block is at least replace_latest_block old
  t = 0;
  t = in3_time(&t) + (nl->avg_block_time * (c->replace_latest_block - 1)); // store expected update time
  TEST_ASSERT_EQUAL(nl->nodelist_upd8_params->timestamp, t);

  ADD_RESPONSE_BLOCK_NUMBER("87989051", "87989053", "0x53E9B3D");
  // another request must not trigger the nodeList update just yet
  blk = eth_blockNumber(c);
  TEST_ASSERT_NOT_EQUAL(0, blk);
  TEST_ASSERT_NOT_NULL(nl->nodelist_upd8_params);

  // we are told that the nodelist changed again at 87989051, so reverify wait time and update params
  TEST_ASSERT_NOT_NULL(nl->nodelist_upd8_params);
  TEST_ASSERT_EQUAL(nl->nodelist_upd8_params->exp_last_block, 87989051);
  t = 0;
  t = in3_time(&t) + (nl->avg_block_time * (c->replace_latest_block - 2)); // store expected update time
  TEST_ASSERT_EQUAL(nl->nodelist_upd8_params->timestamp, t);

  // fast forward to expected update time
  in3_time(&t);

  // reset rand to be deterministic
  s = 0;
  in3_rand(&s);

  // any request should now trigger a nodeList update
  // in new update node 3 was removed
  ADD_RESPONSE_BLOCK_NUMBER("87989048", "87989050", "0x53E9B3A");
  ADD_RESPONSE_NODELIST_2("87989048");
  blk = eth_blockNumber(c);
  TEST_ASSERT_NOT_EQUAL(0, blk);
  TEST_ASSERT_EQUAL(nl->nodelist_length, 2);
  TEST_ASSERT_NULL(nl->nodelist_upd8_params);

  in3_free(c);
}

// Scenario 3: Usual case
// Begin with 3 nodes in first nodeList update (always taken from a boot node) which happened at block 87989012 (i.e. `lastBlockNumber`).
// After 200 secs, `eth_blockNumber` returns 87989050 and reports a nodeList update happened at 87989038 (i.e. `lastNodeList`).
// A request now should trigger a nodeList update.
static void test_nodelist_update_3() {
  in3_t* c                = in3_init_test(CHAIN_ID_MAINNET);
  c->proof                = PROOF_NONE;
  c->replace_latest_block = DEF_REPL_LATEST_BLK;

  // start time
  uint64_t t = 1;
  in3_time(&t);

  // reset rand to be deterministic
  int s = 0;
  in3_rand(&s);

  // begin with 3 nodes, i.e. one node more than the usual boot nodes
  ADD_RESPONSE_BLOCK_NUMBER("87989038", "87989050", "0x53E9B3A");
  ADD_RESPONSE_NODELIST_3("87989012");

  t = 200;
  in3_time(&t);

  uint64_t blk = eth_blockNumber(c);
  TEST_ASSERT_NOT_EQUAL(0, blk);

  in3_nodeselect_def_t* nl = in3_nodeselect_def_data(c);
  TEST_ASSERT_EQUAL(nl->nodelist_length, 3);
  TEST_ASSERT_NOT_NULL(nl->nodelist_upd8_params);
  TEST_ASSERT_EQUAL(nl->nodelist_upd8_params->exp_last_block, 87989038);

  // reset rand to be deterministic
  s = 0;
  in3_rand(&s);

  // any request should now trigger a nodeList update
  ADD_RESPONSE_BLOCK_NUMBER("87989038", "87989050", "0x53E9B3A");
  ADD_RESPONSE_NODELIST_2("87989038");
  blk = eth_blockNumber(c);
  TEST_ASSERT_NOT_EQUAL(0, blk);
  TEST_ASSERT_EQUAL(nl->nodelist_length, 2);
  TEST_ASSERT_NULL(nl->nodelist_upd8_params);

  in3_free(c);
}

// Scenario 4: node lies about nodeList update by reporting a newer lastNodeList, but gives old nodeList when asked
// Begin with 3 nodes in first nodeList update (always taken from a boot node) which happened at block 87989012 (i.e. `lastBlockNumber`).
// After 200 secs, `eth_blockNumber` returns 87989050 and lies to us saying that a nodeList update happened at 87989038 (i.e. `lastNodeList`).
// A request now should trigger a nodeList update (taken from same node that reported the update). Because the node lied to us, it cannot
// furnish a newer nodeList and still gives us an old list. This must be detected and the node should be blacklisted.
static void test_nodelist_update_4() {
  in3_t* c                = in3_init_test(CHAIN_ID_MAINNET);
  c->proof                = PROOF_NONE;
  c->replace_latest_block = DEF_REPL_LATEST_BLK;

  // start time
  uint64_t t = 1;
  in3_time(&t);

  // reset rand to be deterministic
  int s = 0;
  in3_rand(&s);

  // begin with 3 nodes, i.e. one node more than the usual boot nodes
  ADD_RESPONSE_BLOCK_NUMBER("87989038", "87989050", "0x53E9B3A");
  ADD_RESPONSE_NODELIST_3("87989012");

  t = 200;
  in3_time(&t);

  uint64_t blk = eth_blockNumber(c);
  TEST_ASSERT_NOT_EQUAL(0, blk);

  in3_nodeselect_def_t* nl = in3_nodeselect_def_data(c);
  TEST_ASSERT_EQUAL(nl->nodelist_length, 3);
  TEST_ASSERT_NOT_NULL(nl->nodelist_upd8_params);
  TEST_ASSERT_EQUAL(nl->nodelist_upd8_params->exp_last_block, 87989038);

  // reset rand to be deterministic
  s = 0;
  in3_rand(&s);

  // any request should now trigger a nodeList update
  // but since the node lied and cannot give us a new list it gives us the old list
  ADD_RESPONSE_BLOCK_NUMBER("87989012", "87989050", "0x53E9B3A");
  ADD_RESPONSE_NODELIST_3("87989012");
  blk = eth_blockNumber(c);
  TEST_ASSERT_NOT_EQUAL(0, blk);
  TEST_ASSERT_EQUAL(nl->nodelist_length, 3);
  TEST_ASSERT_NULL(nl->nodelist_upd8_params);

  bool is_blacklisted = false;
  for (int i = 0; i < nl->nodelist_length; ++i)
    if (nl->weights[i].blacklisted_until)
      is_blacklisted = true;
  TEST_ASSERT_TRUE(is_blacklisted);

  in3_free(c);
}

// Scenario 5: Boot nodes do not respond for first update
static void test_nodelist_update_5() {
  in3_t* c                = in3_init_test(CHAIN_ID_MAINNET);
  c->replace_latest_block = DEF_REPL_LATEST_BLK;
  c->proof                = PROOF_NONE;
  c->max_attempts         = 1;

  // start time
  uint64_t t = 1;
  in3_time(&t);

  // reset rand to be deterministic
  int s = 0;
  in3_rand(&s);

  ADD_RESPONSE_BLOCK_NUMBER("87989038", "87989050", "0x53E9B3A");
  add_response(FN_IN3_NODELIST,
               "[0,\"0x0000000100000002000000030000000400000005000000060000000700000008\",[]]",
               NULL,
               "\"Internal server error!\"",
               NULL);

  uint64_t blk = eth_blockNumber(c);
  TEST_ASSERT_NOT_EQUAL(0, blk);

  in3_nodeselect_def_t* nl = in3_nodeselect_def_data(c);
  TEST_ASSERT_NOT_NULL(nl->nodelist_upd8_params);
  TEST_ASSERT_EQUAL(nl->nodelist_upd8_params->exp_last_block, 87989038);

  // reset rand to be deterministic
  s = 0;
  in3_rand(&s);

  ADD_RESPONSE_BLOCK_NUMBER("87989038", "87989050", "0x53E9B3A");
  ADD_RESPONSE_NODELIST_3("87989038");
  blk = eth_blockNumber(c);
  TEST_ASSERT_NOT_EQUAL(0, blk);
  TEST_ASSERT_EQUAL(nl->nodelist_length, 3);
  TEST_ASSERT_NULL(nl->nodelist_upd8_params);

  in3_free(c);
}

// Scenario 6: Wrong first update response
static void test_nodelist_update_6() {
  in3_t* c                = in3_init_test(CHAIN_ID_MAINNET);
  c->replace_latest_block = DEF_REPL_LATEST_BLK;
  c->max_attempts         = 1;

  // start time
  uint64_t t = 1;
  in3_time(&t);

  // reset rand to be deterministic
  int s = 0;
  in3_rand(&s);

  ADD_RESPONSE_BLOCK_NUMBER("87989038", "87989050", "0x53E9B3A");
  add_response(FN_IN3_NODELIST,
               "[0,\"0x0000000100000002000000030000000400000005000000060000000700000008\",[]]",
               "{}",
               NULL,
               NULL);

  uint64_t blk = eth_blockNumber(c);
  TEST_ASSERT_NOT_EQUAL(0, blk);

  in3_nodeselect_def_t* nl = in3_nodeselect_def_data(c);
  TEST_ASSERT_NOT_NULL(nl->nodelist_upd8_params);
  TEST_ASSERT_EQUAL(nl->nodelist_upd8_params->exp_last_block, 87989038);

  // reset rand to be deterministic
  s = 0;
  in3_rand(&s);

  c->proof = PROOF_NONE;

  ADD_RESPONSE_BLOCK_NUMBER("87989038", "87989050", "0x53E9B3A");
  ADD_RESPONSE_NODELIST_3("87989038");
  blk = eth_blockNumber(c);
  TEST_ASSERT_NOT_EQUAL(0, blk);
  TEST_ASSERT_EQUAL(nl->nodelist_length, 3);
  TEST_ASSERT_NULL(nl->nodelist_upd8_params);

  in3_free(c);
}

// Scenario 7: lastNodeList > currentBlock
// Begin with 3 nodes in first nodeList update (always taken from a boot node) which happened at block 87989012 (i.e. `lastBlockNumber`).
// eth_blockNumber returns a lastNodeList > currentBlock. This is impossible, so we ignore it and do NOT update the nodeList.
static void test_nodelist_update_7() {
  in3_t* c                = in3_init_test(CHAIN_ID_MAINNET);
  c->proof                = PROOF_NONE;
  c->replace_latest_block = DEF_REPL_LATEST_BLK;

  // start time
  uint64_t t = 1;
  in3_time(&t);

  // reset rand to be deterministic
  int s = 0;
  in3_rand(&s);

  // begin with 3 nodes, i.e. one node more than the usual boot nodes
  ADD_RESPONSE_BLOCK_NUMBER("87989999", "87989050", "0x53E9B3A");
  ADD_RESPONSE_NODELIST_3("87989012");

  uint64_t blk = eth_blockNumber(c);
  TEST_ASSERT_NOT_EQUAL(0, blk);

  in3_nodeselect_def_t* nl = in3_nodeselect_def_data(c);
  TEST_ASSERT_EQUAL(nl->nodelist_length, 3);
  TEST_ASSERT_NULL(nl->nodelist_upd8_params);

  in3_free(c);
}

// Scenario 8: Newer `lastNodeList` with wrong response
// Begin with 3 nodes in first nodeList update (always taken from a boot node) which happened at block 87989012 (i.e. `lastBlockNumber`).
// After 200 secs, `eth_blockNumber` returns an error result but reports a nodeList update happened at 87989038 (i.e. `lastNodeList`).
// Since the result was not valid, we must not accept the `lastNodeList` and should not trigger a nodeList update.
// A request now should trigger a nodeList update.
static void test_nodelist_update_8() {
  in3_t* c                = in3_init_test(CHAIN_ID_MAINNET);
  c->proof                = PROOF_NONE;
  c->replace_latest_block = DEF_REPL_LATEST_BLK;
  c->max_attempts         = 1;

  // start time
  uint64_t t = 1;
  in3_time(&t);

  // reset rand to be deterministic
  int s = 0;
  in3_rand(&s);

  // begin with 3 nodes, i.e. one node more than the usual boot nodes
  ADD_RESPONSE_BLOCK_NUMBER("87989038", "87989050", "0x53E9B3A");
  ADD_RESPONSE_NODELIST_3("87989038");

  t = 200;
  in3_time(&t);
  TEST_ASSERT_NOT_EQUAL(0, eth_blockNumber(c));

  // reset rand to be deterministic
  s = 0;
  in3_rand(&s);

  // test that nodelist_upd8_params are not set when we have an error response
  add_response(FN_ETH_BLOCKNUMBER,
               "[]",
               NULL,
               "\"Error: Internal server error\"",
               "{"
               "  \"lastValidatorChange\": 0,"
               "  \"lastNodeList\": 87989049,"
               "  \"execTime\": 59,"
               "  \"rpcTime\": 59,"
               "  \"rpcCount\": 1,"
               "  \"currentBlock\": 87989092,"
               "  \"version\": \"2.0.0\""
               "}");
  TEST_ASSERT_EQUAL(0, eth_blockNumber(c));
  in3_nodeselect_def_t* nl = in3_nodeselect_def_data(c);
  TEST_ASSERT_NULL(nl->nodelist_upd8_params);

  // test that nodelist_upd8_params are not set when we have a response without proof
  c->proof = PROOF_STANDARD;
  add_response(FN_ETH_GETBALANCE,
               "[\"0x000000000000000000000000000000000000dead\",\"latest\"]",
               "\"0x2a595770eb8ed0c5827\"",
               NULL,
               "{"
               "  \"lastValidatorChange\": 0,"
               "  \"lastNodeList\": 87989049,"
               "  \"execTime\": 59,"
               "  \"rpcTime\": 59,"
               "  \"rpcCount\": 1,"
               "  \"currentBlock\": 87989092,"
               "  \"version\": \"2.0.0\""
               "}");

  address_t addr;
  hex_to_bytes("0x000000000000000000000000000000000000dead", -1, addr, 20);
  long double balance = as_double(eth_getBalance(c, addr, BLKNUM_LATEST()));
  TEST_ASSERT_EQUAL(0, balance);

  TEST_ASSERT_NULL(in3_nodeselect_def_data(c)->nodelist_upd8_params);

  in3_free(c);
}

static void test_nodelist_pick_signer_exclusions() {
  in3_t* in3 = in3_for_chain(0x34ff);
  char*  err = in3_configure(in3, "{\"chainId\":\"0x34ff\",\"chainType\":0,\"autoUpdateList\":false,\"signatureCount\":1,\"requestCount\":1,\"maxAttempts\":1,\"maxVerifiedHashes\":0,"
                                   "\"nodeRegistry\":{"
                                   "   \"needsUpdate\":false,"
                                   "   \"contract\": \"0x5f51e413581dd76759e9eed51e63d14c8d1379c8\","
                                   "   \"registryId\": \"0x67c02e5e272f9d6b4a33716614061dd298283f86351079ef903bf0d4410a44ea\","
                                   "   \"nodeList\": [{"
                                   "      \"url\":\"https://in3-v2.slock.it/priv/nd-1\","
                                   "      \"address\":\"0x45d45e6ff99e6c34a235d263965910298985fcfe\","
                                   "      \"props\":\"0x1dd\""
                                   "    },"
                                   "    {"
                                   "      \"url\":\"https://in3-v2.slock.it/priv/nd-2\","
                                   "      \"address\":\"0x1fe2e9bf29aa1938859af64c413361227d04059a\","
                                   "      \"props\":\"0x1dd\""
                                   "    },"
                                   "    {"
                                   "      \"url\":\"https://in3-v2.slock.it/goerli/nd-3\","
                                   "      \"address\":\"0x945f75c0408c0026a3cd204d36f5e47745182fd4\","
                                   "      \"props\":\"0x1dd\""
                                   "    }]"
                                   "}}");
  TEST_ASSERT_NULL_MESSAGE(err, err);
  register_transport(in3, test_transport);

  size_t test_count = 100; /* we should surely hit a collision by then (unless the code is handling exclusions correctly) */
  for (size_t i = 0; i < test_count; i++) {
    in3_req_t*        ctx  = req_new(in3, "{\"jsonrpc\":\"2.0\","
                                                  "\"method\":\"eth_getTransactionByHash\","
                                                  "\"params\":[\"0x715ece6967d0dc6aa6e8e4ee83937d3d4a79fdc644b64f07aa72f877df156be7\"]}");
    in3_nl_pick_ctx_t pctx = {.type = NL_DATA, .req = ctx};
    TEST_ASSERT_EQUAL(IN3_OK, in3_plugin_execute_first(ctx, PLGN_ACT_NL_PICK, &pctx));
    pctx.type = NL_SIGNER;
    TEST_ASSERT_EQUAL(IN3_OK, in3_plugin_execute_first(ctx, PLGN_ACT_NL_PICK, &pctx));
    TEST_ASSERT_NOT_NULL(ctx->in3_state->nodes);
    TEST_ASSERT_EQUAL(1, ctx->in3_state->signers_length);
    TEST_ASSERT(memcmp(ctx->in3_state->nodes->address, ctx->in3_state->signers, 20) != 0);
    req_free(ctx);
  }

  in3_free(in3);
}

/*
 * Main
 */
int main() {
  TESTS_BEGIN();
  in3_register_default(in3_register_eth_full);
  in3_register_default(in3_register_nodeselect_def);
  in3_set_func_time(mock_time);
  in3_set_func_rand(mock_rand);
  RUN_TEST(test_nodelist_pick_signer_exclusions);
  RUN_TEST(test_capabilities);
  RUN_TEST(test_nodelist_update_1);
  RUN_TEST(test_nodelist_update_2);
  RUN_TEST(test_nodelist_update_3);
  RUN_TEST(test_nodelist_update_4);
  RUN_TEST(test_nodelist_update_5);
  RUN_TEST(test_nodelist_update_6);
  RUN_TEST(test_nodelist_update_7);
  RUN_TEST(test_nodelist_update_8);
  return TESTS_END();
}
