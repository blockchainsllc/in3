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
#include "../../src/core/client/nodelist.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/debug.h"
#include "../../src/core/util/utils.h"
#include "../../src/verifier/eth1/nano/eth_nano.h"
#include "../test_utils.h"
#include <stdio.h>
#include <unistd.h>

void test_get_config() {
  in3_register_default(in3_register_eth_nano);
  in3_register_default(in3_register_eth_api);
  in3_t* c      = in3_for_chain(CHAIN_ID_KOVAN);
  char * result = NULL, *error = NULL;
  in3_client_rpc(c, "in3_getConfig", "[]", &result, &error);
  if (error) printf("ERROR: %s\n", error);
  TEST_ASSERT_NULL(error);
  TEST_ASSERT_EQUAL_STRING("{\"autoUpdateList\":true,\"chainId\":42,\"signatureCount\":0,\"finality\":0,\"includeCode\":false,\"bootWeights\":true,\"maxAttempts\":7,\"keepIn3\":false,\"stats\":true,\"useBinary\":false,\"useHttp\":false,\"maxBlockCache\":0,\"maxCodeCache\":0,\"maxVerifiedHashes\":5,\"timeout\":10000,\"minDeposit\":0,\"nodeProps\":0,\"nodeLimit\":0,\"proof\":\"standard\",\"requestCount\":1,\"nodes\":{\"0x2a\":{\"contract\":\"0x4c396dcf50ac396e5fdea18163251699b5fcca25\",\"registryId\":\"0x92eb6ad5ed9068a24c1c85276cd7eb11eda1e8c50b17fbaffaf3e8396df4becf\",\"needsUpdate\":true,\"avgBlockTime\":6}}}", result);
  _free(result);
  in3_free(c);
}

/*
 * Main
 */
int main() {
  dbg_log("starting cor tests");

  TESTS_BEGIN();
  RUN_TEST(test_get_config);
  return TESTS_END();
}
