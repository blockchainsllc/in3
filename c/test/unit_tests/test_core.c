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

#include "../../src/core/client/request.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/debug.h"
#include "../../src/core/util/utils.h"
#include "../../src/verifier/eth1/nano/eth_nano.h"
#include "../test_utils.h"
#include "nodeselect/full/cache.h"
#include "nodeselect/full/nodelist.h"
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
  _free(clone.data);
}

void test_float_parser() {
  TEST_ASSERT_EQUAL_INT64(1, parse_float_val("1.005", 0));
  TEST_ASSERT_EQUAL_INT64(100, parse_float_val("1.005", 2));
  TEST_ASSERT_EQUAL_INT64(0, parse_float_val("1.005", -2));
  TEST_ASSERT_EQUAL_INT64(8, parse_float_val("800.9", -2));
  TEST_ASSERT_EQUAL_INT64(1, parse_float_val("0.0012", 3));
  TEST_ASSERT_EQUAL_INT64(10, parse_float_val("0.1e2", 0));
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
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_EPAYMENT_REQUIRED));
  TEST_ASSERT_NOT_NULL(in3_errmsg(IN3_EIGNORE));
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
  char*       data  = "abc";
  json_ctx_t* json  = json_create();
  int         array = json_create_array(json);
  json_array_add_value(json, array, json_create_bool(json, true));
  json_object_add_prop(json, array, key("key"), json_create_null(json));
  json_array_add_value(json, array, ((d_token_internal_t*) json->result) + json_create_object(json));
  json_array_add_value(json, array, json_create_bytes(json, bytes((uint8_t*) data, 3)));
  json_array_add_value(json, array, json_create_string(json, data, -1));
  json_array_add_value(json, array, json_create_int(json, 10));
  char* jdata = d_create_json(json, json->result);
  TEST_ASSERT_EQUAL_STRING("[true,null,{},\"0x616263\",\"abc\",10]", jdata);
  free(jdata);
  json_free(json);
}

#define verify_valid_json(json, type, check)                                       \
  {                                                                                \
    json_ctx_t* d   = parse_json(json);                                            \
    char*       err = d ? NULL : parse_json_error(json);                           \
    TEST_ASSERT_NULL_MESSAGE(err, err);                                            \
    TEST_ASSERT_EQUAL_INT_MESSAGE(d_type(d->result), type, "Wrong typ for " json); \
    check;                                                                         \
    json_free(d);                                                                  \
    _free(err);                                                                    \
  }
#define verify_invalid_json(json, err_msg)                                   \
  {                                                                          \
    char* err = parse_json_error(json);                                      \
    TEST_ASSERT_FALSE_MESSAGE(!err, "parsing " json " should have failed "); \
    char* e2 = strchr(err, ':');                                             \
    if (strchr(err, '\n')) strchr(err, '\n')[0] = 0;                         \
    if (e2) e2 += 2;                                                         \
    TEST_ASSERT_EQUAL_STRING(err_msg, e2);                                   \
    _free(err);                                                              \
  }
void test_parse_json() {
  verify_valid_json("\"m/44\\u0027/60\\u0027/0\\u0027/0/0\"", T_STRING, );
  verify_valid_json("1.2e-5", T_STRING, );
  verify_invalid_json("0eb", "Unexpected character");
  verify_valid_json("[{\"id\":0,\"jsonrpc\":\"2.0\",\"error\":{\"message\":\"VM Exception while processing transaction: revert \\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000'\\u0011\",\"code\":-32000,\"data\":{\"0x1f426a9536e776d61eccca7500db78b53a8296ee50977e50d6c76c44f8430571\":{\"error\":\"revert\",\"program_counter\":112,\"return\":\"0x08c379a0000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000002711\",\"reason\":\"\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000'\\u0011\"},\"stack\":\"c: VM Exception while processing transaction: revert \\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000\\u0000'\\u0011\n    at Function.c.fromResults (/Users/simon/ws/custody/cutody-lib/node_modules/ganache-cli/build/ganache-core.node.cli.js:2:157333)\n    at readyCall (/Users/simon/ws/custody/cutody-lib/node_modules/ganache-cli/build/ganache-core.node.cli.js:17:121221)\",\"name\":\"c\"}}}]", T_ARRAY, );
}

void test_sb() {
  sb_t* sb = sb_new("a=\"");
  TEST_ASSERT_EQUAL_STRING("a=\"", sb->data);
  sb_add_escaped_chars(sb, ",x=\"123\"", -1);
  TEST_ASSERT_EQUAL_STRING("a=\",x=\\\"123\\\"", sb->data);

  sb->len            = 0;
  uint8_t*  testdata = (uint8_t*) "\"1234567890\"";
  uint32_t  i        = 5;
  bytes32_t b        = {0};
  long_to_bytes(12345678901, b + 24);
  sb_printx(sb, "a=%B,b=%i,c=%s,d=%S,e=%w", bytes(testdata, 10), i, testdata, testdata, bytes(b, 32));
  TEST_ASSERT_EQUAL_STRING("a=0x22313233343536373839,b=5,c=\"1234567890\",d=\\\"1234567890\\\",e=12345678901", sb->data);

  sb_free(sb);
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
  RUN_TEST(test_parse_json);
  RUN_TEST(test_float_parser);
  RUN_TEST(test_debug);
  RUN_TEST(test_c_to_long);
  RUN_TEST(test_bytes);
  RUN_TEST(test_json);
  RUN_TEST(test_str_replace);
  RUN_TEST(test_sb);
  RUN_TEST(test_utils);
  return TESTS_END();
}
