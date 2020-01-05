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

#include "../../src/core/client/cache.h"
#include "../../src/core/client/context.h"
#include "../../src/core/client/nodelist.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/log.h"
#include "../../src/verifier/eth1/basic/eth_basic.h"
#include "../../src/verifier/eth1/basic/signer-priv.h"
#include "../../src/verifier/eth1/basic/signer.h"
#include "../test_utils.h"
#include "../util/transport.h"
#include <stdio.h>
#include <unistd.h>

static void test_sign() {

  in3_register_eth_basic();

  in3_t* c = in3_for_chain(ETH_CHAIN_ID_MAINNET);
  ;
  c->transport        = test_transport;
  c->chain_id         = 0x1;
  c->auto_update_list = false;
  c->proof            = PROOF_NONE;
  c->signature_count  = 0;

  for (int i = 0; i < c->chains_length; i++) c->chains[i].needs_update = false;

  bytes32_t pk;
  hex_to_bytes("0x34a314920b2ffb438967bcf423112603134a0cdef0ad0bf7ceb447067eced303", -1, pk, 32);
  eth_set_pk_signer(c, pk);
  in3_set_default_signer(c->signer);

  add_response("eth_getTransactionCount", "[\"0xb91bd1b8624d7a0a13f1f6ccb1ae3f254d3888ba\",\"latest\"]", "\"0x1\"", NULL, NULL);
  add_response("eth_gasPrice", "[]", "\"0xffff\"", NULL, NULL);
  add_response("eth_sendRawTransaction", "[\"0xf8620182ffff8252089445d45e6ff99e6c34a235d263965910298985fcfe81ff8025a0a0973de4296ec3507fb718e2edcbd226504a9b01680e2c974212dc03cdd2ab4da016b3a55129723ebde5dca4f761c2b48d798ec7fb597ae7d8e3905f66fe03d93a\"]",
               "\"0x812510201f48a86df62f08e4e6366a63cbcfba509897edcc5605917bc2bf002f\"", NULL, NULL);

  in3_ctx_t* ctx = in3_client_rpc_ctx(c, "eth_sendTransaction", "[{\"to\":\"0x45d45e6ff99e6c34a235d263965910298985fcfe\", \"value\":\"0xff\" }]");
  TEST_ASSERT_EQUAL(IN3_OK, ctx_check_response_error(ctx, 0));
  TEST_ASSERT_TRUE(ctx && ctx_get_error(ctx, 0) == IN3_OK);
  ctx_free(ctx);
}

static void test_signer() {
  in3_t*    c = in3_for_chain(ETH_CHAIN_ID_MAINNET);
  bytes32_t pk;
  hex_to_bytes("0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8", -1, pk, 32);
  eth_set_pk_signer(c, pk);
  uint8_t    sig[65]  = {0};
  in3_ctx_t* ctx      = ctx_new(c, "{\"method\":\"eth_getBlockByNumber\",\"params\":[\"latest\",false]}");
  char*      data_str = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
  bytes_t*   data     = hex_to_new_bytes(data_str, strlen(data_str));
  TEST_ASSERT_EQUAL(65, eth_sign(ctx, SIGN_EC_RAW, *data, bytes(NULL, 0), sig));
  TEST_ASSERT_FALSE(memiszero(sig, 65));
  sig[64] += 27;

  b_free(data);
  in3_free(c);
}
  in3_free(c);
}
/*
 * Main
 */
int main() {
  in3_log_set_level(LOG_ERROR);
  TESTS_BEGIN();
  RUN_TEST(test_sign);
  RUN_TEST(test_signer);
  return TESTS_END();
}
