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

void test_c_to_long() {
  TEST_ASSERT_EQUAL(0, char_to_long("0x", 2));
  TEST_ASSERT_EQUAL(2, char_to_long("0x2", 3));
  TEST_ASSERT_EQUAL(2, char_to_long("0x02", 4));
  TEST_ASSERT_EQUAL(65535, char_to_long("0xFFff", 6));
  TEST_ASSERT_EQUAL(0, char_to_long("xx", 2));
  TEST_ASSERT_EQUAL(10, char_to_long("10", 2));
}

void test_bytes() {
  bytes32_t data;
  memset(data, 0, 32);
  data[1] = 0x12;
  ba_print(data, 32);
  bytes_t b = bytes(data, 32);
  b_print(&b);

  bytes_t clone = cloned_bytes(b);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(data, clone.data, 32);
  // make sure it does not crash
  b_free(NULL);
}
void test_debug() {
  msg_dump("test", (void*) "abc", 3);
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_ECONFIG));
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_EUNKNOWN));
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_ENOMEM));
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_ENOTSUP));
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_EINVAL));
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_EFIND));
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_ELIMIT));
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_EVERS));
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_EINVALDT));
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_EPASS));
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_ERPC));
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_ERPCNRES));
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_EUSNURL));
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_ETRANS));
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_ERANGE));
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_WAITING));
  TEST_ASSERT_NULL(in3_errmsg(IN3_OK));
  TEST_ASSERT_NULL(in3_errmsg(100));
}

void test_str_replace() {
  char* testdata = "aabbcc abc";
  char* res      = str_replace(testdata, "a", "A");
  TEST_ASSERT_EQUAL_STRING("AAbbcc Abc", res);
  free(res);

  TEST_ASSERT_NULL(str_replace(NULL, "a", "A"));
  TEST_ASSERT_NULL(str_replace("", NULL, "A"));
  TEST_ASSERT_NULL(str_replace("", "", NULL));
  TEST_ASSERT_NULL(str_replace("", "", ""));

  res = str_replace_pos(testdata, 7, 3, "#");
  TEST_ASSERT_EQUAL_STRING("aabbcc #", res);
  free(res);

  TEST_ASSERT_NULL(str_replace_pos(NULL, 0, 0, "A"));

  TEST_ASSERT_EQUAL_STRING("bbcc abc", str_find(testdata, "bb"));

  TEST_ASSERT_NULL(str_find(NULL, "A"));
  TEST_ASSERT_NULL(str_find("ABC", "D"));
}

void test_json() {
  char*       data = "abc";
  json_ctx_t* json = json_create();
  json->result     = json_create_array(json);
  json_array_add_value(json->result, json_create_bool(json, true));
  json_object_add_prop(json->result, key("key"), json_create_null(json));
  json_array_add_value(json->result, json_create_object(json));
  json_array_add_value(json->result, json_create_bytes(json, bytes((uint8_t*) data, 3)));
  json_array_add_value(json->result, json_create_string(json, data));
  json_array_add_value(json->result, json_create_int(json, 10));
  char* jdata = d_create_json(json->result);
  TEST_ASSERT_EQUAL_STRING("[true,null,{},\"0x616263\",\"abc\",\"0xa\"]", jdata);
  free(jdata);
}

static void test_utils() {
  TEST_ASSERT_EQUAL(1, IS_APPROX(5, 4, 1));
  TEST_ASSERT_EQUAL(0, bytes_to_int(NULL, 0));
  uint8_t mem[20] = {0};
  TEST_ASSERT_TRUE(memiszero(mem, 20));
}

/*
 * Main
 */
int main() {
  dbg_log("starting cor tests");

  TESTS_BEGIN();
  RUN_TEST(test_debug);
  RUN_TEST(test_c_to_long);
  RUN_TEST(test_bytes);
  RUN_TEST(test_json);
  RUN_TEST(test_str_replace);
  RUN_TEST(test_utils);
  return TESTS_END();
}
