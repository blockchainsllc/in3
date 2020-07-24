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

#include "../../include/in3/error.h"
#include "../../src/api/eth1/eth_api.h"
#include "../../src/core/client/cache.h"
#include "../../src/core/client/context.h"
#include "../../src/core/client/keys.h"
#include "../../src/core/client/nodelist.h"
#include "../../src/core/util/bytes.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/log.h"
#include "../../src/core/util/mem.h"
#include "../../src/core/util/scache.h"
#include "../../src/verifier/eth1/full/eth_full.h"
#include "../../src/verifier/eth1/nano/eth_nano.h"

#include "../../src/signer/pk-signer/signer.h"
#include "../../src/third-party/crypto/ecdsa.h"
#include "../../src/third-party/crypto/secp256k1.h"
#include "../../src/verifier/eth1/basic/eth_basic.h"
#include "../test_utils.h"
#include "../util/transport.h"
#include <stdio.h>
#include <unistd.h>
#define ETH_PRIVATE_KEY "0x8da4ef21b864d2cc526dbdb2a120bd2874c36c9d0a1fb7f8c63d7f7a8b41de8f"
static void test_sign() {
  in3_t* c           = in3_for_chain(CHAIN_ID_MAINNET);
  c->chain_id        = 0x1;
  c->flags           = FLAGS_STATS;
  c->proof           = PROOF_NONE;
  c->signature_count = 0;
  register_transport(c, test_transport);

  for (int i = 0; i < c->chains_length; i++) {
    _free(c->chains[i].nodelist_upd8_params);
    c->chains[i].nodelist_upd8_params = NULL;
  }
  bytes32_t pk;
  hex_to_bytes("0x34a314920b2ffb438967bcf423112603134a0cdef0ad0bf7ceb447067eced303", -1, pk, 32);
  eth_set_pk_signer(c, pk);

  add_response("eth_sendRawTransaction", "[\"0xf8620182ffff8252089445d45e6ff99e6c34a235d263965910298985fcfe81ff8025a0a0973de4296ec3507fb718e2edcbd226504a9b01680e2c974212dc03cdd2ab4da016b3a55129723ebde5dca4f761c2b48d798ec7fb597ae7d8e3905f66fe03d93a\"]",
               "\"0x812510201f48a86df62f08e4e6366a63cbcfba509897edcc5605917bc2bf002f\"", NULL, NULL);
  add_response("eth_gasPrice", "[]", "\"0xffff\"", NULL, NULL);
  add_response("eth_getTransactionCount", "[\"0xb91bd1b8624d7a0a13f1f6ccb1ae3f254d3888ba\",\"latest\"]", "\"0x1\"", NULL, NULL);

  in3_ctx_t* ctx = in3_client_rpc_ctx(c, "eth_sendTransaction", "[{\"to\":\"0x45d45e6ff99e6c34a235d263965910298985fcfe\", \"value\":\"0xff\" }]");
  TEST_ASSERT_EQUAL(IN3_OK, ctx_check_response_error(ctx, 0));
  TEST_ASSERT_TRUE(ctx && ctx_get_error(ctx, 0) == IN3_OK);
  ctx_free(ctx);
  in3_free(c);
}

static void to_checksum_addr(uint8_t* address, chain_id_t chain, char* result) {
  in3_ret_t res = to_checksum(address, 0, result);
}

static void test_tx() {
  // create new incubed client
  in3_t* in3 = in3_for_chain(CHAIN_ID_MAINNET);
  in3_configure(in3, "{\"autoUpdateList\":false,\"nodes\":{\"0x1\": {\"needsUpdate\":false}}}");
  replace_transport(in3, test_transport);
  add_response("eth_sendRawTransaction", "[\"0xf892808609184e72a0008296c094d46e8dd67c5d32be8058bb8eb970870f07244567849184e72aa9d46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f07244567526a06f0103fccdcae0d6b265f8c38ee42f4a722c1cb36230fe8da40315acc30519a8a06252a68b26a5575f76a65ac08a7f684bc37b0c98d9e715d73ddce696b58f2c72\"]",
               "\"0x309f89063df0b28e40af95708edb72041d5715ed1e71701ed4ccb6433218088f\"", NULL, NULL);

  // convert the hexstring to bytes
  bytes32_t pk;
  hex_to_bytes(ETH_PRIVATE_KEY, -1, pk, 32);

  // create a simple signer with this key
  eth_set_pk_signer(in3, pk);

  address_t to, from;
  hex_to_bytes("0x63FaC9201494f0bd17B9892B9fae4d52fe3BD377", -1, from, 20);
  hex_to_bytes("0xd46e8dd67c5d32be8058bb8eb970870f07244567", -1, to, 20);

  bytes_t* data = hex_to_new_bytes("d46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675", 82);

  // send the tx
  bytes_t* tx_hash = eth_sendTransaction(in3, from, to, OPTIONAL_T_VALUE(uint64_t, 0x96c0), OPTIONAL_T_VALUE(uint64_t, 0x9184e72a000), OPTIONAL_T_VALUE(uint256_t, to_uint256(0x9184e72a)), OPTIONAL_T_VALUE(bytes_t, *data), OPTIONAL_T_VALUE(uint64_t, 0x0));
  // if the result is null there was an error and we can get the latest error message from eth_last_error()
  if (!tx_hash)
    printf("error sending the tx : %s\n", eth_last_error());
  else {
    printf("Transaction hash: ");
  }
  b_free(data);
  bytes_t* hash = hex_to_new_bytes("309f89063df0b28e40af95708edb72041d5715ed1e71701ed4ccb6433218088f", 64);

  TEST_ASSERT_TRUE(b_cmp(tx_hash, hash));
  b_free(tx_hash);
  b_free(hash);

  // cleanup client after usage
  in3_free(in3);
}

static void test_sign_hex() {

  in3_t* c = in3_for_chain(CHAIN_ID_MAINNET);
  c->proof = PROOF_NONE;
  c->flags = FLAGS_STATS;
  replace_transport(c, test_transport);

  for (int i = 0; i < c->chains_length; i++) {
    _free(c->chains[i].nodelist_upd8_params);
    c->chains[i].nodelist_upd8_params = NULL;
  }
  uint8_t* pk = eth_set_pk_signer_hex(c, "0x34a314920b2ffb438967bcf423112603134a0cdef0ad0bf7ceb447067eced303");

  add_response("eth_sendRawTransaction", "[\"0xf8620182ffff8252089445d45e6ff99e6c34a235d263965910298985fcfe81ff8025a0a0973de4296ec3507fb718e2edcbd226504a9b01680e2c974212dc03cdd2ab4da016b3a55129723ebde5dca4f761c2b48d798ec7fb597ae7d8e3905f66fe03d93a\"]",
               "\"0x812510201f48a86df62f08e4e6366a63cbcfba509897edcc5605917bc2bf002f\"", NULL, NULL);
  add_response("eth_gasPrice", "[]", "\"0xffff\"", NULL, NULL);
  add_response("eth_getTransactionCount", "[\"0xb91bd1b8624d7a0a13f1f6ccb1ae3f254d3888ba\",\"latest\"]", "\"0x1\"", NULL, NULL);

  in3_ctx_t* ctx = in3_client_rpc_ctx(c, "eth_sendTransaction", "[{\"to\":\"0x45d45e6ff99e6c34a235d263965910298985fcfe\", \"value\":\"0xff\" }]");
  TEST_ASSERT_EQUAL(IN3_OK, ctx_check_response_error(ctx, 0));
  TEST_ASSERT_TRUE(ctx && ctx_get_error(ctx, 0) == IN3_OK);
  free(pk);
  ctx_free(ctx);
  in3_free(c);
}

static void test_sign_sans_signer_and_from() {
  in3_t*     c   = in3_for_chain(CHAIN_ID_MAINNET);
  in3_ctx_t* ctx = in3_client_rpc_ctx(c, "eth_sendTransaction", "[{\"to\":\"0x45d45e6ff99e6c34a235d263965910298985fcfe\", \"value\":\"0xff\" }]");
  TEST_ASSERT_NOT_NULL(ctx->error);
  ctx_free(ctx);
  in3_free(c);
}

static void test_signer() {
  in3_t*    c = in3_for_chain(CHAIN_ID_MAINNET);
  bytes32_t pk;
  hex_to_bytes("0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8", -1, pk, 32);
  eth_set_pk_signer(c, pk);
  in3_ctx_t*     ctx      = ctx_new(c, "{\"method\":\"eth_getBlockByNumber\",\"params\":[\"latest\",false]}");
  char*          data_str = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
  bytes_t*       data     = hex_to_new_bytes(data_str, strlen(data_str));
  in3_sign_ctx_t sc;
  sc.ctx     = ctx;
  sc.message = *data;
  sc.type    = SIGN_EC_RAW;
  sc.account = bytes(NULL, 0);
  TEST_ASSERT_EQUAL(IN3_OK, in3_plugin_execute_first(ctx, PLGN_ACT_SIGN, &sc));
  TEST_ASSERT_FALSE(memiszero(sc.signature, 65));
  b_free(data);
  ctx_free(ctx);
  in3_free(c);
}

static in3_ret_t prep_tx(void* ctx, d_token_t* old_tx, json_ctx_t** new_tx) {
  if (d_get_int(old_tx, "success")) {
    *new_tx = parse_json("{\"from\": \"0xb60e8dd61c5d32be8058bb8eb970870f07233155\","
                         "\"to\": \"0xd46e8dd67c5d32be8058bb8eb970870f07244567\","
                         "\"gas\": \"0x76c0\","
                         "\"nonce\": \"0x15\","
                         "\"gasPrice\": \"0x9184e72a000\","
                         "\"value\": \"0x9184e72a\","
                         "\"data\": \"0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675\"}");
    return IN3_OK;
  }
  return IN3_EUNKNOWN;
}
/*
static void test_signer_prepare_tx() {
  // setup in3
  in3_t*    c = in3_for_chain(CHAIN_ID_MAINNET);
  bytes32_t pk;
  hex_to_bytes("0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8", -1, pk, 32);
  eth_set_pk_signer(c, pk);

  // prepare request
  in3_ctx_t* ctx        = ctx_new(c, "{\"method\":\"eth_getBlockByNumber\",\"params\":[\"latest\",false]}");
  c->signer->prepare_tx = prep_tx;
  json_ctx_t* jtx       = parse_json("{\"success\":false}");
  bytes_t     raw_tx    = sign_tx(jtx->result, ctx);
  TEST_ASSERT_FALSE(raw_tx.data && raw_tx.len);
  TEST_ASSERT_NOT_EQUAL(IN3_OK, ctx_get_error(ctx, 0));
  json_free(jtx);
  ctx_free(ctx);

  ctx    = ctx_new(c, "{\"method\":\"eth_getBlockByNumber\",\"params\":[\"latest\",false]}");
  jtx    = parse_json("{\"success\":true}");

  raw_tx = sign_tx(jtx->result, ctx);
  TEST_ASSERT_TRUE(ctx->type == CT_RPC && ctx->verification_state == IN3_WAITING && ctx->required);
  TEST_ASSERT_EQUAL(IN3_OK, in3_send_ctx(ctx->required));

  raw_tx = sign_tx(jtx->result, ctx);
  TEST_ASSERT_NOT_NULL(raw_tx.data);
  TEST_ASSERT_NOT_EQUAL(IN3_OK, ctx_get_error(ctx, 0));
  _free(raw_tx.data);
  ctx_free(ctx);
  json_free(jtx);
  in3_free(c);
}
*/
/*
 * Main
 */
int main() {
  in3_log_set_quiet(true);
  in3_log_set_level(LOG_ERROR);
  in3_register_default(in3_register_eth_full);
  TESTS_BEGIN();
  RUN_TEST(test_tx);
  RUN_TEST(test_sign);
  RUN_TEST(test_sign_hex);
  RUN_TEST(test_sign_sans_signer_and_from);
  RUN_TEST(test_signer);
  //  RUN_TEST(test_signer_prepare_tx);
  return TESTS_END();
}
