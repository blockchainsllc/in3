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

#include "../../src/api/eth1/abi.h"
#include "../../src/core/client/keys.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/log.h"
#include "../../src/core/util/mem.h"
#include "../test/testdata/mock/eth_call_response_binary.h"
#include "../test_utils.h"
#include "../util/transport.h"
#include <stdio.h>
#include <unistd.h>

d_key_t key_(const char* c) {
  uint16_t val = 0;
  size_t   l   = strlen(c);
  for (; l; l--, c++) val ^= *c | val << 7;
  return val;
}

const char* account_proof_array[] = {
    "f90211a07a5f1f32b8c5fd400ae9c05f4c9cad078d2441bb2805213f68e3eedee2ab5428a0b9fe2fa942d62c903cc400af871638bf72c8d8c7edb9d7cf05a6357397571d6fa0472a3b5d3d5fa2efda7d1881e9fc2055114d0ea44fc9b8f321634e9550e8481da06423169bc800c19afe68b3cf1d35c4fea27673b9bf9b86b543389e1e17faef17a0bae5224883a0eeb64cf9388366f3999e056319615e391031aef3216d0484d369a0b9c4bc799589e49d31531afccae07d034525ec6adf11917b2a1d5be17cb81642a004830179158177c0171b5fc4975fbaacb7017d04f711a91fe9bde27ccebc84e7a0fd80007fbc36c9668a242740f87103f303a2a5d5d41ff96a1196f4818d19b601a08e2c476e4f0a338a4863e882a6f00ba17be3774c82de8db85b2ecb6551504ab9a03f8e19cd9ccaf0c7b05326b5aab0092844b8749af91741fc0b807069d1240356a0230b2728d940b628f1a8d14c26b49f0774b492a9018144560e3515a011473695a03b23a72d7adbff9a30fa65be4197631a80e24030f973b5e60389097f79bbc402a0f13d82ffb37d2723c6dc22e8677f986785e3845a34d9110ebe4e944807dd29cea042901a4da75d52378adcb15e69403ee6f8590353b4bfeddde4880f31fdced892a0bd587f5d7caf355ed61f0f77e56891064e1f89c7dcf370cfe96be3278ec34f4aa08e3b1e2d6a85020e9c3b101a080f3569bb3f66b4081e354f004fa80a35e60cf880",
    "f90211a0f322df9d41189d1c3de89ce4ccca4b46f2a67d564035d76036019d328a423e2fa06e0ed9268c8a684f09041b7ca475c887042dab7b0532f5e9de1f9fc712b87098a0174c761ee16622901df2083c4c21a0c9530b6924a4b8c9e229f4ef650e07b168a029daec36a3b36d75fcf1b08eb0b6f9169349d49b0525f366a1daa277ce6f0f5fa063b5cf3772f47a2f28a1f874f23d9f353d2cf7e670a52ae1e2d62cf25b962544a0cd2bf9fa4a0c70dafc224ea2dc11b28a83801a140f96c9ed108d6d831a14b3f4a0e8b7fadf492331cd48e4f598b4d3415f49a58269a29f54f2634585f7c00dfceca011aff132137d5e16db6dee568bb8484e2e8d7a99c985efd9a8db45cfe8447201a01d7a4742b95b4aff8fd19f34ea85212c8746a51d27b19db5ee18064a4ae0e75ca0e8e81d3c5d40d7b5f14cd7287dc26b5526694511c65a5409deed1809e4403bf4a05904f849941fbf96e3575fd668c9d384d67086fad05b675efb7cbcea9239fb7ea03c9bef4b03ea6aa1e7e5e24a19bd92fd0ae5368599922e55ef0996998a51b933a09c9d86e02ae33f733154cb10507e8f32efa9f2c3d4b31784dfd627ff330a68d6a0171afbb13a828dab8cd42f6d8257bd2c52040d09ab79e9cd12e2b29aabf0736da06ef40f704235c455e06f0658e994edba6d3fa98d3997c37d66cd92cd58e7ba80a096361d0cfcc4cda8d90471f3c3c4ad32a04890158660ddcc9f8ebc5af397c53980",
    "f90211a0e857fc28938ccaace8c403128bf9848fef722f25f8d55c9dfcddd980b0b85c7ea0329d29fce8ca4e4807205751e9f4376926e3d56dc3bb639345e410dc20bc2c7ba0075edfc7246e4719a2c09c397c4cea41d448254820f1974a70841b9d58e3bcdda08f48b35a917b095ed73971faf013879b77a2838de736651b48a6aaa10ab15e8fa0c5983fe9441f72cc6cb8b579f7071224280661a45a14408401bb1f759dbf0128a03b8cbc5ed56781199fb4fb9768c2ddcc2bdf00816097ec10291b30ac8964bd76a0d3a8e19461710aee36c32068722261863b86c7eacb50188e8f30deaffae024d2a0f1b377bcc8de17636d7f51dbb5377fdc8b9b98da3ea32edbafd10ff557e65f67a01756aebd26a37a6dfa2ebab54206ec14a4ed1c0771316a93b2302d029dc9fb77a016cc5db286c0a3694dd0a5dfbf17ae7e6e71ee86345f5b447c47703d216e8a49a0985de3d814a4a420fb4aa29cea550447ff26700e6f8201979853e2ab58ffb881a05c37520f1f9b351efb77546b89ba562cab7e3ff846f660d2924a327149fda54aa007b0f783eeecba8d2e1b19cd27bc32962d21c78589fb2469918c27fa1fb9f3f5a07aadde6f5a16f1bc14deae2cfdc59a3b12fa4334c8ab3ca938a0ad1f165171d7a08553712a5b419ad8742e02c1b2826f9d6f226ec29205f317460e3543c4432cfaa0989cbf17578dc31a7048bb6f7951bbcb4390a4dc34548749ddec5808ff70179080",
    "f901d1a0e58784367fe398679d1e6bf78a3dde3fa10c258a65eea417848eaa9c1365c52880a02c563764c450274db1cb7ed88cb682e357593263c87f03702e61f27038e6e922a07922b0a621b87690b1d9c3ac5d65453e8986f7119310aaa2382ce97790c970f6a03b791f9b5ea7b53ccc84bab05c1ab7fc182e5bd509489443e243a52834f12c4da02f3649d1bacefd45f4b86a92519e11fefde52bf9c033de305ab01e52da1e48e1a0a5b9e0f7516da5fc7363da0173758cf09a9d5e690871de79cb679c4fc58461a3a01b364c944363705f8ece380cff7c2e97f02ec60bf1dfab7fb05416650e521e07a0d4fc2dbc2c9cda95844bd354e7b15d597129986c08d647936566cdc6e6706703a0c0e749ab6f93f5e2d8c3ddce4069b14209bc1af57ae92495a2aab329c05cec4680a02ce6842baa2cf1f1ae3ca83d14671d0542a68d56ee6e5988fcf3bebe350871fba08b653c5eec87175195fbc9f2d4ea4812f247e8750c82a898da213754dd432740a067fd774407561e7a0a58dab51f060e6aae9f328b5b0da61eabb9c3fbdc7d0885a0fd684711826b61ca2db1c759aa1fa7358076077d6b39eebb634372acaf24d513a0f6a420560467e2f0373f048d29563653021b07d9e6bec69b0b441a2aba5c184680",
    "f8718080a05962e5c16b55955a1b3e61772a6e724d3a1781d01560a451af4863fd8e9fa93a808080a01f98d45e3935f95ab00d8719a0fc6622c7b94f222ac5a8646cddda41c9d2a05f80808080a0fa04c9c955cb58796e3c31163dfc2a7eabe844afa89b2fa56e41e374498b3d788080808080",
    "f8679e3f79d03bccd3d5515f1e14b1abc5eefe027e5a7252bcfd9f428823d3593db846f8440180a0821e2556a290c86405f8160a2d662042a431ba456b9db265c79bb837c04be5f0a08bfc1c7c7464660a68ba241ea78b6a1d36cd85b5cb4fba521d4270bd0c7d7bb7"};

static json_ctx_t* init() {
  bytes_t b = {.data = (uint8_t*) mock_eth_call_response, .len = mock_eth_call_response_len};
  //print_debug(&b);
  return parse_binary(&b);
}

static void test_binary_primitives() {
  json_ctx_t* ctx        = init();
  int         k_result   = key_("result");
  int         k_id       = key_("id");
  int         k_jsonrpc  = key_("jsonrpc");
  bytes_t*    d          = d_bytes(d_get(ctx->result, k_result));
  char*       str_result = malloc(d->len * 2 + 2);
  bytes_to_hex(d->data, d->len, str_result);
  char*   jsonrpc = d_get_stringk(ctx->result, k_jsonrpc);
  int32_t id_     = d_get_intk(ctx->result, k_id);
  TEST_ASSERT_EQUAL_STRING(str_result, "0000000000000000000000000000000000000000000000000000000000000001");
  TEST_ASSERT_EQUAL_STRING(jsonrpc, "2.0");
  TEST_ASSERT_EQUAL_INT32(id_, 1);
  free(str_result);
}

static void test_binary_object() {
  json_ctx_t* ctx            = init();
  int         k_in3          = key_("in3");
  int         k_accounts     = key_("accounts");
  int         k_storageProof = key_("storageProof");
  int         k_key          = key_("key");
  int         k_proof        = key_("proof");
  int         k_value        = key_("value");
  //test object
  d_token_t* accounts  = d_get(d_get(d_get(ctx->result, k_in3), k_proof), k_accounts);
  char*      str_proof = NULL;
  for (d_iterator_t iter = d_iter(accounts); iter.left; d_iter_next(&iter)) {
    d_token_t* storage_object = d_get_at(d_get(iter.token, k_storageProof), 0);
    bytes_t*   proof_s        = d_get_bytes_at(d_get(storage_object, k_proof), 0);
    int32_t    key            = d_int(d_get(storage_object, k_key));
    int32_t    value          = d_int(d_get(storage_object, k_value));
    str_proof                 = malloc(proof_s->len * 2 + 10);
    bytes_to_hex(proof_s->data, proof_s->len, str_proof);
    TEST_ASSERT_EQUAL_STRING(str_proof, "e3a120290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e56301");
    TEST_ASSERT_EQUAL_INT32(key, 0);
    TEST_ASSERT_EQUAL_INT32(value, 1);
  }
  free(str_proof);
}
static void test_binary_array() {
  json_ctx_t* ctx            = init();
  int         k_in3          = key_("in3");
  int         k_accounts     = key_("accounts");
  int         k_accountProof = key_("accountProof");
  int         k_proof        = key_("proof");
  // test byte array
  d_token_t* accounts = d_get(d_get(d_get(ctx->result, k_in3), k_proof), k_accounts);
  for (d_iterator_t iter = d_iter(accounts); iter.left; d_iter_next(&iter)) {
    d_token_t* accounts_array = d_get(iter.token, k_accountProof);
    int        index          = 0;
    char*      str_proof      = NULL;
    for (d_iterator_t iter_arr = d_iter(accounts_array); iter_arr.left; d_iter_next(&iter_arr)) {
      bytes_t* proof_data = d_bytes(iter_arr.token);
      str_proof           = malloc(proof_data->len * 2 + 10);
      bytes_to_hex(proof_data->data, proof_data->len, str_proof);
      TEST_ASSERT_EQUAL_STRING(str_proof, account_proof_array[index]);
      index++;
      free(str_proof);
    }
  }
}

/*
 * Main
 */
int main() {
  // now run tests
  TESTS_BEGIN();
  RUN_TEST(test_binary_primitives);
  RUN_TEST(test_binary_array);
  RUN_TEST(test_binary_object);
  return TESTS_END();
}
