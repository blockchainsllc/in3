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
#include "../../src/core/util/scache.h"
#include "../../src/verifier/eth1/basic/signer.h"
#include "../../src/verifier/eth1/full/eth_full.h"
#include "../../src/verifier/eth1/nano/eth_nano.h"

#include "../test_utils.h"
#include "../util/transport.h"
#include <stdio.h>
#include <unistd.h>

in3_t* init_in3(in3_transport_send custom_transport, uint64_t chain) {
  in3_t* in3 = NULL;
  int    err;
  in3_register_eth_full();
  in3 = in3_new();
  if (custom_transport)
    in3->transport = custom_transport; // use curl to handle the requests
  in3->requestCount   = 1;             // number of requests to sendp
  in3->includeCode    = 1;
  in3->chainId        = chain;
  in3->max_attempts   = 1;
  in3->requestCount   = 1; // number of requests to sendp
  in3->autoUpdateList = false;
  for (int i = 0; i < in3->chainsCount; i++) in3->chains[i].needsUpdate = false;
  return in3;
}

static void test_get_balance() {
  in3_t* in3 = init_in3(mock_transport, 0x5);
  // the address of account whose balance we want to get
  address_t account;
  hex2byte_arr("0xF99dbd3CFc292b11F74DeEa9fa730825Ee0b56f2", -1, account, 20);
  // get balance of account
  long double balance = as_double(eth_getBalance(in3, account, BLKNUM(1555415)));
  printf("Balance: %Lf\n", balance);
  TEST_ASSERT_TRUE(balance > 0.0);
  _free(in3);
}

static void test_get_tx_count() {
  in3_t* in3 = init_in3(mock_transport, 0x5);

  // the address of account whose balance we want to get
  address_t account;
  hex2byte_arr("0x0de496ae79194d5f5b18eb66987b504a0feb32f2", -1, account, 20);
  // get balance of account
  uint64_t count = eth_getTransactionCount(in3, account, BLKNUM_LATEST());
  in3_log_debug("tx count %llu\n", count);

  TEST_ASSERT_TRUE(count > 0.0);
  _free(in3);
}

static void test_new_block_filter() {
  in3_t* in3 = init_in3(mock_transport, 0x5);
  // we can add any mock json as we need trasnport but we are not calling any rpc endpoint
  //get filter id for new block
  size_t fid = eth_newBlockFilter(in3);
  TEST_ASSERT_TRUE(fid > 0);
  _free(in3);
}

static void test_block_number() {
  in3_t*   in3    = init_in3(mock_transport, 0x5);
  uint64_t blknum = eth_blockNumber(in3);
  TEST_ASSERT_TRUE(blknum > 0);
  _free(in3);
}

static void test_new_pending_tx_filter() {
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  in3_ret_t ret = eth_newPendingTransactionFilter(in3);
  //we expect this to fail we dont support pending
  TEST_ASSERT_TRUE(ret == IN3_ENOTSUP);
  _free(in3);
}

static void test_get_filter_changes() {

  in3_t*    in3 = init_in3(mock_transport, 0x5);
  bytes32_t blk_hash1;
  hex2byte_arr("0xbaf52e8d5e9c7ece67b1c3a0788379a4f486d8ec50bbf531b3a6720ca03fe1c4", -1, blk_hash1, 32);
  bytes32_t blk_hash2;
  hex2byte_arr("0xbaf52e8d5e9c7ece67b1c3a0788379a4f486d8ec50bbf531b3a6720ca03fe1c4", -1, blk_hash2, 32);
  bytes32_t** hashes = _malloc(sizeof(bytes32_t*) * 2);
  hashes[0]          = &blk_hash1;
  hashes[1]          = &blk_hash2;
  char b[30];
  sprintf(b, "{\"fromBlock\":\"0x%" PRIx64 "\"}", eth_blockNumber(in3) - 2);
  json_ctx_t* jopt = parse_json(b);
  // Create new filter with options
  size_t fid = eth_newFilter(in3, jopt);
  // Get logs
  eth_log_t** logs_array = _malloc(sizeof(eth_log_t*) * 1);
  eth_log_t*  logs       = NULL;
  in3_ret_t   ret_logs   = eth_getFilterLogs(in3, fid, &logs);
  logs_array[0]          = logs;

  size_t ret = eth_getFilterChanges(in3, 0, hashes, logs_array);
  in3_log_debug("ret %d\n", ret);
  TEST_ASSERT_TRUE(ret > 0);
  _free(in3);
}

static void test_get_logs() {
  in3_t* in3 = init_in3(mock_transport, 0x5);
  // Create filter options
  char b[30];
  sprintf(b, "{\"fromBlock\":\"0x%" PRIx64 "\"}", eth_blockNumber(in3) - 2);
  json_ctx_t* jopt = parse_json(b);

  // Create new filter with options
  size_t fid = eth_newFilter(in3, jopt);

  // Get logs
  eth_log_t* logs = NULL;
  in3_ret_t  ret  = eth_getFilterLogs(in3, fid, &logs);
  if (ret != IN3_OK) {
    printf("eth_getFilterLogs() failed [%d]\n", ret);
    return;
  }
  while (logs) {
    eth_log_t* l = logs;
    printf("--------------------------------------------------------------------------------\n");
    printf("\tlogId: %lu\n", l->log_index);
    printf("\tTxId: %lu\n", l->transaction_index);
    printf("\thash: ");
    ba_print(l->block_hash, 32);
    printf("\n\tnum: %" PRIu64 "\n", l->block_number);
    printf("\taddress: ");
    ba_print(l->address, 20);
    printf("\n\tdata: ");
    b_print(&l->data);
    printf("\ttopics[%lu]: ", l->topic_count);
    for (size_t i = 0; i < l->topic_count; i++) {
      printf("\n\t");
      ba_print(l->topics[i], 32);
    }
    printf("\n");
    logs = logs->next;
    free(l->data.data);
    free(l->topics);
    free(l);
  }
  eth_uninstallFilter(in3, fid);
  free_json(jopt);

  TEST_ASSERT_TRUE(ret == IN3_OK);
  _free(in3);
}
static void test_get_tx_blkhash_index(void) {
  // the hash of transaction that we want to get
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  bytes32_t blk_hash;
  hex2byte_arr("0xbaf52e8d5e9c7ece67b1c3a0788379a4f486d8ec50bbf531b3a6720ca03fe1c4", -1, blk_hash, 32);

  // get the tx by hash
  eth_tx_t* tx = eth_getTransactionByBlockHashAndIndex(in3, blk_hash, 0);

  // if the result is null there was an error an we can get the latest error message from eth_last_error()
  if (!tx)
    printf("error getting the tx : %s\n", eth_last_error());
  else {
    printf("Transaction #%d of block #%" PRId64, tx->transaction_index, tx->block_number);
    free(tx);
  }
  TEST_ASSERT_TRUE(tx != NULL);
  _free(in3);
}

static void test_get_tx_blknum_index(void) {
  // the hash of transaction that we want to get
  in3_t* in3 = init_in3(mock_transport, 0x5);
  // get the tx by hash
  eth_tx_t* tx = eth_getTransactionByBlockNumberAndIndex(in3, BLKNUM(1723267), 0);

  // if the result is null there was an error an we can get the latest error message from eth_last_error()
  if (!tx)
    printf("error getting the tx : %s\n", eth_last_error());
  else {
    printf("Transaction #%d of block #%" PRIx64, tx->transaction_index, tx->block_number);
    free(tx);
  }
  TEST_ASSERT_TRUE(tx != NULL);
  _free(in3);
}

static void test_get_tx_hash(void) {
  // the hash of transaction that we want to get
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  bytes32_t tx_hash;
  hex2byte_arr("0x9241334b0b568ef6cd44d80e37a0ce14de05557a3cfa98b5fd1d006204caf164", -1, tx_hash, 32);
  // get the tx by hash
  eth_tx_t* tx = eth_getTransactionByHash(in3, tx_hash);

  // if the result is null there was an error an we can get the latest error message from eth_last_error()
  if (!tx)
    printf("error getting the tx : %s\n", eth_last_error());
  else {
    printf("Transaction #%d of block #%" PRIx64, tx->transaction_index, tx->block_number);
    //free(tx);
  }
  TEST_ASSERT_TRUE(tx != NULL);
  _free(in3);
}

static void test_get_tx_receipt(void) {
  // the hash of transaction whose receipt we want to get

  in3_t*    in3 = init_in3(mock_transport, 0x5);
  bytes32_t tx_hash;
  hex2byte_arr("0x8e7fb87e95c69a780490fce3ea14b44c78366fc45baa6cb86a582166c10c6d9d", -1, tx_hash, 32);

  // get the tx receipt by hash
  eth_tx_receipt_t* txr = eth_getTransactionReceipt(in3, tx_hash);

  // if the result is null there was an error an we can get the latest error message from eth_last_error()
  if (!txr)
    printf("error getting the tx : %s\n", eth_last_error());
  else {
    printf("Transaction #%d of block #%" PRIx64 ", gas used = %" PRIu64 ", status = %s\n", txr->transaction_index, txr->block_number, txr->gas_used, txr->status ? "success" : "failed");
    //free_tx_receipt(txr);
  }
  TEST_ASSERT_TRUE(txr);
  TEST_ASSERT_TRUE(txr->status);
  TEST_ASSERT_TRUE(txr->gas_used > 0);
  _free(in3);
}

static void test_send_tx(void) {
  in3_t* in3 = init_in3(mock_transport, 0x5);
  // prepare parameters
  address_t to, from;
  hex2byte_arr("0x0dE496AE79194D5F5b18eB66987B504A0FEB32f2", -1, from, 20);
  hex2byte_arr("0xF99dbd3CFc292b11F74DeEa9fa730825Ee0b56f2", -1, to, 20);
  bytes32_t pk;
  hex2byte_arr("0xDD6A4ADA615D13217F35711FAAB1CD119C2A5A6437D08B8DC4EACCF7DF0A2AC4", -1, pk, 32);
  eth_set_pk_signer(in3, pk);
  in3_set_default_signer(in3->signer);
  bytes_t* data = hex2byte_new_bytes("0xf86c088504a817c80082520894f99dbd3cfc292b11f74deea9fa730825ee0b56f288016345785d8a0000802da089a9217cedb1fbe05f815264a355d339693fb80e4dc508c36656d62fa18695eaa04a3185a9a31d7d1feabd3f8652a15628e498eea03e0a08fe736a0ad67735affc", 223);

  // send the tx
  bytes_t* tx_hash = eth_sendTransaction(in3, from, to, OPTIONAL_T_VALUE(uint64_t, 0x96c0), OPTIONAL_T_VALUE(uint64_t, 0x9184e72a000), OPTIONAL_T_VALUE(uint256_t, to_uint256(0x9184e72a)), OPTIONAL_T_VALUE(bytes_t, *data), OPTIONAL_T_UNDEFINED(uint64_t));

  // if the result is null there was an error and we can get the latest error message from eth_last_error()
  if (!tx_hash)
    printf("error sending the tx : %s\n", eth_last_error());
  else {
    printf("Transaction hash: ");
    b_print(tx_hash);
    //b_free(tx_hash);
  }
  //b_free(data);
  TEST_ASSERT_TRUE(tx_hash);
  _free(in3);
}

static void test_eth_chain_id(void) {
  in3_t*   in3      = init_in3(mock_transport, 0x5);
  uint64_t chain_id = eth_chainId(in3);
  TEST_ASSERT_TRUE(chain_id == 5);
  _free(in3);
}

static void test_eth_gas_price(void) {
  in3_t*   in3   = init_in3(mock_transport, 0x5);
  uint64_t price = eth_gasPrice(in3);
  TEST_ASSERT_TRUE(price > 1);
  _free(in3);
}

static void test_eth_getblock_number(void) {
  in3_t*       in3   = init_in3(mock_transport, 0x5);
  eth_block_t* block = eth_getBlockByNumber(in3, BLKNUM(1692767), false);

  // if the result is null there was an error an we can get the latest error message from eth_lat_error()
  uint64_t blk_number = 0;
  // if the result is null there was an error an we can get the latest error message from eth_lat_error()
  if (!block)
    printf("error getting the block : %s\n", eth_last_error());
  else {
    blk_number = (u_int64_t) block->number;
    printf("Number of transactions in Block #%" PRIu64 ": %d\n", blk_number, block->tx_count);
    free(block);
  }
  TEST_ASSERT_EQUAL_INT64(blk_number, 1692767);
  _free(in3);
}

static void test_eth_get_storage_at(void) {
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  address_t contract;
  //setup lock access contract address to be excuted with eth_call
  hex2byte_arr("0x36643F8D17FE745a69A2Fd22188921Fade60a98B", -1, contract, 20);
  bytes32_t key;
  memset(key, 0, 32);
  uint256_t storage = eth_getStorageAt(in3, contract, key, BLKNUM_EARLIEST());

  // if the result is null there was an error an we can get the latest error message from eth_lat_error()
  TEST_ASSERT_TRUE(storage.data);
  _free(in3);
}

static void test_eth_getblock_txcount_number(void) {
  in3_t* in3 = init_in3(mock_transport, 0x5);

  uint64_t tx_count = eth_getBlockTransactionCountByNumber(in3, BLKNUM(1692767));
  // we expect this to fail as we dont have verification for this
  char* error = eth_last_error();
  in3_log_debug("error found: %s", error);
  TEST_ASSERT_TRUE(!tx_count);
  _free(in3);
}

static void test_eth_getblock_txcount_hash(void) {
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  bytes32_t blk_hash;
  hex2byte_arr("0x1c9d592c4ad3fba02f7aa063e8048b3ff12551fd377e78061ab6ad146cc8df4d", -1, blk_hash, 32);

  uint64_t tx_count = eth_getBlockTransactionCountByHash(in3, blk_hash);
  char*    error    = eth_last_error();
  in3_log_debug("error found: %s", error);
  TEST_ASSERT_TRUE(!tx_count);
  _free(in3);
}

static void test_eth_getblock_hash(void) {
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  bytes32_t blk_hash;
  // 0x9cd22d209f24344147494d05d13f335b6e63af930abdc60f3db63627589e1438
  hex2byte_arr("0x1c9d592c4ad3fba02f7aa063e8048b3ff12551fd377e78061ab6ad146cc8df4d", -1, blk_hash, 32);

  //eth_block_t* block = eth_getBlockByNumber(in3, BLKNUM_EARLIEST(), false);
  eth_block_t* block      = eth_getBlockByHash(in3, blk_hash, false);
  uint64_t     blk_number = 0;
  // if the result is null there was an error an we can get the latest error message from eth_lat_error()
  if (!block)
    printf("error getting the block : %s\n", eth_last_error());
  else {
    blk_number = (u_int64_t) block->number;
    printf("Number of transactions in Block #%" PRIu64 ": %d\n", blk_number, (uint32_t) block->tx_count);
    free(block);
  }
  TEST_ASSERT_EQUAL_INT64(blk_number, 1550244);
  _free(in3);
}

static void test_eth_call_fn(void) {
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  address_t contract;
  //setup lock access contract address to be excuted with eth_call
  hex2byte_arr("0x36643F8D17FE745a69A2Fd22188921Fade60a98B", -1, contract, 20);
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
  free_json(response);
  TEST_ASSERT_TRUE(access == 1);
  _free(in3);
}

static void test_eth_get_code(void) {
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  address_t contract;
  //setup lock access contract address to be excuted with eth_call
  hex2byte_arr("0x36643F8D17FE745a69A2Fd22188921Fade60a98B", -1, contract, 20);
  //ask for the access to the lock
  bytes_t code = eth_getCode(in3, contract, BLKNUM_LATEST());
  //    clean up resources
  TEST_ASSERT_TRUE(code.len > 0);
  _free(in3);
}

static void test_estimate_fn(void) {
  in3_t*    in3 = init_in3(mock_transport, 0x5);
  address_t contract;
  //setup lock access contract address to be excuted with eth_call
  hex2byte_arr("0x36643F8D17FE745a69A2Fd22188921Fade60a98B", -1, contract, 20);
  //ask for the access to the lock
  uint64_t estimate = eth_estimate_fn(in3, contract, BLKNUM_LATEST(), "hasAccess():bool");
  //convert the response to a uint32_t,
  in3_log_debug("Gas estimate : %lld \n", estimate);
  TEST_ASSERT_TRUE(estimate > 0);
  _free(in3);
}

static void test_get_uncle_count_blknum(void) {
  in3_t*    in3 = init_in3(mock_transport, 0x1);
  bytes32_t blk_hash;
  // 0x9cd22d209f24344147494d05d13f335b6e63af930abdc60f3db63627589e1438
  hex2byte_arr("0x1c9d592c4ad3fba02f7aa063e8048b3ff12551fd377e78061ab6ad146cc8df4d", -1, blk_hash, 32);
  //ask for the access to the lock
  uint64_t count = eth_getUncleCountByBlockNumber(in3, BLKNUM(56160));
  //we expect this to fail we dont have verification for this
  char* error = eth_last_error();
  in3_log_debug("error found: %s", error);
  TEST_ASSERT_TRUE(!count);
  _free(in3);
}
static void test_get_uncle_count_blkhash(void) {
  in3_t*    in3 = init_in3(mock_transport, 0x1);
  bytes32_t blk_hash;
  // 0x9cd22d209f24344147494d05d13f335b6e63af930abdc60f3db63627589e1438
  hex2byte_arr("0x685b2226cbf6e1f890211010aa192bf16f0a0cba9534264a033b023d7367b845", -1, blk_hash, 32);
  //ask for the access to the lock
  uint64_t count = eth_getUncleCountByBlockHash(in3, blk_hash);
  //we expect this to fail we dont have verification for this
  char* error = eth_last_error();
  in3_log_debug("error found: %s", error);
  TEST_ASSERT_TRUE(!count);
  _free(in3);
}

static void test_get_uncle_blknum_index(void) {
  in3_t* in3 = init_in3(mock_transport, 0x1);
  //get block number
  eth_block_t* block = eth_getUncleByBlockNumberAndIndex(in3, BLKNUM(56160), 0);
  //we expect this to fail we dont have verification for this
  char* error = eth_last_error();
  in3_log_debug("error found: %s", error);
  TEST_ASSERT_TRUE(!block);
  _free(in3);
}
/*
 * Main
 */
int main() {
  in3_log_set_quiet_(true);
  in3_log_set_level(LOG_ERROR);

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
  RUN_TEST(test_send_tx);
  RUN_TEST(test_eth_call_fn);
  RUN_TEST(test_eth_get_code);
  RUN_TEST(test_estimate_fn);
  // /* verification for chain_id not supported */
  RUN_TEST(test_get_uncle_blknum_index);
  RUN_TEST(test_get_uncle_count_blkhash);
  RUN_TEST(test_get_uncle_count_blknum);
  RUN_TEST(test_new_pending_tx_filter);
  RUN_TEST(test_eth_getblock_txcount_hash);
  RUN_TEST(test_eth_getblock_txcount_number);
  RUN_TEST(test_get_filter_changes);
  RUN_TEST(test_new_block_filter);
  RUN_TEST(test_get_tx_hash);

  return TESTS_END();
}
