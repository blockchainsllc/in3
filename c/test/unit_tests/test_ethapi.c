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
#include "../../src/core/client/context.h"
#include "../../src/core/client/keys.h"
#include "../../src/core/client/nodelist.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/log.h"
#include "../../src/core/util/scache.h"
#include "../../src/verifier/eth1/basic/signer.h"
#include "../../src/verifier/eth1/full/eth_full.h"
#include "../../src/verifier/eth1/nano/eth_nano.h"

#include "../test_utils.h"
#include "../util/transport.h"
#include <stdio.h>
#include <unistd.h>

in3_t* init_in3(in3_transport_send custom_transport, chain_id_t chain) {
  in3_t* in3 = NULL;
  int    err;
  in3 = in3_for_chain(0);
  if (custom_transport)
    in3->transport = custom_transport; // use curl to handle the requests
  in3->request_count = 1;              // number of requests to sendp
  in3->max_attempts  = 1;
  in3->request_count = 1; // number of requests to sendp
  in3->chain_id      = chain;
  in3->flags         = FLAGS_STATS | FLAGS_INCLUDE_CODE; // no autoupdate nodelist
  for (int i = 0; i < in3->chains_length; i++) in3->chains[i].nodelist_upd8_params = NULL;
  return in3;
}

static void test_get_balance() {
  in3_t* in3 = init_in3(mock_transport, 0x5);
  // the address of account whose balance we want to get
  address_t account;
  hex_to_bytes("0xF99dbd3CFc292b11F74DeEa9fa730825Ee0b56f2", -1, account, 20);
  // get balance of account
  long double balance = as_double(eth_getBalance(in3, account, BLKNUM(1555415)));
  TEST_ASSERT_TRUE(balance > 0.0);
  in3_free(in3);
}

static void test_get_tx_count() {
  in3_t* in3 = init_in3(mock_transport, 0x5);

  // the address of account whose balance we want to get
  address_t account;
  hex_to_bytes("0x0de496ae79194d5f5b18eb66987b504a0feb32f2", -1, account, 20);
  // get balance of account
  uint64_t count = eth_getTransactionCount(in3, account, BLKNUM_LATEST());
  in3_log_debug("tx count %llu\n", count);

  TEST_ASSERT_TRUE(count > 0.0);
  in3_free(in3);
}

static void test_new_block_filter() {
  in3_t* in3 = init_in3(mock_transport, 0x5);
  // we can add any mock json as we need trasnport but we are not calling any rpc endpoint
  //get filter id for new block
  size_t fid = eth_newBlockFilter(in3);
  TEST_ASSERT_TRUE(fid > 0);
  in3_free(in3);
}

static void test_block_number() {
  in3_t*   in3    = init_in3(mock_transport, 0x5);
  uint64_t blknum = eth_blockNumber(in3);
  TEST_ASSERT_TRUE(blknum > 0);
  in3_free(in3);
}

static void test_new_pending_tx_filter() {
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  in3_ret_t ret = eth_newPendingTransactionFilter(in3);
  //we expect this to fail we dont support pending
  TEST_ASSERT_TRUE(ret == IN3_ENOTSUP);
  in3_free(in3);
}

static void test_get_filter_changes() {
  in3_t* in3 = init_in3(test_transport, ETH_CHAIN_ID_MAINNET);
  in3->proof = PROOF_NONE;

  // Test with no filters registered
  TEST_ASSERT_EQUAL(IN3_EFIND, eth_getFilterChanges(in3, 1, NULL, NULL));

  // Create new filter with options
  add_response("eth_blockNumber", "[]", "\"0x84cf52\"", NULL, NULL);
  json_ctx_t* jopt = parse_json("{\"fromBlock\":\"0x84cf51\",\"address\":\"0xF0AD5cAd05e10572EfcEB849f6Ff0c68f9700455\",\"topics\":[\"0xca6abbe9d7f11422cb6ca7629fbf6fe9efb1c621f71ce8f02b9f2a230097404f\"]}");
  size_t      fid  = eth_newFilter(in3, jopt);
  TEST_ASSERT_GREATER_THAN(0, fid);
  add_response("eth_getLogs", "[{\"fromBlock\":\"0x84cf51\",\"address\":\"0xF0AD5cAd05e10572EfcEB849f6Ff0c68f9700455\",\"topics\":[\"0xca6abbe9d7f11422cb6ca7629fbf6fe9efb1c621f71ce8f02b9f2a230097404f\"]}]", "[]", NULL, NULL);
  add_response("eth_blockNumber", "[]", "\"0x84cf55\"", NULL, NULL);
  // Get changes
  eth_log_t* logs = NULL;
  in3_ret_t  ret  = eth_getFilterChanges(in3, fid, NULL, &logs);
  TEST_ASSERT_EQUAL(IN3_OK, ret);
  json_free(jopt);

  // Create block filter
  add_response("eth_blockNumber", "[]", "\"0x84cf58\"", NULL, NULL);
  size_t bfid = eth_newBlockFilter(in3);
  TEST_ASSERT_GREATER_THAN(0, bfid);
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

  // Get changes
  bytes32_t* hashes = NULL;
  ret               = eth_getFilterChanges(in3, bfid, &hashes, NULL);
  TEST_ASSERT_EQUAL(1, ret);
  bytes32_t blk_hash;
  hex_to_bytes("0xf407f59e59f35659ebf92b7c51d7faab027b3217144dd5bce9fc5b42de1e1de9", -1, blk_hash, 32);
  TEST_ASSERT_EQUAL_MEMORY(hashes, blk_hash, 32);

  add_response("eth_blockNumber", "[]", "\"0x84cf59\"", NULL, NULL);
  ret = eth_getFilterChanges(in3, bfid, &hashes, NULL);
  TEST_ASSERT_EQUAL(0, ret);

  // Test with non-existent filter id
  TEST_ASSERT_EQUAL(IN3_EINVAL, eth_getFilterChanges(in3, 1234, NULL, NULL));

  // Test with all filters uninstalled
  TEST_ASSERT_TRUE(eth_uninstallFilter(in3, fid));
  TEST_ASSERT_TRUE(eth_uninstallFilter(in3, bfid));
  TEST_ASSERT_EQUAL(IN3_EFIND, eth_getFilterChanges(in3, fid, NULL, NULL));

  in3_free(in3);
}

static void test_get_logs() {
  in3_t* in3 = init_in3(mock_transport, 0x5);

  // Test with no filters registered
  TEST_ASSERT_EQUAL(IN3_EFIND, eth_getFilterLogs(in3, 1, NULL));

  // Create filter options
  char b[30];
  sprintf(b, "{\"fromBlock\":\"0x1ca181\"}");
  json_ctx_t* jopt = parse_json(b);

  // Create new filter with options
  size_t fid = eth_newFilter(in3, jopt);

  // Get logs
  eth_log_t *logs = NULL, *l = NULL;
  TEST_ASSERT_EQUAL(IN3_OK, eth_getFilterLogs(in3, fid, &logs));

  while (logs) {
    l    = logs;
    logs = logs->next;
    eth_log_free(l);
  }
  eth_uninstallFilter(in3, fid);
  json_free(jopt);

  // Test with non-existent filter id
  TEST_ASSERT_EQUAL(IN3_EINVAL, eth_getFilterLogs(in3, 1234, NULL));

  // Test with all filters uninstalled
  TEST_ASSERT_EQUAL(IN3_EFIND, eth_getFilterLogs(in3, fid, NULL));

  in3_free(in3);
}

static void test_get_tx_blkhash_index(void) {
  // the hash of transaction that we want to get
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  bytes32_t blk_hash;
  hex_to_bytes("0xbaf52e8d5e9c7ece67b1c3a0788379a4f486d8ec50bbf531b3a6720ca03fe1c4", -1, blk_hash, 32);

  // get the tx by hash
  eth_tx_t* tx = eth_getTransactionByBlockHashAndIndex(in3, blk_hash, 0);
  TEST_ASSERT_TRUE(tx != NULL);
  free(tx);
  in3_free(in3);
}

static void test_get_tx_blknum_index(void) {
  // the hash of transaction that we want to get
  in3_t* in3 = init_in3(mock_transport, 0x5);
  // get the tx by hash
  eth_tx_t* tx = eth_getTransactionByBlockNumberAndIndex(in3, BLKNUM(1723267), 0);
  TEST_ASSERT_TRUE(tx != NULL);
  free(tx);
  in3_free(in3);
}

static void test_get_tx_hash(void) {
  // the hash of transaction that we want to get
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  bytes32_t tx_hash;
  hex_to_bytes("0x9241334b0b568ef6cd44d80e37a0ce14de05557a3cfa98b5fd1d006204caf164", -1, tx_hash, 32);
  // get the tx by hash
  eth_tx_t* tx = eth_getTransactionByHash(in3, tx_hash);
  TEST_ASSERT_NOT_NULL(tx);
  TEST_ASSERT_EQUAL(36, tx->data.len);
  TEST_ASSERT_EQUAL(1, tx->data.data[35]); // based on the mockdata the
  TEST_ASSERT_EQUAL(0x17a7a4, tx->block_number);
  TEST_ASSERT_EQUAL(0x31, tx->nonce);
  TEST_ASSERT_EQUAL(0xa3d7, tx->gas);
  TEST_ASSERT_EQUAL(0, tx->transaction_index);

  free(tx);

  // get non-existent txn
  in3->transport = test_transport;
  add_response("eth_getTransactionByHash", "[\"0x9241334b0b568ef6cd44d80e37a0ce14de05557a3cfa98b5fd1d006204caf164\"]", "null", NULL, NULL);
  tx = eth_getTransactionByHash(in3, tx_hash);
  TEST_ASSERT_NULL(tx);
  free(tx);

  in3_free(in3);
}

static void test_get_tx_receipt(void) {
  // the hash of transaction whose receipt we want to get

  in3_t*    in3 = init_in3(mock_transport, 0x5);
  bytes32_t tx_hash;
  hex_to_bytes("0x8e7fb87e95c69a780490fce3ea14b44c78366fc45baa6cb86a582166c10c6d9d", -1, tx_hash, 32);

  // get the tx receipt by hash
  eth_tx_receipt_t* txr = eth_getTransactionReceipt(in3, tx_hash);
  TEST_ASSERT_TRUE(txr);
  TEST_ASSERT_TRUE(txr->status);
  TEST_ASSERT_TRUE(txr->gas_used > 0);
  eth_tx_receipt_free(txr);
  in3_free(in3);
}

static void test_send_tx(void) {
  in3_t* in3 = init_in3(mock_transport, 0x5);
  // prepare parameters
  address_t to, from;
  hex_to_bytes("0x0dE496AE79194D5F5b18eB66987B504A0FEB32f2", -1, from, 20);
  hex_to_bytes("0xF99dbd3CFc292b11F74DeEa9fa730825Ee0b56f2", -1, to, 20);
  bytes32_t pk;
  hex_to_bytes("0xDD6A4ADA615D13217F35711FAAB1CD119C2A5A6437D08B8DC4EACCF7DF0A2AC4", -1, pk, 32);
  eth_set_pk_signer(in3, pk);
  in3_set_default_signer(in3->signer);
  bytes_t* data = hex_to_new_bytes("0xf86c088504a817c80082520894f99dbd3cfc292b11f74deea9fa730825ee0b56f288016345785d8a0000802da089a9217cedb1fbe05f815264a355d339693fb80e4dc508c36656d62fa18695eaa04a3185a9a31d7d1feabd3f8652a15628e498eea03e0a08fe736a0ad67735affc", 223);

  // send the tx
  bytes_t* tx_hash = eth_sendTransaction(in3, from, to, OPTIONAL_T_VALUE(uint64_t, 0x96c0), OPTIONAL_T_VALUE(uint64_t, 0x9184e72a000), OPTIONAL_T_VALUE(uint256_t, to_uint256(0x9184e72a)), OPTIONAL_T_VALUE(bytes_t, *data), OPTIONAL_T_UNDEFINED(uint64_t));
  TEST_ASSERT_NOT_NULL(tx_hash);
  b_free(tx_hash);
  b_free(data);
  in3_free(in3);
}

static void test_eth_chain_id(void) {
  in3_t*     in3      = init_in3(mock_transport, 0x5);
  chain_id_t chain_id = eth_chainId(in3);
  TEST_ASSERT_TRUE(chain_id == 5);
  in3_free(in3);
}

static void test_eth_gas_price(void) {
  in3_t*   in3   = init_in3(mock_transport, 0x5);
  uint64_t price = eth_gasPrice(in3);
  TEST_ASSERT_TRUE(price > 1);
  in3_free(in3);
}

static void test_eth_getblock_number(void) {
  in3_t*       in3   = init_in3(mock_transport, 0x5);
  eth_block_t* block = eth_getBlockByNumber(in3, BLKNUM(1692767), true);
  // if the result is null there was an error an we can get the latest error message from eth_lat_error()
  TEST_ASSERT_EQUAL_INT64(block->number, 1692767);
  free(block);
  in3_free(in3);
}

static void test_eth_get_storage_at(void) {
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  address_t contract;
  //setup lock access contract address to be excuted with eth_call
  hex_to_bytes("0x36643F8D17FE745a69A2Fd22188921Fade60a98B", -1, contract, 20);
  bytes32_t key;
  memset(key, 0, 32);
  uint256_t storage = eth_getStorageAt(in3, contract, key, BLKNUM_EARLIEST());

  // if the result is null there was an error an we can get the latest error message from eth_lat_error()
  TEST_ASSERT_TRUE(storage.data);
  in3_free(in3);
}

static void test_eth_getblock_txcount_number(void) {
  in3_t* in3 = init_in3(mock_transport, 0x5);

  uint64_t tx_count = eth_getBlockTransactionCountByNumber(in3, BLKNUM(1692767));
  // we expect this to fail as we dont have verification for this
  char* error = eth_last_error();
  in3_log_debug("error found: %s", error);
  TEST_ASSERT_EQUAL_INT64(tx_count,6ll);
  in3_free(in3);
}

static void test_eth_getblock_txcount_hash(void) {
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  bytes32_t blk_hash;
  hex_to_bytes("0x1c9d592c4ad3fba02f7aa063e8048b3ff12551fd377e78061ab6ad146cc8df4d", -1, blk_hash, 32);

  uint64_t tx_count = eth_getBlockTransactionCountByHash(in3, blk_hash);
  char*    error    = eth_last_error();
  in3_log_debug("error found: %s", error);
  TEST_ASSERT_EQUAL_INT64(tx_count,2ll);
  in3_free(in3);
}

static void test_eth_getblock_hash(void) {
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  bytes32_t blk_hash;
  // 0x9cd22d209f24344147494d05d13f335b6e63af930abdc60f3db63627589e1438
  hex_to_bytes("0x1c9d592c4ad3fba02f7aa063e8048b3ff12551fd377e78061ab6ad146cc8df4d", -1, blk_hash, 32);

  //eth_block_t* block = eth_getBlockByNumber(in3, BLKNUM_EARLIEST(), false);
  eth_block_t* block = eth_getBlockByHash(in3, blk_hash, false);
  TEST_ASSERT_EQUAL_INT64(block->number, 1550244);
  free(block);
  in3_free(in3);
}

static void test_eth_call_fn(void) {
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  address_t contract;
  //setup lock access contract address to be excuted with eth_call
  hex_to_bytes("0x36643F8D17FE745a69A2Fd22188921Fade60a98B", -1, contract, 20);
  //ask for the access to the lock
  json_ctx_t* response = eth_call_fn(in3, contract, BLKNUM_LATEST(), "hasAccess():bool");
  if (!response) {
    in3_log_debug("Could not get the response: %s", eth_last_error());
    return;
  }
  //convert the response to a uint32_t,
  uint8_t access = d_int(response->result);
  in3_log_debug("Access granted? : %d \n", access);

  //    clean up resources
  json_free(response);
  TEST_ASSERT_TRUE(access == 1);
  in3_free(in3);
}

static void test_eth_get_code(void) {
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  address_t contract;
  //setup lock access contract address to be excuted with eth_call
  hex_to_bytes("0x36643F8D17FE745a69A2Fd22188921Fade60a98B", -1, contract, 20);
  //ask for the access to the lock
  bytes_t code = eth_getCode(in3, contract, BLKNUM_LATEST());
  //    clean up resources
  TEST_ASSERT_TRUE(code.len > 0);
  in3_free(in3);
}

static void test_estimate_fn(void) {
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  address_t contract;
  //setup lock access contract address to be excuted with eth_call
  hex_to_bytes("0x36643F8D17FE745a69A2Fd22188921Fade60a98B", -1, contract, 20);
  //ask for the access to the lock
  uint64_t estimate = eth_estimate_fn(in3, contract, BLKNUM_LATEST(), "hasAccess():bool");
  //convert the response to a uint32_t,
  in3_log_debug("Gas estimate : %lld \n", estimate);
  TEST_ASSERT_TRUE(estimate > 0);
  in3_free(in3);

  // Test ABI arg parsing
  in3 = init_in3(test_transport, 0x5);
  add_response("eth_estimateGas",
               "[{\"to\":\"0x36643f8d17fe745a69a2fd22188921fade60a98b\",\"data\":\"0x8a843727000000000000000000000000000000000000000000000000000000000000000000000000000000000000000036643f8d17fe745a69a2fd22188921fade60a98b00000000000000000000000000000000000000000000000000000000000000c0ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe6000000000000000000000000000000000000000000000000000000000000ffff0000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000000b536f6d6520737472696e67000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001436643f8d17fe745a69a2fd22188921fade60a98b000000000000000000000000\"},\"latest\"]",
               "\"0x123123\"",
               NULL,
               NULL);
  uint64_t gas = eth_estimate_fn(in3,
                                 contract,
                                 BLKNUM_LATEST(),
                                 "mockMethod(bool,address,string,int8,uint16,bytes):bool",
                                 false, contract, "Some string", "-26", UINT16_MAX, (bytes_t){.data = contract, .len = 20});
  TEST_ASSERT_EQUAL_UINT64(0x123123, gas);
  in3_free(in3);
}

static void test_get_uncle_count_blknum(void) {
  in3_t*    in3 = init_in3(mock_transport, 0x1);
  bytes32_t blk_hash;
  // 0x9cd22d209f24344147494d05d13f335b6e63af930abdc60f3db63627589e1438
  hex_to_bytes("0x1c9d592c4ad3fba02f7aa063e8048b3ff12551fd377e78061ab6ad146cc8df4d", -1, blk_hash, 32);
  //ask for the access to the lock
  uint64_t count = eth_getUncleCountByBlockNumber(in3, BLKNUM(56160));
  //we expect this to fail we dont have verification for this
  char* error = eth_last_error();
  in3_log_debug("error found: %s", error);
  TEST_ASSERT_TRUE(!count);
  in3_free(in3);
}

static void test_get_uncle_count_blkhash(void) {
  in3_t*    in3 = init_in3(mock_transport, 0x1);
  bytes32_t blk_hash;
  // 0x9cd22d209f24344147494d05d13f335b6e63af930abdc60f3db63627589e1438
  hex_to_bytes("0x685b2226cbf6e1f890211010aa192bf16f0a0cba9534264a033b023d7367b845", -1, blk_hash, 32);
  //ask for the access to the lock
  uint64_t count = eth_getUncleCountByBlockHash(in3, blk_hash);
  //we expect this to fail we dont have verification for this
  char* error = eth_last_error();
  in3_log_debug("error found: %s", error);
  TEST_ASSERT_TRUE(!count);
  in3_free(in3);
}

static void test_get_uncle_blknum_index(void) {
  in3_t* in3 = init_in3(mock_transport, 0x1);
  //get block number
  eth_block_t* block = eth_getUncleByBlockNumberAndIndex(in3, BLKNUM(56160), 0);
  //we expect this to fail we dont have verification for this
  char* error = eth_last_error();
  in3_log_debug("error found: %s", error);
  TEST_ASSERT_TRUE(!block);
  in3_free(in3);
}

static void test_utilities(void) {
  uint256_t u256 = {0};
  bytes32_t var;
  memset(var, 0, 32);
  hex_to_bytes("0xac1b824795e1eb1f", -1, var, 32);
  uint256_set(var, 32, u256.data);
  long double d = as_double(u256);
  TEST_ASSERT_TRUE(d > 0.0);
  uint64_t l = as_long(u256);
}

static void test_eth_call_multiple(void) {
  address_t contract;
  in3_t*    c = init_in3(test_transport, 0x5);
  c->proof    = PROOF_NONE;

  add_response("eth_call",
               "[{\"to\":\"0x2736d225f85740f42d17987100dc8d58e9e16252\",\"data\":\"0x15625c5e\"},\"latest\"]",
               "\"0x0000000000000000000000000000000000000000000000000000000000000005\"",
               NULL,
               NULL);
  hex_to_bytes("0x2736D225f85740f42D17987100dc8d58e9e16252", -1, contract, 20);
  json_ctx_t* response = eth_call_fn(c, contract, BLKNUM_LATEST(), "totalServers():uint256");
  TEST_ASSERT_NOT_NULL(response);
  json_free(response);

  add_response("eth_call",
               "[{\"to\":\"0x2736d225f85740f42d17987100dc8d58e9e16252\",\"data\":\"0x5cf0f3570000000000000000000000000000000000000000000000000000000000000000\"},\"latest\"]",
               "\"0x00000000000000000000000000000000000000000000000000000000000000e0000000000000000000000000784bfa9eb182c3a02dbeb5285e3dba92d717e07a000000000000000000000000000000000000000000000000000000000000ffff000000000000000000000000000000000000000000000000000000000000ffff000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002168747470733a2f2f696e332e736c6f636b2e69742f6d61696e6e65742f6e642d3100000000000000000000000000000000000000000000000000000000000000\"",
               NULL,
               NULL);
  response = eth_call_fn(c, contract, BLKNUM_LATEST(), "servers(uint256):(string,address,uint,uint,uint,address)", to_uint256(0));
  TEST_ASSERT_NOT_NULL(response);

  char*    url     = d_get_string_at(response->result, 0); // get the first item of the result (the url)
  bytes_t* owner   = d_get_bytes_at(response->result, 1);  // get the second item of the result (the owner)
  uint64_t deposit = d_get_long_at(response->result, 2);   // get the third item of the result (the deposit)

  TEST_ASSERT_EQUAL_STRING("https://in3.slock.it/mainnet/nd-1", url);
  TEST_ASSERT_EQUAL_UINT64(65535, deposit);

  bytes_t* owner_ = hex_to_new_bytes("784bfa9eb182c3a02dbeb5285e3dba92d717e07a", 40);
  TEST_ASSERT(b_cmp(owner_, owner));
  b_free(owner_);

  json_free(response);
  in3_free(c);
}

static void test_get_result_no_error(void) {
  in3_t* c = init_in3(test_transport, ETH_CHAIN_ID_MAINNET);
  c->proof = PROOF_NONE;
  add_response("eth_blockNumber", "[]", NULL, "{}", NULL);
  uint64_t blknum = eth_blockNumber(c);
  TEST_ASSERT_EQUAL(0, blknum);
  in3_free(c);
}

static void test_get_nonexistent_block(void) {
  in3_t* c = init_in3(test_transport, ETH_CHAIN_ID_MAINNET);
  c->proof = PROOF_NONE;
  add_response("eth_getBlockByNumber", "[\"0xffffffffffffffff\",false]", "null", NULL, NULL);
  eth_block_t* blk = eth_getBlockByNumber(c, BLKNUM(UINT64_MAX), false);
  TEST_ASSERT_NULL(blk);
  in3_free(c);
}

static void test_wait_for_receipt(void) {
  in3_t*    c = init_in3(mock_transport, ETH_CHAIN_ID_GOERLI);
  bytes32_t blk_hash;
  hex_to_bytes("0x8e7fb87e95c69a780490fce3ea14b44c78366fc45baa6cb86a582166c10c6d9d", -1, blk_hash, 32);
  char* r = eth_wait_for_receipt(c, blk_hash);
  TEST_ASSERT_NOT_NULL(r);
  in3_free(c);

  c = init_in3(test_transport, ETH_CHAIN_ID_GOERLI);
  add_response("eth_getTransactionReceipt", "[\"0x8e7fb87e95c69a780490fce3ea14b44c78366fc45baa6cb86a582166c10c6d9d\"]", NULL, "Unknown error", NULL);
  r = eth_wait_for_receipt(c, blk_hash);
  TEST_ASSERT_NULL(r);
  in3_free(c);
}

static void test_send_raw_tx(void) {
  in3_t*   in3     = init_in3(mock_transport, 0x5);
  bytes_t* data    = hex_to_new_bytes("f8da098609184e72a0008296c094f99dbd3cfc292b11f74deea9fa730825ee0b56f2849184e72ab87000ff86c088504a817c80082520894f99dbd3cfc292b11f74deea9fa730825ee0b56f288016345785d8a0000802da089a9217cedb1fbe05f815264a355d339693fb80e4dc508c36656d62fa18695eaa04a3185a9a31d7d1feabd3f8652a15628e498eea03e0a08fe736a0ad67735affff2ea0936324cf235541114275bb72b5acfb5a5c1f6f6e7f426c94806ff4093539bfaaa010a7482378b19ee0930a77c14b18c5664b3aa6c3ebc7420954d81263625d6d6a", 440);
  bytes_t* tx_hash = eth_sendRawTransaction(in3, *data);
  TEST_ASSERT_NOT_NULL(tx_hash);
  b_free(tx_hash);
  b_free(data);
}

/*
 * Main
 */
int main() {
  in3_log_set_quiet(true);
  in3_log_set_level(LOG_ERROR);
  in3_register_eth_full();

  // now run tests
  TESTS_BEGIN();
  //PASSING..
  RUN_TEST(test_eth_chain_id);
  RUN_TEST(test_eth_get_storage_at);
  RUN_TEST(test_get_balance);
  RUN_TEST(test_block_number);
  RUN_TEST(test_eth_gas_price);
  RUN_TEST(test_eth_getblock_number);
  RUN_TEST(test_eth_getblock_hash);
  RUN_TEST(test_get_logs);
  RUN_TEST(test_eth_call_fn);
  RUN_TEST(test_get_tx_blkhash_index);
  RUN_TEST(test_get_tx_blknum_index);
  RUN_TEST(test_get_tx_count);
  RUN_TEST(test_get_tx_receipt);
  RUN_TEST(test_eth_call_fn);
  RUN_TEST(test_eth_get_code);
  RUN_TEST(test_estimate_fn);
  RUN_TEST(test_get_uncle_blknum_index);
  RUN_TEST(test_get_uncle_count_blkhash);
  RUN_TEST(test_get_uncle_count_blknum);
  RUN_TEST(test_new_pending_tx_filter);
  RUN_TEST(test_eth_getblock_txcount_hash);
  RUN_TEST(test_eth_getblock_txcount_number);
  RUN_TEST(test_get_filter_changes);
  RUN_TEST(test_new_block_filter);
  RUN_TEST(test_get_tx_hash);
  RUN_TEST(test_utilities);
  RUN_TEST(test_eth_call_multiple);
  RUN_TEST(test_get_result_no_error);
  RUN_TEST(test_get_nonexistent_block);
  RUN_TEST(test_wait_for_receipt);
  RUN_TEST(test_send_tx);
  RUN_TEST(test_send_raw_tx);
  return TESTS_END();
}
