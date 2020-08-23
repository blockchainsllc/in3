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

#include "../../src/core/util/data.h"
#include "../../src/core/util/mem.h"
#include "../../src/core/util/used_keys.h"
#include <stdio.h>
#include <string.h>

#include "../test_utils.h"

typedef struct {
  uint16_t k;
  char*    name;
} key_hash_t;

static void test_key_hash_collisions();

// Internal implementation that mirrors key() from data.c with IN3_DONT_HASH_KEYS not defined
d_key_t key_(const char* c) {
  uint16_t val = 0;
  size_t   l   = strlen(c);
  for (; l; l--, c++) val ^= *c | val << 7;
  return val;
}

/*
 * Main
 */
int main() {
  TESTS_BEGIN();
  RUN_TEST(test_key_hash_collisions);
  return TESTS_END();
}

static int compare(const void* a, const void* b) {
  return (((key_hash_t*) a)->k - ((key_hash_t*) b)->k);
}

static bool is_allowed(char* a, char* b) {
  // this function may contains exceptions which are ok

  if (strcmp(a, "autoUpdateList") == 0 && strcmp(b, "params") == 0) return true; // params and autoupdateList will not appear in the same object
  return false;
}

static int collision(key_hash_t* hashes, size_t sz) {
  bool dup = false;
  int  i   = 1;
  for (; i < sz; ++i) {
    if (hashes[i].k == hashes[i - 1].k) {
      if (is_allowed(hashes[i - 1].name, hashes[i].name)) continue;

      printf("Collision :%s <-> %s : [%u]\n", hashes[i - 1].name, hashes[i].name, hashes[i].k);
      dup = true;
      break;
    }
  }
  return dup ? i : 0;
}

void test_key_hash_collisions() {
  size_t      cap    = 10;
  key_hash_t* hashes = _malloc(cap * sizeof(key_hash_t));
  TEST_ASSERT(hashes != NULL);
  int key_total = 0;
  for (int i = 0; USED_KEYS[i]; i++) {
    key_total++;
    if (i == cap) {
      hashes = _realloc(hashes, cap * 2 * sizeof(key_hash_t), cap);
      cap *= 2;
    }
    hashes[i].name = USED_KEYS[i];
    hashes[i].k    = key_(USED_KEYS[i]);
  }
  qsort(hashes, key_total, sizeof(key_hash_t), compare);
  int nc = collision(hashes, key_total);
  TEST_ASSERT_EQUAL(0, nc);
  _free(hashes);
}