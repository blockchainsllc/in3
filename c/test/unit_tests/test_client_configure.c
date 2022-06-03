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
#endif
#ifndef TEST
#define DEBUG
#endif

#include "../../src/api/core/core_api.h"
#include "../../src/api/eth1/eth_api.h"
#include "../../src/core/client/plugin.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/debug.h"
#include "../../src/core/util/utils.h"
#include "../../src/nodeselect/full/cache.h"
#include "../../src/nodeselect/full/nodelist.h"
#include "../../src/nodeselect/full/nodeselect_def.h"
#include "../../src/tools/clientdata/client_data.h"
#include "../../src/verifier/eth1/nano/eth_nano.h"
#include "../test_utils.h"
#include <stdio.h>
#include <unistd.h>

static in3_t* create_sdk() {
  in3_t* c = in3_for_chain(CHAIN_ID_MAINNET);
  TEST_ASSERT_EQUAL(IN3_OK, in3_register_client_data(c, NULL));
  return c;
}

void test_get_config_default() {
  in3_t* c      = create_sdk();
  char * result = NULL, *error = NULL;
  error        = in3_configure(c, "{\"environment\":\"default\"}");
  char* config = in3_get_config(c);
  if (error) printf("ERROR: %s\n", error);
  TEST_ASSERT_NULL(error);
  TEST_ASSERT_EQUAL_STRING(config,
                           "{\"autoUpdateList\":true,\"chainId\":1,\"signatureCount\":0,\"finality\":0,\"includeCode\":false,\"bootWeights\":true,\"maxAttempts\":7,\"keepIn3\":false,\"stats\":true,\"useBinary\":false,\"useHttp\":false,\"experimental\":false,\"maxVerifiedHashes\":5,\"timeout\":10000,\"proof\":\"standard\"}");
  _free(config);
  in3_free(c);
}

void test_get_config_dev() {
  in3_t* c      = create_sdk();
  char * result = NULL, *error = NULL;
  error        = in3_configure(c, "{\"environment\":\"dev\"}");
  char* config = in3_get_config(c);
  if (error) printf("ERROR: %s\n", error);
  TEST_ASSERT_NULL(error);
  TEST_ASSERT_EQUAL_STRING(config,
                           "{\"autoUpdateList\":true,\"chainId\":1,\"signatureCount\":0,\"finality\":0,\"includeCode\":false,\"bootWeights\":true,\"maxAttempts\":7,\"keepIn3\":false,\"stats\":true,\"useBinary\":false,\"useHttp\":false,\"experimental\":false,\"maxVerifiedHashes\":5,\"timeout\":10000,\"proof\":\"standard\"}");
  _free(config);
  in3_free(c);
}

void test_get_config_test() {
  in3_t* c      = create_sdk();
  char * result = NULL, *error = NULL;
  error        = in3_configure(c, "{\"environment\":\"test\"}");
  char* config = in3_get_config(c);
  if (error) printf("ERROR: %s\n", error);
  TEST_ASSERT_NULL(error);
  TEST_ASSERT_EQUAL_STRING(config,
                           "{\"autoUpdateList\":true,\"chainId\":1,\"signatureCount\":0,\"finality\":0,\"includeCode\":false,\"bootWeights\":true,\"maxAttempts\":7,\"keepIn3\":false,\"stats\":true,\"useBinary\":false,\"useHttp\":false,\"experimental\":false,\"maxVerifiedHashes\":5,\"timeout\":10000,\"proof\":\"standard\"}");
  _free(config);
  in3_free(c);
}

void test_get_config_prod() {
  in3_t* c      = create_sdk();
  char * result = NULL, *error = NULL;
  error        = in3_configure(c, "{\"environment\":\"prod\"}");
  char* config = in3_get_config(c);
  if (error) printf("ERROR: %s\n", error);
  TEST_ASSERT_NULL(error);
  TEST_ASSERT_EQUAL_STRING(config,
                           "{\"autoUpdateList\":true,\"chainId\":1,\"signatureCount\":0,\"finality\":0,\"includeCode\":false,\"bootWeights\":true,\"maxAttempts\":7,\"keepIn3\":false,\"stats\":true,\"useBinary\":false,\"useHttp\":false,\"experimental\":false,\"maxVerifiedHashes\":5,\"timeout\":10000,\"proof\":\"standard\"}");
  _free(config);
  in3_free(c);
}

/*
 * Main
 */
int main() {
  dbg_log("Start test: IN3 configure environrments");
  TESTS_BEGIN();
  RUN_TEST(test_get_config_default);
  RUN_TEST(test_get_config_dev);
  RUN_TEST(test_get_config_test);
  RUN_TEST(test_get_config_prod);
  return TESTS_END();
}