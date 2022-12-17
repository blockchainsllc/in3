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

#include "../../include/in3/error.h"
#include "../../src/api/eth1/eth_api.h"
#include "../../src/core/client/keys.h"
#include "../../src/core/client/request.h"
#include "../../src/core/util/bytes.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/log.h"
#include "../../src/core/util/mem.h"
#include "../../src/core/util/scache.h"
#include "../../src/signer/pk-signer/rpcs.h"
#include "../../src/signer/pk-signer/signer.h"
#include "../../src/verifier/eth1/full/eth_full.h"
#include "../../src/verifier/eth1/nano/eth_nano.h"
#include "../../src/verifier/eth1/nano/rpcs.h"
#include "../test_utils.h"
#include "../util/transport.h"
#include <stdio.h>
#include <unistd.h>

#define ETH_PRIVATE_KEY "0x8da4ef21b864d2cc526dbdb2a120bd2874c36c9d0a1fb7f8c63d7f7a8b41de8f"
#define SEEDPHRASE      "dizzy nice link honey faith expect protect nasty manage athlete need brown boost rose tongue brass flash tip leaf release horse guide lens rigid"
#define SEEDPASSWORD    "test"
#define PATH            "m/44'/60'/0'/0/0"
#define HD_ADDR         "0x9307f84ece09d5904a9baf1071e1e1b6917555c9"
#define HD_ADDR2        "0xb5c48eb2a385564c6dfce87e0674ec2193fec0b0"
#define HD_PUB          "0x3bbf1bf437744ab20223a7611815f2130f19f65699fbaae67f40d4ad1a5bfd54c758d5b5889105770ea5ba4829f7d1cca2d96de256ede6e38432966a0a9d5668"

static void test_configure_hd() {
  in3_t* in3 = in3_for_chain(1); // new instance
  eth_register_pk_signer(in3);   // register pk-signer
  TEST_CONFIGURE(in3, "{\"hd\":{ \"seed_phrase\":\"" SEEDPHRASE "\" ,\"seed_password\":\"" SEEDPASSWORD "\",\"paths\":[\"" PATH "\"] }}")
  TEST_RPC(in3, FN_SIGNER_IDS, "[]", "[\"" HD_ADDR "\"]")

  uint8_t signer_id[20];
  uint8_t pub_key[64] = {0};

  hex_to_bytes(HD_ADDR, -1, signer_id, 20);
  hex_to_bytes(HD_PUB, -1, pub_key, 64);

  // dummy req
  in3_req_t req = {0};
  req.client    = in3;
  req.status    = IN3_WAITING;
  req.type      = RT_RPC;

  in3_sign_public_key_ctx_t pub_ctx = {
      .account      = signer_id,
      .req          = &req,
      .curve_type   = ECDSA_SECP256K1,
      .convert_type = CONV_PK32_TO_PUB64,
      .public_key   = 0};
  in3_plugin_execute_first(&req, PLGN_ACT_SIGN_PUBLICKEY, &pub_ctx);

  TEST_ASSERT_EACH_EQUAL_MEMORY_MESSAGE(pub_key, pub_ctx.public_key, 64, 1, "wrong pub key");

  // we derive the same as the existing address
  sign_derive_key_ctx_t derive_ctx = {
      .path      = PATH,
      .req       = &req,
      .seed_hash = NULL};

  in3_plugin_execute_first(&req, PLGN_ACT_SIGN_DERIVE, &derive_ctx);

  TEST_ASSERT_EACH_EQUAL_MEMORY_MESSAGE(signer_id, derive_ctx.account, 20, 1, "wrong derrived adr");

  // we derive a new address with different path
  derive_ctx.path = "m/44'/60'/0'/0/1";                                                                // new path
  hex_to_bytes(HD_ADDR2, -1, signer_id, 20);                                                           // this is the expected address
  in3_plugin_execute_first(&req, PLGN_ACT_SIGN_DERIVE, &derive_ctx);                                   // execute derive

  TEST_ASSERT_EACH_EQUAL_MEMORY_MESSAGE(signer_id, derive_ctx.account, 20, 1, "invalid derrived key"); // check key

  TEST_RPC(in3, FN_SIGNER_IDS, "[]", "[\"" HD_ADDR "\",\"" HD_ADDR2 "\"]")                             // make sure the new key is visible as signer_id

  in3_free(in3);
}
static void test_configure_mnemonic_string() {
  in3_t* in3 = in3_for_chain(1); // new instance
  eth_register_pk_signer(in3);   // register pk-signer
  eth_set_pk_signer_from_string(in3, SEEDPHRASE, PATH, SEEDPASSWORD);
  TEST_RPC(in3, FN_SIGNER_IDS, "[]", "[\"" HD_ADDR "\"]")

  uint8_t signer_id[20];
  uint8_t pub_key[64] = {0};

  hex_to_bytes(HD_ADDR, -1, signer_id, 20);
  hex_to_bytes(HD_PUB, -1, pub_key, 64);

  // dummy req
  in3_req_t req = {0};
  req.client    = in3;
  req.status    = IN3_WAITING;
  req.type      = RT_RPC;

  in3_sign_public_key_ctx_t pub_ctx = {
      .account      = signer_id,
      .req          = &req,
      .curve_type   = ECDSA_SECP256K1,
      .convert_type = CONV_PK32_TO_PUB64,
      .public_key   = 0};
  in3_plugin_execute_first(&req, PLGN_ACT_SIGN_PUBLICKEY, &pub_ctx);

  TEST_ASSERT_EACH_EQUAL_MEMORY_MESSAGE(pub_key, pub_ctx.public_key, 64, 1, "wrong pub key");

  in3_free(in3);
}

/*
 * Main
 */
int main() {

  TESTS_BEGIN();
  RUN_TEST(test_configure_hd);
  RUN_TEST(test_configure_mnemonic_string);
  return TESTS_END();
}
