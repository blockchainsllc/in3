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
#include "../src/core/util/data.h"
#include "../src/core/util/utils.h"
#include "../src/verifier/eth1/basic/trie.h"
#include "../src/verifier/eth1/evm/evm.h"
#include "../src/verifier/eth1/nano/rlp.h"
#include "../src/verifier/eth1/nano/serialize.h"
#include <inttypes.h>
#include <stdio.h>
#include <time.h>

#include "vm_runner.h"

void append_rlp(bytes_builder_t* bb, d_token_t* in) {
  int i;
  if (d_type(in) == T_ARRAY) {
    bytes_builder_t* tmp = bb_new();
    d_token_t*       t   = NULL;
    for (i = 0, t = in + 1; i < d_len(in); i++, t = d_next(t))
      append_rlp(tmp, t);
    rlp_encode_list(bb, &tmp->b);
    bb_free(tmp);
    return;
  }
  bytes_t b = d_to_bytes(in);
  rlp_encode_item(bb, &b);
}

int test_rlp(d_token_t* test, uint32_t props, uint64_t* ms) {
  uint64_t   start = clock();
  int        res   = 0;
  d_token_t* in    = d_get(test, key("in"));
  bytes_t    out   = d_to_bytes(d_get(test, key("out")));
  if (d_type(in) == T_STRING && d_len(in) > 0 && *in->data == '#') return 0;
  bytes_builder_t* bb = bb_new();
  append_rlp(bb, in);
  if (!b_cmp(&bb->b, &out)) {
    print_error("Wrong result");
    printf("\nexpected:");
    ba_print(out.data, out.len);
    printf("\nis      :");
    ba_print(bb->b.data, bb->b.len);
    res = -1;
  }
  bb_free(bb);

  *ms = (clock() - start) / 1000;
  return res;
}