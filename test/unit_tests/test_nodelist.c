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
#define DEBUG
#endif

#include "../../src/core/client/nodelist.h"
#include "../test_utils.h"

IN3_IMPORT_TEST bool in3_node_props_match(in3_node_props_t np_config, in3_node_props_t np);

static void test_capabilities(void) {
  in3_node_props_t npclient, npserver;

  in3_node_props_init(&npclient);
  in3_node_props_init(&npserver);
  TEST_ASSERT(in3_node_props_match(npclient, npserver));
  TEST_ASSERT_EQUAL(in3_node_props_get(npclient, NODE_PROP_MIN_BLOCK_HEIGHT), 0);
  TEST_ASSERT_FALSE(in3_node_props_get(npclient, NODE_PROP_PROOF));
  TEST_ASSERT_FALSE(in3_node_props_matches(npclient, NODE_PROP_HTTP));

  in3_node_props_init(&npclient);
  in3_node_props_set(&npclient, NODE_PROP_MIN_BLOCK_HEIGHT, 6);
  in3_node_props_init(&npserver);
  in3_node_props_set(&npserver, NODE_PROP_PROOF, true);
  in3_node_props_set(&npserver, NODE_PROP_MULTICHAIN, true);
  in3_node_props_set(&npserver, NODE_PROP_ARCHIVE, true);
  in3_node_props_set(&npserver, NODE_PROP_HTTP, true);
  in3_node_props_set(&npserver, NODE_PROP_BINARY, true);
  in3_node_props_set(&npserver, NODE_PROP_ONION, true);
  in3_node_props_set(&npserver, NODE_PROP_MIN_BLOCK_HEIGHT, 10);
  TEST_ASSERT(in3_node_props_match(npclient, npserver));

  in3_node_props_init(&npclient);
  in3_node_props_set(&npclient, NODE_PROP_PROOF, true);
  in3_node_props_set(&npclient, NODE_PROP_ARCHIVE, true);
  in3_node_props_set(&npclient, NODE_PROP_BINARY, true);
  in3_node_props_set(&npclient, NODE_PROP_SIGNER, true);
  in3_node_props_init(&npserver);
  in3_node_props_set(&npserver, NODE_PROP_PROOF, true);
  in3_node_props_set(&npserver, NODE_PROP_MULTICHAIN, true);
  in3_node_props_set(&npserver, NODE_PROP_ARCHIVE, true);
  in3_node_props_set(&npserver, NODE_PROP_HTTP, true);
  in3_node_props_set(&npserver, NODE_PROP_BINARY, true);
  in3_node_props_set(&npserver, NODE_PROP_ONION, true);
  in3_node_props_set(&npserver, NODE_PROP_SIGNER, true);
  in3_node_props_set(&npserver, NODE_PROP_DATA, true);
  in3_node_props_set(&npserver, NODE_PROP_STATS, true);
  TEST_ASSERT(in3_node_props_match(npclient, npserver));

  in3_node_props_init(&npclient);
  in3_node_props_init(&npserver);
  in3_node_props_set(&npserver, NODE_PROP_PROOF, true);
  in3_node_props_set(&npserver, NODE_PROP_MULTICHAIN, true);
  in3_node_props_set(&npserver, NODE_PROP_ARCHIVE, true);
  in3_node_props_set(&npserver, NODE_PROP_HTTP, true);
  in3_node_props_set(&npserver, NODE_PROP_BINARY, true);
  in3_node_props_set(&npserver, NODE_PROP_ONION, true);
  TEST_ASSERT(in3_node_props_match(npclient, npserver));

  in3_node_props_init(&npclient);
  in3_node_props_set(&npclient, NODE_PROP_PROOF, true);
  in3_node_props_init(&npserver);
  in3_node_props_set(&npserver, NODE_PROP_HTTP, true);
  TEST_ASSERT_FALSE(in3_node_props_match(npclient, npserver));

  in3_node_props_init(&npclient);
  in3_node_props_set(&npclient, NODE_PROP_MIN_BLOCK_HEIGHT, 6);
  in3_node_props_init(&npserver);
  in3_node_props_set(&npserver, NODE_PROP_MIN_BLOCK_HEIGHT, 5);
  TEST_ASSERT_FALSE(in3_node_props_match(npclient, npserver));

  in3_node_props_init(&npclient);
  in3_node_props_set(&npclient, NODE_PROP_PROOF, true);
  in3_node_props_set(&npclient, NODE_PROP_HTTP, true);
  in3_node_props_set(&npclient, NODE_PROP_BINARY, true);
  in3_node_props_set(&npclient, NODE_PROP_MIN_BLOCK_HEIGHT, 5);
  in3_node_props_init(&npserver);
  npserver = 1U | (1U << 3U) | (1U << 4U) | (5UL << 32U);
  TEST_ASSERT_EQUAL_MEMORY(&npclient, &npserver, sizeof(in3_node_props_t));

  in3_node_props_init(&npclient);
  in3_node_props_set(&npclient, NODE_PROP_MIN_BLOCK_HEIGHT, 5);
  TEST_ASSERT_EQUAL(in3_node_props_get(npclient, NODE_PROP_MIN_BLOCK_HEIGHT), 5);
  in3_node_props_set(&npclient, NODE_PROP_MIN_BLOCK_HEIGHT, 0);
  TEST_ASSERT_EQUAL(in3_node_props_get(npclient, NODE_PROP_MIN_BLOCK_HEIGHT), 0);
  in3_node_props_set(&npclient, NODE_PROP_MIN_BLOCK_HEIGHT, 255);
  TEST_ASSERT_EQUAL(in3_node_props_get(npclient, NODE_PROP_MIN_BLOCK_HEIGHT), 255);
}

/*
 * Main
 */
int main() {
  TESTS_BEGIN();
  RUN_TEST(test_capabilities);
  return TESTS_END();
}
