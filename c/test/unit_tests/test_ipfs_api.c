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

#include "../../src/api/ipfs/ipfs_api.h"
#include "../../src/verifier/ipfs/ipfs.h"
#include "../test_utils.h"
#include "../util/transport.h"
#include <core/util/log.h>

#define LOREM_IPSUM "Lorem ipsum dolor sit amet"
#define LOREM_IPSUM_LONG "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation " \
                         "ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint " \
                         "occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."
#define LOREM_IPSUM_LONG_B64 "TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGNvbnNlY3RldHVyIGFkaXBpc2NpbmcgZWxpdCwgc2VkIGRvIGVpdXNtb2QgdGVtcG9yIGluY2lkaWR1bnQgdXQgbGFib3JlIGV0IGRvbG9yZSBtYWduYSBhbGlxdWEuIFV0IGV" \
                             "uaW0gYWQgbWluaW0gdmVuaWFtLCBxdWlzIG5vc3RydWQgZXhlcmNpdGF0aW9uIHVsbGFtY28gbGFib3JpcyBuaXNpIHV0IGFsaXF1aXAgZXggZWEgY29tbW9kbyBjb25zZXF1YXQuIER1aXMgYXV0ZSBpcnVyZSBkb2xvciBpbi" \
                             "ByZXByZWhlbmRlcml0IGluIHZvbHVwdGF0ZSB2ZWxpdCBlc3NlIGNpbGx1bSBkb2xvcmUgZXUgZnVnaWF0IG51bGxhIHBhcmlhdHVyLiBFeGNlcHRldXIgc2ludCBvY2NhZWNhdCBjdXBpZGF0YXQgbm9uIHByb2lkZW50LCBzd" \
                             "W50IGluIGN1bHBhIHF1aSBvZmZpY2lhIGRlc2VydW50IG1vbGxpdCBhbmltIGlkIGVzdCBsYWJvcnVtLg=="

static in3_t* in3_init_test(chain_id_t chain) {
  in3_t* in3     = in3_for_chain(chain);
  in3->transport = test_transport;
  in3_configure(in3, "{\"autoUpdateList\":false,\"nodes\":{\"0x7d0\": {\"needsUpdate\":false}}}");
  return in3;
}

void test_in3_ipfs_api() {
  in3_t*  in3 = in3_init_test(ETH_CHAIN_ID_IPFS);
  bytes_t b   = {.data = (uint8_t*) LOREM_IPSUM, .len = strlen(LOREM_IPSUM)};

  add_response("ipfs_put", "[\"TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQ=\",\"base64\"]", "\"QmbGySCLuGxu2GxVLYWeqJW9XeyjGFvpoZAhGhXDGEUQu8\"", NULL, NULL);
  char* multihash = ipfs_put(in3, &b);
  TEST_ASSERT_EQUAL_STRING("QmbGySCLuGxu2GxVLYWeqJW9XeyjGFvpoZAhGhXDGEUQu8", multihash);

  add_response("ipfs_get", "[\"QmbGySCLuGxu2GxVLYWeqJW9XeyjGFvpoZAhGhXDGEUQu8\",\"base64\"]", "\"TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQ=\"", NULL, NULL);
  bytes_t* content = ipfs_get(in3, multihash);

  TEST_ASSERT_EQUAL_MEMORY(LOREM_IPSUM, content->data, content->len);

  free(multihash);
  b_free(content);
  in3_free(in3);
}

void test_in3_ipfs_api_long() {
  in3_t*  in3 = in3_init_test(ETH_CHAIN_ID_IPFS);
  bytes_t b   = {.data = (uint8_t*) LOREM_IPSUM_LONG, .len = strlen(LOREM_IPSUM_LONG)};

  add_response("ipfs_put", "[\"" LOREM_IPSUM_LONG_B64 "\",\"base64\"]", "\"QmdefnkMuNaAro8sT2i5rufFbJmfd621UFsaoTaaL1Goh9\"", NULL, NULL);
  char* multihash = ipfs_put(in3, &b);
  TEST_ASSERT_EQUAL_STRING("QmdefnkMuNaAro8sT2i5rufFbJmfd621UFsaoTaaL1Goh9", multihash);

  add_response("ipfs_get", "[\"QmdefnkMuNaAro8sT2i5rufFbJmfd621UFsaoTaaL1Goh9\",\"base64\"]", "\"" LOREM_IPSUM_LONG_B64 "\"", NULL, NULL);
  bytes_t* content = ipfs_get(in3, multihash);

  TEST_ASSERT_EQUAL_MEMORY(LOREM_IPSUM_LONG, content->data, content->len);

  free(multihash);
  b_free(content);
  in3_free(in3);
}

/*
 * Main
 */
int main() {
  in3_register_ipfs();
  in3_log_set_quiet(true);

  TESTS_BEGIN();
  RUN_TEST(test_in3_ipfs_api);
  RUN_TEST(test_in3_ipfs_api_long);
  return TESTS_END();
}
