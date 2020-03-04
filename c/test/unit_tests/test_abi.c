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
#include "../../src/core/util/data.h"
#include "../../src/core/util/log.h"
#include "../test_utils.h"
#include <stdio.h>
#include <unistd.h>

#define err_string(msg) ("Error:" msg)
#define TEST_ABI(signature, input, expected)                                                         \
  {                                                                                                  \
    char* tmp = abi_encode("test(" signature ")", input);                                            \
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, tmp + 10, "Error encoding the signature " signature); \
    free(tmp);                                                                                       \
    tmp = abi_decode("test():(" signature ")", expected);                                            \
    TEST_ASSERT_EQUAL_STRING_MESSAGE(input, tmp, "Error decoding " signature);                       \
    free(tmp);                                                                                       \
  }
#define TEST_ABI_DESC(description, signature, input, expected) TEST_ABI(signature, input, expected)
#define TEST_ASSERT_ABI_ENCODE(description, signature, input, output)                 \
  {                                                                                   \
    char* tmp = abi_encode("test(" signature ")", input);                             \
    TEST_ASSERT_EQUAL_STRING(tmp + (!strncmp(output, "Error:", 6) ? 0 : 10), output); \
    if (tmp) free(tmp);                                                               \
  }
#define TEST_ASSERT_ABI_DECODE(description, signature, input, output) \
  {                                                                   \
    char* tmp = abi_decode("test():(" signature ")", input);          \
    TEST_ASSERT_EQUAL_STRING(tmp, output);                            \
    if (tmp) free(tmp);                                               \
  }

static char* abi_encode(char* sig, char* json_params) {
  call_request_t* req = parseSignature(sig);
  if (!req) return strdup(err_string("invalid function signature"));

  json_ctx_t* params = parse_json(json_params);
  if (!params) {
    req_free(req);
    return strdup(err_string("invalid json data"));
  }

  if (set_data(req, params->result, req->in_data) < 0) {
    req_free(req);
    json_free(params);
    return strdup(err_string("invalid input data"));
  }
  json_free(params);
  char* result = malloc(req->call_data->b.len * 2 + 3);
  if (!result) {
    req_free(req);
    return strdup(err_string("malloc failed for the result"));
  }
  bytes_to_hex(req->call_data->b.data, req->call_data->b.len, result + 2);
  result[0] = '0';
  result[1] = 'x';
  req_free(req);
  return result;
}

static char* abi_decode(char* sig, char* hex_data) {
  call_request_t* req = parseSignature(sig);
  if (!req) return strdup(err_string("invalid function signature"));
  int     l = strlen(hex_data);
  uint8_t data[l >> 1];
  l = hex_to_bytes(hex_data, -1, data, l >> 1);

  json_ctx_t* res = req_parse_result(req, bytes(data, l));
  req_free(req);
  if (!res)
    return strdup(err_string("the input data can not be decoded"));
  char* result = d_create_json(res->result);
  json_free(res);
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

static void test_abi_encode_decode() {
  TEST_ABI("address,string", "[\"0x1234567890123456789012345678901234567890\",\"xyz\"]",
           "00000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000000000000000000000000000000000040000000000000000000000000000000000000000000000000000000000000000378797a0000000000000000000000000000000000000000000000000000000000")
  TEST_ABI("address,string,uint8,string", "[\"0x1234567890123456789012345678901234567890\",\"xyz\",\"0xff\",\"abc\"]",
           "0000000000000000000000001234567890123456789012345678901234567890000000000000000000000000000000000000000000000000000000000000008000000000000000000000000000000000000000000000000000000000000000ff00000000000000000000000000000000000000000000000000000000000000c0000000000000000000000000000000000000000000000000000000000000000378797a000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000036162630000000000000000000000000000000000000000000000000000000000")
  TEST_ABI("bytes32,bool", "[\"0x0000000000000000000000000000000000000000000000001234567890abcdef\",false]",
           "0000000000000000000000000000000000000000000000001234567890abcdef0000000000000000000000000000000000000000000000000000000000000000")
  TEST_ABI("uint256,bool", "[\"0x1234567890abcdef\",true]",
           "0000000000000000000000000000000000000000000000001234567890abcdef0000000000000000000000000000000000000000000000000000000000000001")
  TEST_ABI("bool,uint32", "[true,\"0x45\"]",
           "00000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000045")
  TEST_ABI("uint8", "[\"0xff\"]",
           "00000000000000000000000000000000000000000000000000000000000000ff")
  TEST_ABI("uint", "[\"0x10000000000000000000000000000000000000000000000000000000000000ff\"]",
           "10000000000000000000000000000000000000000000000000000000000000ff")
  TEST_ABI("uint8", "[\"0x1f\"]", "000000000000000000000000000000000000000000000000000000000000001f")
  TEST_ABI("int8", "[\"-1\"]", "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff")
  //  TEST_ABI("int8", "[2]", "0000000000000000000000000000000000000000000000000000000000000002")
  TEST_ABI_DESC("official test vector 1", "uint32,bool", "[\"0x45\",true]", "00000000000000000000000000000000000000000000000000000000000000450000000000000000000000000000000000000000000000000000000000000001")
  TEST_ABI_DESC("official test vector 3", "bytes,bool,uint256[]", "[\"0x64617665\",true,[\"0x01\",\"0x02\",\"0x03\"]]", "0000000000000000000000000000000000000000000000000000000000000060000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000a0000000000000000000000000000000000000000000000000000000000000000464617665000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000003000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000003")
  //  TEST_ABI("official test vector 4", "uint,uint32,bytes10,bytes[]", "[\"0x123\",[\"0x456\",\"0x789\"],\"1234567890\",\"Hello, world!\"]", "00000000000000000000000000000000000000000000000000000000000001230000000000000000000000000000000000000000000000000000000000000080313233343536373839300000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000e0000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000004560000000000000000000000000000000000000000000000000000000000000789000000000000000000000000000000000000000000000000000000000000000d48656c6c6f2c20776f726c642100000000000000000000000000000000000000")
  TEST_ABI_DESC("negative int32", "int32", "[\"-97\"]", "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff9f")
  //  TEST_ABI_DESC("negative int256", "int256", "[\"-19999999999999999999999999999999999999999999999999999999999999\"]", "fffffffffffff38dd0f10627f5529bdb2c52d4846810af0ac000000000000001")
  TEST_ABI_DESC("string >32bytes", "string", "[\" hello world hello world hello world hello world  hello world hello world hello world hello world  hello world hello world hello world hello world hello world hello world hello world hello world\"]", "000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000c22068656c6c6f20776f726c642068656c6c6f20776f726c642068656c6c6f20776f726c642068656c6c6f20776f726c64202068656c6c6f20776f726c642068656c6c6f20776f726c642068656c6c6f20776f726c642068656c6c6f20776f726c64202068656c6c6f20776f726c642068656c6c6f20776f726c642068656c6c6f20776f726c642068656c6c6f20776f726c642068656c6c6f20776f726c642068656c6c6f20776f726c642068656c6c6f20776f726c642068656c6c6f20776f726c64000000000000000000000000000000000000000000000000000000000000")
  TEST_ABI_DESC("uint32 response", "uint32", "[\"0x2a\"]", "000000000000000000000000000000000000000000000000000000000000002a")
  TEST_ABI("string,uint256[2]", "[\"foo\",[\"0x05\",\"0x06\"]]", "0000000000000000000000000000000000000000000000000000000000000060000000000000000000000000000000000000000000000000000000000000000500000000000000000000000000000000000000000000000000000000000000060000000000000000000000000000000000000000000000000000000000000003666f6f0000000000000000000000000000000000000000000000000000000000")
  TEST_ABI_DESC("utf8 handling", "string", "[\"ethereum számítógép\"]", "00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000017657468657265756d20737ac3a16dc3ad74c3b367c3a970000000000000000000")
  TEST_ABI_DESC("non-latin chars", "string", "[\"为什么那么认真？\"]", "00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000018e4b8bae4bb80e4b988e982a3e4b988e8aea4e79c9fefbc9f0000000000000000")
  //  TEST_ABI_DESC("ufixed", "ufixed128x128", "[\"0x01\"]", "0000000000000000000000000000000100000000000000000000000000000000")
  //  TEST_ABI_DESC("ufixed", "fixed128x128", "[\"-1\"]", "0000000000000000000000000000000100000000000000000000000000000000")
}

static void test_abi_encode_edge_cases() {
  TEST_ASSERT_ABI_ENCODE("empty data", "bytes33", "", err_string("invalid json data"));
  //  TEST_ASSERT_ABI_ENCODE("invalid uint suffix", "uint0", "[1]", err_string("invalid input data"));
  //  TEST_ASSERT_ABI_ENCODE("invalid uint suffix", "uint257", "[1]", err_string("invalid input data"));
  //  TEST_ASSERT_ABI_ENCODE"invalid int suffix", ("int0", "[1]", err_string("invalid input data"));
  //  TEST_ASSERT_ABI_ENCODE("invalid int suffix", "int257", "[1]", err_string("invalid input data"));
  TEST_ASSERT_ABI_ENCODE("array size mismatch", "uint[2]", "[[1,2,3]]", err_string("invalid input data"));
  //  TEST_ASSERT_ABI_ENCODE("data size exceeds data type", "uint8", "[\"0x111\"]", err_string("invalid input data"));
  //  TEST_ASSERT_ABI_ENCODE("-1 as uint", "uint", "[\"-1\"]", err_string("invalid input data"));
  TEST_ASSERT_ABI_ENCODE("encoding 256 bits as bytes", "bytes", "[\"0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff\"]", "00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000020ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
}

static void test_abi_decode_edge_cases() {
  TEST_ASSERT_ABI_DECODE("address with leading 0s", "address", "0000000000000000000000000005b7d915458ef540ade6068dfe2f44e8fa733c", "[\"0x0005b7d915458ef540ade6068dfe2f44e8fa733c\"]");
  TEST_ASSERT_ABI_DECODE("array size mismatch", "uint[2]", "000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000003", "[\"0x01\",\"0x02\"]");
  //  TEST_ASSERT_ABI_DECODE("multi-dimensional array",
  //                         "(uint128[2][3],uint)",
  //                         "0000000000000000000000000000000000000000000000000000000000000001"
  //                         "0000000000000000000000000000000000000000000000000000000000000002"
  //                         "0000000000000000000000000000000000000000000000000000000000000003"
  //                         "0000000000000000000000000000000000000000000000000000000000000004"
  //                         "0000000000000000000000000000000000000000000000000000000000000005"
  //                         "0000000000000000000000000000000000000000000000000000000000000006"
  //                         "000000000000000000000000000000000000000000000000000000000000000a",
  //                         "[]");
  //  TEST_ASSERT_ABI_DECODE("multi-dimensional array 2",
  //                         "(uint128[2][3][2],uint)",
  //                         "0000000000000000000000000000000000000000000000000000000000000001"
  //                         "0000000000000000000000000000000000000000000000000000000000000002"
  //                         "0000000000000000000000000000000000000000000000000000000000000003"
  //                         "0000000000000000000000000000000000000000000000000000000000000004"
  //                         "0000000000000000000000000000000000000000000000000000000000000005"
  //                         "0000000000000000000000000000000000000000000000000000000000000006"
  //                         "0000000000000000000000000000000000000000000000000000000000000001"
  //                         "0000000000000000000000000000000000000000000000000000000000000002"
  //                         "0000000000000000000000000000000000000000000000000000000000000003"
  //                         "0000000000000000000000000000000000000000000000000000000000000004"
  //                         "0000000000000000000000000000000000000000000000000000000000000005"
  //                         "0000000000000000000000000000000000000000000000000000000000000006"
  //                         "000000000000000000000000000000000000000000000000000000000000000a",
  //                         "[]");
}

static void test_abi_tuples() {
  TEST_ABI("(bytes4,string)", "[\"0x0259ba8c\",\"https://search-test-usn.slock.it\"]",
           "0259ba8c000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000040000000000000000000000000000000000000000000000000000000000000002068747470733a2f2f7365617263682d746573742d75736e2e736c6f636b2e6974")
  TEST_ABI("(address,string)", "[\"0x1234567890123456789012345678901234567890\",\"xyz\"]",
           "00000000000000000000000012345678901234567890123456789012345678900000000000000000000000000000000000000000000000000000000000000040000000000000000000000000000000000000000000000000000000000000000378797a0000000000000000000000000000000000000000000000000000000000")
  TEST_ABI("address,string,(uint8,string)", "[\"0x1234567890123456789012345678901234567890\",\"xyz\",\"0xff\",\"abc\"]",
           "0000000000000000000000001234567890123456789012345678901234567890000000000000000000000000000000000000000000000000000000000000008000000000000000000000000000000000000000000000000000000000000000ff00000000000000000000000000000000000000000000000000000000000000c0000000000000000000000000000000000000000000000000000000000000000378797a000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000036162630000000000000000000000000000000000000000000000000000000000")
  TEST_ABI("(bytes32,bool)", "[\"0x0000000000000000000000000000000000000000000000001234567890abcdef\",false]",
           "0000000000000000000000000000000000000000000000001234567890abcdef0000000000000000000000000000000000000000000000000000000000000000")
}

/*
 * Main
 */
int main() {
  // now run tests
  TESTS_BEGIN();
  RUN_TEST(test_abi_encode_decode);
  RUN_TEST(test_abi_encode_edge_cases);
  RUN_TEST(test_abi_decode_edge_cases);
  RUN_TEST(test_abi_tuples);
  return TESTS_END();
}
