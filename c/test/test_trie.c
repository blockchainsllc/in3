
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
#include "../src/core/util/log.h"
#include "../src/core/util/utils.h"
#include "../src/verifier/eth1/basic/trie.h"
#include "../src/verifier/eth1/evm/evm.h"
#include "../src/verifier/eth1/nano/serialize.h"
#include <inttypes.h>
#include <string.h>
#include <time.h>

#include "vm_runner.h"

bytes_t get_bytes(d_token_t* t, uint8_t* tmp, uint8_t is_hex) {
  bytes_t res;
  res = d_to_bytes(t);
  /*
  if (d_type(t) == T_BYTES && !is_hex) {
    tmp[0] = '0';
    tmp[1] = 'x';
    bytes_to_hex(res.data, res.len, (char*) tmp + 2);
    res.data = tmp;
    res.len  = res.len * 2 + 2;
  }
  */
  return res;
}

int test_trie(d_token_t* test, uint32_t props, uint64_t* ms) {

  if (props & 2) {
    EVM_DEBUG_BLOCK({
      in3_log_trace("\n using secure trie and hashing the key...\n");
    });
  }
  uint64_t   start  = clock();
  trie_t*    trie   = trie_new();
  d_token_t *in     = d_get(test, key("in")), *t, *el;
  uint8_t    is_hex = d_get_int(test, "hexEncoded"), i, tmp[64], tmp2[64], tmp3[32], res = 0;

  if (d_type(in) == T_ARRAY) {
    for (i = 0, t = in + 1; i < d_len(in); i++, t = d_next(t)) {
      bytes_t    key_bytes = get_bytes(d_get_at(t, 0), tmp2, is_hex), value_bytes = get_bytes(d_get_at(t, 1), tmp, is_hex);
      uint8_t    will_be_null = 0;
      bytes_t    tmp_key;
      int        n;
      d_token_t* tt = NULL;
      for (n = i + 1, tt = d_next(t); n < d_len(in) && !will_be_null; n++, tt = d_next(tt)) {
        tmp_key = d_to_bytes(d_get_at(tt, 0));

        if (b_cmp(&key_bytes, &tmp_key) && d_type(d_get_at(tt, 1)) == T_NULL)
          will_be_null = 1;
      }
      if (props & 2) {
        sha3_to(&key_bytes, tmp3);
        key_bytes.data = tmp3;
        key_bytes.len  = 32;
      }

      if (!will_be_null)
        trie_set_value(trie, &key_bytes, &value_bytes);

      EVM_DEBUG_BLOCK({
        in3_log_trace(will_be_null ? "\n\n_____________________\n%i:####### SKIP " : "\n\n_____________________\n%i:####### SET ", i + 1);
        ba_print(key_bytes.data, key_bytes.len);
        in3_log_trace(" = ");
        ba_print(value_bytes.data, value_bytes.len);
      });

#ifdef TRIETEST
      trie_dump(trie, 0);
#endif
    }
  } else {

    for (i = 0, t = in + 1; i < d_len(in); i++, t = d_next(t)) {
      char*   k = d_get_keystr(t->key);
      bytes_t key_bytes, value_bytes = get_bytes(t, tmp, is_hex);
      if (k[0] == '0' && k[1] == 'x') {
        key_bytes.data = tmp;
        key_bytes.len  = hex_to_bytes(k + 2, strlen(k) - 2, tmp, 64);
      } else {
        key_bytes.data = (uint8_t*) k;
        key_bytes.len  = strlen(k);
      }

      if (props & 2) {
        sha3_to(&key_bytes, tmp3);
        key_bytes.data = tmp3;
        key_bytes.len  = 32;
      }
      trie_set_value(trie, &key_bytes, &value_bytes);
      EVM_DEBUG_BLOCK({
        in3_log_trace("\n\n_____________________\n%i:####### SET ", i + 1);
        ba_print(key_bytes.data, key_bytes.len);
        in3_log_trace(" = ");
        ba_print(value_bytes.data, value_bytes.len);
      });
#ifdef TRIETEST
      trie_dump(trie, 0);
#endif
    }
  }
  bytes_t root_bytes = d_to_bytes(d_get(test, key("root")));
  if (root_bytes.len == 32 && memcmp(root_bytes.data, trie->root, 32)) {
    EVM_DEBUG_BLOCK({
      in3_log_trace("\n expected : ");
      ba_print(root_bytes.data, 32);
      in3_log_trace("\n       is : ");
      ba_print(trie->root, 32);
    });
    print_error("wrong root-hash");
    res = 1;
  }

  trie_free(trie);
  *ms = (clock() - start) / 1000;
  return res;
}