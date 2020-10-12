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
#define RECORDER
#endif

#include "../../src/api/eth1/eth_api.h"
#include "../../src/core/client/plugin.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/debug.h"
#include "../../src/core/util/log.h"
#include "../../src/tools/recorder/recorder.h"
#include "../../src/verifier/eth1/full/eth_full.h"
#include "../test_utils.h"
#include "../util/transport.h"
#include <nodeselect/nodeselect_def.h>
#include <unistd.h>

in3_t* init_in3(in3_plugin_act_fn custom_transport, chain_id_t chain) {
  in3_t* in3 = NULL;
  in3        = in3_for_chain(CHAIN_ID_MAINNET);
  if (custom_transport)
    register_transport(in3, custom_transport);
  in3_register_nodeselect_def(in3);
  TEST_ASSERT_NULL(in3_configure(in3, "{\"autoUpdateList\":false,\"requestCount\":1,\"maxAttempts\":1,\"nodes\":{\"0x1\": {\"needsUpdate\":false}}}"));
  in3->flags = FLAGS_STATS | FLAGS_INCLUDE_CODE; // no autoupdate nodelist
  return in3;
}

static void test_recorder() {
  in3_t* in3 = init_in3(mock_transport, 0x5);
  in3_record(in3, "test_record", false);
  uint64_t blknum = eth_blockNumber(in3);
  TEST_ASSERT_TRUE(blknum > 0);
  in3_free(in3);
}

/*
 * Main
 */
int main() {
  dbg_log("record test\n");
  in3_log_set_quiet(true);
  in3_log_set_level(LOG_ERROR);
  in3_register_default(in3_register_eth_full);
  TESTS_BEGIN();
  RUN_TEST(test_recorder);
  return TESTS_END();
}
