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

#include "../../src/core/util/bitset.h"
#include "../test_utils.h"

static void bs_print(bitset_t* bs) {
  fprintf(stderr, "> [%s]: ", bs->len > BS_MAX ? "heap" : "stack");
  for (size_t i = bs->len; i != 0; --i)
    fprintf(stderr, "%x", bs_isset(bs, i - 1));
  fprintf(stderr, "\n");
}

static void test_bitset_op() {
  bitset_t* bs = bs_new(30);
  bs_set(bs, 16);
  bs_toggle(bs, 16);
  bs_set(bs, 14);
  bs_clear(bs, 14);
  bs_toggle(bs, 28);
  bs_clear(bs, 28);
  TEST_ASSERT_TRUE(bs_isempty(bs));
  bs_free(bs);
  bs = bs_new(130);
  bs_set(bs, 116);
  bs_toggle(bs, 116);
  bs_set(bs, 114);
  bs_clear(bs, 114);
  bs_toggle(bs, 128);
  bs_clear(bs, 128);
  TEST_ASSERT_TRUE(bs_isempty(bs));
  bs_free(bs);
}

static void test_bitset_transition() {
  bitset_t* bs = bs_new(20);
  bs_set(bs, 19);
  bs_set(bs, 11);
  bs_set(bs, 121);
  TEST_ASSERT_TRUE(bs_isset(bs, 19));
  TEST_ASSERT_TRUE(bs_isset(bs, 11));
  TEST_ASSERT_TRUE(bs_isset(bs, 121));
  bs_clear(bs, 19);
  bs_clear(bs, 11);
  bs_clear(bs, 121);
  TEST_ASSERT_TRUE(bs_isempty(bs));
  bs_free(bs);
}

static void test_bitset_realloc() {
  bitset_t* bs = bs_new(8);
  bs_set(bs, 16);
  bs_set(bs, 32);
  bs_set(bs, 64);
  bs_set(bs, 128);
  bs_set(bs, 256);
  TEST_ASSERT_TRUE(bs_isset(bs, 16));
  TEST_ASSERT_TRUE(bs_isset(bs, 32));
  TEST_ASSERT_TRUE(bs_isset(bs, 64));
  TEST_ASSERT_TRUE(bs_isset(bs, 128));
  TEST_ASSERT_TRUE(bs_isset(bs, 256));
  bs_free(bs);
}

static void test_bitset_edge_cases() {
  bitset_t* bs = bs_new(64);
  bs_set(bs, 63);
  bs_set(bs, 64);
  bs_set(bs, 65);
  TEST_ASSERT_TRUE(bs_isset(bs, 63));
  TEST_ASSERT_TRUE(bs_isset(bs, 64));
  TEST_ASSERT_TRUE(bs_isset(bs, 65));
  bs_clear(bs, 63);
  bs_clear(bs, 64);
  bs_clear(bs, 65);
  TEST_ASSERT_TRUE(bs_isempty(bs));
  bs_free(bs);
  bs = bs_new(128);
  bs_set(bs, 127);
  bs_set(bs, 126);
  bs_set(bs, 125);
  bs_set(bs, 128);
  bs_set(bs, 129);
  TEST_ASSERT_TRUE(bs_isset(bs, 125));
  TEST_ASSERT_TRUE(bs_isset(bs, 126));
  TEST_ASSERT_TRUE(bs_isset(bs, 127));
  TEST_ASSERT_TRUE(bs_isset(bs, 128));
  TEST_ASSERT_TRUE(bs_isset(bs, 129));
  bs_clear(bs, 127);
  bs_clear(bs, 126);
  bs_clear(bs, 125);
  bs_clear(bs, 128);
  bs_clear(bs, 129);
  TEST_ASSERT_TRUE(bs_isempty(bs));
  bs_free(bs);
}

static void test_bitset_clone() {
  bitset_t* bs1 = bs_new(11);
  bs_set(bs1, 0);
  bs_set(bs1, 2);
  bs_set(bs1, 4);
  bitset_t* bs2 = bs_clone(bs1);
  TEST_ASSERT_TRUE(bs_isset(bs2, 0));
  TEST_ASSERT_TRUE(bs_isset(bs2, 2));
  TEST_ASSERT_TRUE(bs_isset(bs2, 4));
  bs_clear(bs2, 0);
  bs_clear(bs2, 2);
  bs_clear(bs2, 4);
  TEST_ASSERT_TRUE(bs_isempty(bs2));
  bs_free(bs1);
  bs_free(bs2);
  bs1 = bs_new(130);
  bs_set(bs1, 120);
  bs_set(bs1, 122);
  bs_set(bs1, 124);
  bs2 = bs_clone(bs1);
  TEST_ASSERT_TRUE(bs_isset(bs2, 120));
  TEST_ASSERT_TRUE(bs_isset(bs2, 122));
  TEST_ASSERT_TRUE(bs_isset(bs2, 124));
  bs_clear(bs2, 120);
  bs_clear(bs2, 122);
  bs_clear(bs2, 124);
  TEST_ASSERT_TRUE(bs_isempty(bs2));
  bs_free(bs1);
  bs_free(bs2);
}

static void test_bitset_out_of_range() {
  bitset_t* bs = bs_new(30);
  TEST_ASSERT_FALSE(bs_isset(bs, 64));
  TEST_ASSERT_FALSE(bs_isset(bs, 128));
  bs_free(bs);
  bs = bs_new(120);
  TEST_ASSERT_FALSE(bs_isset(bs, 128));
  TEST_ASSERT_FALSE(bs_isset(bs, 256));
  bs_free(bs);
}

/*
 * Main
 */
int main() {
  TESTS_BEGIN();
  RUN_TEST(test_bitset_op);
  RUN_TEST(test_bitset_transition);
  RUN_TEST(test_bitset_realloc);
  RUN_TEST(test_bitset_edge_cases);
  RUN_TEST(test_bitset_clone);
  RUN_TEST(test_bitset_out_of_range);
  return TESTS_END();
}
