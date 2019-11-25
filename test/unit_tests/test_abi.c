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
#endif
#ifndef TEST
#define DEBUG
#endif

#include "../../src/api/eth1/abi.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/log.h"
#include "../test_utils.h"
#include <stdio.h>
#include <unistd.h>

#define err_string(msg) ("Error:" msg)

char* abi_encode(char* sig, char* json_params) {
  call_request_t* req = parseSignature(sig);
  if (!req) return err_string("invalid function signature");

  json_ctx_t* params = parse_json(json_params);
  if (!params) {
    req_free(req);
    return err_string("invalid json data");
  }

  if (set_data(req, params->result, req->in_data) < 0) {
    req_free(req);
    free_json(params);
    return err_string("invalid input data");
  }
  free_json(params);
  char* result = malloc(req->call_data->b.len * 2 + 3);
  if (!result) {
    req_free(req);
    return err_string("malloc failed for the result");
  }
  bytes_to_hex(req->call_data->b.data, req->call_data->b.len, result + 2);
  result[0] = '0';
  result[1] = 'x';
  req_free(req);
  return result;
}

char* abi_decode(char* sig, char* hex_data) {
  call_request_t* req = parseSignature(sig);
  if (!req) return err_string("invalid function signature");
  int     l = strlen(hex_data);
  uint8_t data[l >> 1];
  l = hex2byte_arr(hex_data, -1, data, l >> 1);

  json_ctx_t* res = req_parse_result(req, bytes(data, l));
  req_free(req);
  if (!res)
    return err_string("the input data can not be decoded");
  char* result = d_create_json(res->result);
  free_json(res);
  // Enclose output in square brackets if not already the case
  if (result[0] != '[') {
    size_t l_ = strlen(result);
    char*  r_ = malloc(l_ + 3);
    r_[0]     = '[';
    for (size_t i = 0; i < l_; ++i)
      r_[i + 1] = result[i];
    r_[l_ + 1] = ']';
    r_[l_ + 2] = '\0';
    free(result);
    result = r_;
  }
  return result;
}

#define TEST_ABI(signature, input, expected)                                                         \
  {                                                                                                  \
    char* tmp = abi_encode("test(" signature ")", input);                                            \
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, tmp + 10, "Error encoding the signature " signature); \
    free(tmp);                                                                                       \
    tmp = abi_decode("test():(" signature ")", expected);                                            \
    TEST_ASSERT_EQUAL_STRING_MESSAGE(input, tmp, "Error decoding " signature);                       \
    free(tmp);                                                                                       \
  }

static void test_abi_encode() {
  TEST_ABI("address,string", "[\"0x1234567890123456789012345678901234567890\",\"xyz\"]",
           "00000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000000000000000000000000000000000040000000000000000000000000000000000000000000000000000000000000000378797a0000000000000000000000000000000000000000000000000000000000")
  TEST_ABI("address,string,uint8,string", "[\"0x1234567890123456789012345678901234567890\",\"xyz\",\"0xff\",\"abc\"]",
           "0000000000000000000000001234567890123456789012345678901234567890000000000000000000000000000000000000000000000000000000000000008000000000000000000000000000000000000000000000000000000000000000ff00000000000000000000000000000000000000000000000000000000000000c0000000000000000000000000000000000000000000000000000000000000000378797a000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000036162630000000000000000000000000000000000000000000000000000000000")
  TEST_ABI("bytes32,bool", "[\"0x0000000000000000000000000000000000000000000000001234567890abcdef\",false]",
           "0000000000000000000000000000000000000000000000001234567890abcdef0000000000000000000000000000000000000000000000000000000000000000")
  TEST_ABI("uint256,bool", "[\"0x1234567890abcdef\",true]",
           "0000000000000000000000000000000000000000000000001234567890abcdef0000000000000000000000000000000000000000000000000000000000000001")
}
/*
 * Main
 */
int main() {
  // now run tests
  TESTS_BEGIN();
  RUN_TEST(test_abi_encode);
  return TESTS_END();
}
/*
0000000000000000000000001234567890123456789012345678901234567890
0000000000000000000000000000000000000000000000000000000000000040
0000000000000000000000000000000000000000000000000000000000000003
78797a0000000000000000000000000000000000000000000000000000000000

0000000000000000000000001234567890123456789012345678901234567890
0000000000000000000000000000000000000000000000000000000000000020
0000000000000000000000000000000000000000000000000000000000000003
78797a0000000000000000000000000000000000000000000000000000000000
*/