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

#include "../../src/api/btc/btc_api.h"
#include "../../src/api/utils/api_utils.h"
#include "../../src/verifier/btc/btc.h"
#include "../test_utils.h"
#include "../util/transport.h"
#include <core/util/log.h>

static in3_t* in3_init_test() {
  in3_t* in3 = in3_for_chain(CHAIN_ID_BTC);
  register_transport(in3, mock_transport);
  in3_configure(in3, "{\"autoUpdateList\":false,\"maxAttempts\":1,\"nodes\":{\"0x99\": {\"needsUpdate\":false}}}");
  return in3;
}

void test_btc_api_get_transaction() {
  in3_t* in3 = in3_init_test();

  bytes32_t txid;
  hex_to_bytes("83ce5041679c75721ec7135e0ebeeae52636cfcb4844dbdccf86644df88da8c1", -1, txid, 32);

  // get it structured
  btc_transaction_t* data = btc_get_transaction(in3, txid);
  TEST_ASSERT_NOT_NULL_MESSAGE(data, api_last_error());
  TEST_ASSERT_EQUAL_UINT32(1, data->version);
  TEST_ASSERT_EQUAL_UINT32(247, data->size);
  TEST_ASSERT_EQUAL_UINT32(166, data->vsize);
  TEST_ASSERT_EQUAL_UINT32(661, data->weight);
  TEST_ASSERT_EQUAL_UINT32(0, data->locktime);
  TEST_ASSERT_EQUAL_UINT32(20, data->confirmations);
  TEST_ASSERT_EQUAL_UINT32(1589863750, data->time);
  TEST_ASSERT_EQUAL_UINT32(1589863750, data->blocktime);
  TEST_ASSERT_EQUAL_HEX_BYTES("83ce5041679c75721ec7135e0ebeeae52636cfcb4844dbdccf86644df88da8c1", data->txid, 32, "wrong txid");
  TEST_ASSERT_EQUAL_HEX_BYTES("4041e8162e2c1a9711b15fd2a2b0c7aae59fbc06a95667682f1271fab0393f69", data->hash, 32, "wrong hash");
  TEST_ASSERT_EQUAL_HEX_BYTES("01000000000101dccee3ce73ba66bc2d2602d647e1238a76d795cfb120f520ba64b0f085e2f694010000001716001430d71be06aa53fd845913f8613ed518d742d082affffffff02c0d8a7000000000017a914d129842dbe1ee73e69d14d54a8a62784877fb83e87108428030000000017a914e483fe5491d8ef5acf043fac5eb1af0f049a80318702473044022035c13c5fdf5f5d07c2101176db8a9c727cec9c31c612b15ae0a4cbdeb25b4dc2022046849e039477aa67fb60e24635668ae1de0bddb9ade3eac2d5ca350898d43c2b01210344715d54ec59240a4ae9f5d8e469f3933a7b03d5c09e15ac3ff53239ea1041b800000000", data->data.data, data->data.len, "wrong data");
  TEST_ASSERT_EQUAL_UINT32(1, data->vin_len);
  TEST_ASSERT_EQUAL_HEX_BYTES("16001430d71be06aa53fd845913f8613ed518d742d082a", data->vin->script.data, data->vin->script.len, "wrong vin-script");
  TEST_ASSERT_EQUAL_UINT32(4294967295, data->vin->sequence);
  TEST_ASSERT_EQUAL_UINT32(1, data->vin->vout);
  TEST_ASSERT_EQUAL_UINT32(2, data->vout_len);
  TEST_ASSERT_EQUAL_UINT32(1, data->vout[1].n);
  TEST_ASSERT_EQUAL_HEX_BYTES("a914d129842dbe1ee73e69d14d54a8a62784877fb83e87", data->vout->script_pubkey.data, data->vout->script_pubkey.len, "wrong vout-script");
  TEST_ASSERT_EQUAL_UINT64(11000000, data->vout->value);
  free(data);

  // test bytes
  bytes_t* bytes = btc_get_transaction_bytes(in3, txid);
  TEST_ASSERT_NOT_NULL(bytes);
  TEST_ASSERT_EQUAL_HEX_BYTES("01000000000101dccee3ce73ba66bc2d2602d647e1238a76d795cfb120f520ba64b0f085e2f694010000001716001430d71be06aa53fd845913f8613ed518d742d082affffffff02c0d8a7000000000017a914d129842dbe1ee73e69d14d54a8a62784877fb83e87108428030000000017a914e483fe5491d8ef5acf043fac5eb1af0f049a80318702473044022035c13c5fdf5f5d07c2101176db8a9c727cec9c31c612b15ae0a4cbdeb25b4dc2022046849e039477aa67fb60e24635668ae1de0bddb9ade3eac2d5ca350898d43c2b01210344715d54ec59240a4ae9f5d8e469f3933a7b03d5c09e15ac3ff53239ea1041b800000000", bytes->data, bytes->len, "wrong data");
  b_free(bytes);

  in3_free(in3);
}

void test_btc_api_get_blockheader() {
  in3_t* in3 = in3_init_test();

  bytes32_t hash;
  hex_to_bytes("00000000000000000007171457f3352e101d92bca75f055c330fe33e84bb183b", -1, hash, 32);

  // test structured data
  btc_blockheader_t* data = btc_get_blockheader(in3, hash);

  TEST_ASSERT_NOT_NULL(data);
  TEST_ASSERT_EQUAL_UINT32(536870912, data->version);
  TEST_ASSERT_EQUAL_UINT32(15, data->confirmations);
  TEST_ASSERT_EQUAL_UINT32(631076, data->height);
  TEST_ASSERT_EQUAL_UINT32(1589991753, data->time);
  TEST_ASSERT_EQUAL_UINT32(2921687714, data->nonce);
  TEST_ASSERT_EQUAL_UINT32(2339, data->n_tx);

  TEST_ASSERT_EQUAL_HEX_BYTES("00000000000000000007171457f3352e101d92bca75f055c330fe33e84bb183b", data->hash, 32, "wrong hash");
  TEST_ASSERT_EQUAL_HEX_BYTES("18f5756c66b14aa8bace933001b16d78bd89a16612468122442559ce9f296eb6", data->merkleroot, 32, "wrong merkleroot");
  TEST_ASSERT_EQUAL_HEX_BYTES("00000000000000000000000000000000000000000f9f574f8d39680a92ad1bdc", data->chainwork, 32, "wrong chainwork");
  TEST_ASSERT_EQUAL_HEX_BYTES("000000000000000000061e12d6a29bd0175a6045dfffeafd950c0513f9b82c80", data->previous_hash, 32, "wrong previous hash");
  TEST_ASSERT_EQUAL_HEX_BYTES("00000000000000000000eac6e799c468b3a140d9e1400c31f7603fdb20e1198d", data->next_hash, 32, "wrong next hash");
  TEST_ASSERT_EQUAL_HEX_BYTES("171297f6", data->bits, 4, "wrong bits");
  TEST_ASSERT_EQUAL_HEX_BYTES("00000020802cb8f913050c95fdeaffdf45605a17d09ba2d6121e06000000000000000000b66e299fce5925442281461266a189bd786db1013093cebaa84ab1666c75f5184959c55ef6971217a26a25ae", data->data, 80, "wrong data");

  free(data);

  // test bytes
  bytes_t* bytes = btc_get_blockheader_bytes(in3, hash);
  TEST_ASSERT_NOT_NULL(bytes);
  TEST_ASSERT_EQUAL_HEX_BYTES("00000020802cb8f913050c95fdeaffdf45605a17d09ba2d6121e06000000000000000000b66e299fce5925442281461266a189bd786db1013093cebaa84ab1666c75f5184959c55ef6971217a26a25ae", bytes->data, bytes->len, "wrong data");
  b_free(bytes);

  in3_free(in3);
}

void test_btc_api_get_block() {
  in3_t* in3 = in3_init_test();

  bytes32_t hash;
  hex_to_bytes("00000000000000000007171457f3352e101d92bca75f055c330fe33e84bb183b", -1, hash, 32);

  // get it with txids only
  btc_block_txids_t* block = btc_get_block_txids(in3, hash);
  TEST_ASSERT_NOT_NULL(block);
  btc_blockheader_t* data = &block->header;
  TEST_ASSERT_NOT_NULL(data);
  TEST_ASSERT_EQUAL_UINT32(536870912, data->version);
  TEST_ASSERT_EQUAL_UINT32(15, data->confirmations);
  TEST_ASSERT_EQUAL_UINT32(631076, data->height);
  TEST_ASSERT_EQUAL_UINT32(1589991753, data->time);
  TEST_ASSERT_EQUAL_UINT32(2921687714, data->nonce);
  TEST_ASSERT_EQUAL_UINT32(2339, data->n_tx);

  TEST_ASSERT_EQUAL_HEX_BYTES("00000000000000000007171457f3352e101d92bca75f055c330fe33e84bb183b", data->hash, 32, "wrong hash");
  TEST_ASSERT_EQUAL_HEX_BYTES("18f5756c66b14aa8bace933001b16d78bd89a16612468122442559ce9f296eb6", data->merkleroot, 32, "wrong merkleroot");
  TEST_ASSERT_EQUAL_HEX_BYTES("00000000000000000000000000000000000000000f9f574f8d39680a92ad1bdc", data->chainwork, 32, "wrong chainwork");
  TEST_ASSERT_EQUAL_HEX_BYTES("000000000000000000061e12d6a29bd0175a6045dfffeafd950c0513f9b82c80", data->previous_hash, 32, "wrong previous hash");
  TEST_ASSERT_EQUAL_HEX_BYTES("00000000000000000000eac6e799c468b3a140d9e1400c31f7603fdb20e1198d", data->next_hash, 32, "wrong next hash");
  TEST_ASSERT_EQUAL_HEX_BYTES("171297f6", data->bits, 4, "wrong bits");
  TEST_ASSERT_EQUAL_HEX_BYTES("00000020802cb8f913050c95fdeaffdf45605a17d09ba2d6121e06000000000000000000b66e299fce5925442281461266a189bd786db1013093cebaa84ab1666c75f5184959c55ef6971217a26a25ae", data->data, 80, "wrong data");

  TEST_ASSERT_EQUAL_HEX_BYTES("ee14c60b365a068744496644c1b5643c4ecf27c6bdb2d9d7ad96df180f6aecc8", block->tx[1], 32, "wrong tx id");

  free(block);

  // get it with full tx (takes long)
  btc_block_txdata_t* b = btc_get_block_txdata(in3, hash);
  TEST_ASSERT_NOT_NULL_MESSAGE(b, btc_last_error());
  data = &b->header;
  TEST_ASSERT_NOT_NULL(data);
  TEST_ASSERT_EQUAL_UINT32(536870912, data->version);
  TEST_ASSERT_EQUAL_UINT32(2946, data->confirmations);
  TEST_ASSERT_EQUAL_UINT32(631076, data->height);
  TEST_ASSERT_EQUAL_UINT32(1589991753, data->time);
  TEST_ASSERT_EQUAL_UINT32(2921687714, data->nonce);
  TEST_ASSERT_EQUAL_UINT32(2339, data->n_tx);

  TEST_ASSERT_EQUAL_HEX_BYTES("00000000000000000007171457f3352e101d92bca75f055c330fe33e84bb183b", data->hash, 32, "wrong hash");
  TEST_ASSERT_EQUAL_HEX_BYTES("18f5756c66b14aa8bace933001b16d78bd89a16612468122442559ce9f296eb6", data->merkleroot, 32, "wrong merkleroot");
  TEST_ASSERT_EQUAL_HEX_BYTES("00000000000000000000000000000000000000000f9f574f8d39680a92ad1bdc", data->chainwork, 32, "wrong chainwork");
  TEST_ASSERT_EQUAL_HEX_BYTES("000000000000000000061e12d6a29bd0175a6045dfffeafd950c0513f9b82c80", data->previous_hash, 32, "wrong previous hash");
  TEST_ASSERT_EQUAL_HEX_BYTES("00000000000000000000eac6e799c468b3a140d9e1400c31f7603fdb20e1198d", data->next_hash, 32, "wrong next hash");
  TEST_ASSERT_EQUAL_HEX_BYTES("171297f6", data->bits, 4, "wrong bits");
  TEST_ASSERT_EQUAL_HEX_BYTES("00000020802cb8f913050c95fdeaffdf45605a17d09ba2d6121e06000000000000000000b66e299fce5925442281461266a189bd786db1013093cebaa84ab1666c75f5184959c55ef6971217a26a25ae", data->data, 80, "wrong data");

  TEST_ASSERT_EQUAL_HEX_BYTES("ee14c60b365a068744496644c1b5643c4ecf27c6bdb2d9d7ad96df180f6aecc8", b->tx[1].txid, 32, "wrong tx id");

  free(b);

  // test bytes
  bytes_t* bytes = btc_get_block_bytes(in3, hash);
  TEST_ASSERT_NOT_NULL(bytes);
  TEST_ASSERT_EQUAL_HEX_BYTES("00000020802cb8f913050c95fdeaffdf45605a17d09ba2d6121e06000000000000000000b66e299fce5925442281461266a189bd786db1013093cebaa84ab1666c75f5184959c55ef6971217a26a25ae", bytes->data, 80, "wrong data");
  TEST_ASSERT_EQUAL(1460409, bytes->len);
  b_free(bytes);

  in3_free(in3);
}

/*
 * Main
 */
int main() {
  in3_register_btc();
  in3_log_set_quiet(true);

  TESTS_BEGIN();
  RUN_TEST(test_btc_api_get_blockheader);
  RUN_TEST(test_btc_api_get_block);
  RUN_TEST(test_btc_api_get_transaction);
  return TESTS_END();
}
