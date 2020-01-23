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
#include "../src/core/util/data.h"
#include "../src/core/util/error.h"
#include "../src/verifier/ipfs/ipfs.h"
#include "test_utils.h"

static void test_ipfs(void) {
  TEST_ASSERT_EQUAL(IN3_OK, ipfs_verify_hash("01020304FF", "hex", "QmPLrtpXZLUBF24Vu5NXqJg4w4sFYSCFLpWVqqSvmUkL1V"));
  TEST_ASSERT_EQUAL(IN3_OK, ipfs_verify_hash("Hello World", "utf8", "QmUXTtySmd7LD4p6RG6rZW6RuUuPZXTtNMmRQ6DSQo3aMw"));
  // fixme - wrong hash for empty content
  // TEST_ASSERT_EQUAL(IN3_OK, ipfs_verify_hash("", "hex", "QmbFMke1KXqnYyBBWxB74N4c5SBnJMVAiMNRcGu6x1AwQH"));
  // TEST_ASSERT_EQUAL(IN3_OK, ipfs_verify_hash("", "utf8", "QmbFMke1KXqnYyBBWxB74N4c5SBnJMVAiMNRcGu6x1AwQH"));
  TEST_ASSERT_EQUAL(IN3_OK, ipfs_verify_hash("f870830d01cd85057344cdae83015f9094f21051abfe6ac49da790394f2f96fb2ea"
                                             "873efef880df38151b24fcc808026a01d139a50af6cdccf1b76a689a74a00050ae3429454b"
                                             "455473cd048d831ee4ab7a079e558cb7ee74b444ff4ba96035987a04ad6dfaf6addb460208f7a74873bc804",
                                             "hex", "QmaprwXVzwoBR2qVi3wxGAZsYTHqWjExXjPCFJsiND14Fb"));
  TEST_ASSERT_EQUAL(IN3_OK, ipfs_verify_hash("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
                                             "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, "
                                             "quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.",
                                             "utf8", "QmRNW9kJMUuP7dY7YPSxTCLMS9wMYRTADZ5fYf9EHWCabc"));
}

int main() {
  TESTS_BEGIN();
  RUN_TEST(test_ipfs);
  return TESTS_END();
}