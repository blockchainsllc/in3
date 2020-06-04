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

#if defined(LEDGER_NANO)
#include "../../src/signer/ledger-nano/signer/ethereum_apdu_client.h"
#include "../../src/signer/ledger-nano/signer/ethereum_apdu_client_priv.h"
#endif

#include "../../src/signer/pk-signer/signer.h"
#include "../../src/third-party/crypto/ecdsa.h"
#include "../../src/third-party/crypto/secp256k1.h"
#include "../../src/verifier/eth1/basic/eth_basic.h"
#include "../test_utils.h"
#include "../util/transport.h"
#include <stdio.h>
#include <unistd.h>

#if defined(LEDGER_NANO)
static uint8_t bip_path[5] = {44, 60, 0, 0, 0};
#endif

static void test_tx() {
#if defined(LEDGER_NANO)
  // create new incubed client
  in3_t* in3 = in3_for_chain(ETH_CHAIN_ID_MAINNET);
  in3_configure(in3, "{\"autoUpdateList\":false,\"nodes\":{\"0x1\": {\"needsUpdate\":false}}}");
  in3->transport = test_transport;
  add_response("eth_sendRawTransaction", "[\"0xf86b808609184e72a0008296c094d46e8dd67c5d32be8058bb8eb970870f07244567849184e72a80820124a080f51ea2b39381d5c4f89b243649ef7b33878611b125145d96ac4459a6a089bba052fecd4b6151450f0887ccfa68120584cbf975414104d488c3e9dd71014a4a64\"]",
               "\"0x67681e9ca0e2a186f97ec63cc6c738005e6fa28c7d800558758245bf75b3c354\"", NULL, NULL);

  // create a ledger nano signer
  eth_ledger_set_signer_txn(in3, bip_path);

  address_t to, from;
  hex_to_bytes("0xC51fBbe0a68a7cA8d33f14a660126Da2A2FAF8bf", -1, from, 20);
  hex_to_bytes("0xd46e8dd67c5d32be8058bb8eb970870f07244567", -1, to, 20);

  bytes_t* data = hex_to_new_bytes("0x00", 0);

  // send the tx
  bytes_t* tx_hash = eth_sendTransaction(in3, from, to, OPTIONAL_T_VALUE(uint64_t, 0x96c0), OPTIONAL_T_VALUE(uint64_t, 0x9184e72a000), OPTIONAL_T_VALUE(uint256_t, to_uint256(0x9184e72a)), OPTIONAL_T_VALUE(bytes_t, *data), OPTIONAL_T_VALUE(uint64_t, 0x0));
  // if the result is null there was an error and we can get the latest error message from eth_last_error()
  if (!tx_hash)
    printf("error sending the tx : %s\n", eth_last_error());
  else {
    printf("Transaction hash: ");
  }
  b_free(data);
  bytes_t* hash = hex_to_new_bytes("67681e9ca0e2a186f97ec63cc6c738005e6fa28c7d800558758245bf75b3c354", 64);

  TEST_ASSERT_TRUE(b_cmp(tx_hash, hash));
  b_free(tx_hash);

  // cleanup client after usage
  in3_free(in3);
#endif
}

static void test_signer() {
#if defined(LEDGER_NANO)
  in3_t* c = in3_for_chain(ETH_CHAIN_ID_MAINNET);
  eth_ledger_set_signer_txn(c, bip_path);

  uint8_t    sig[65]  = {0};
  in3_ctx_t* ctx      = ctx_new(c, "{\"method\":\"eth_getBlockByNumber\",\"params\":[\"latest\",false]}");
  char*      data_str = "msgABCDEF"; // prefixing messages with msg to differentiate between transaction and message signing
  bytes_t*   data     = b_new((uint8_t*) data_str, strlen(data_str));

  TEST_ASSERT_EQUAL(65, eth_ledger_sign_txn(ctx, SIGN_EC_HASH, *data, bytes(NULL, 0), sig));
  TEST_ASSERT_FALSE(memiszero(sig, 65));
  b_free(data);
  in3_free(c);
#endif
}

/*
 * Main
 */
int main() {
  in3_log_set_quiet(true);
  in3_log_set_level(LOG_ERROR);
  in3_register_eth_full();
  TESTS_BEGIN();
#if defined(LEDGER_NANO)
  RUN_TEST(test_tx);
  RUN_TEST(test_signer);
#endif
  return TESTS_END();
}