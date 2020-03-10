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
#include <stdio.h>
#include <string.h>

#include "../test_utils.h"

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

static char* substr(char* s, char* delim1, char* delim2) {
  char *target, *start, *end;
  target = start = end = NULL;

  if ((start = strstr(s, delim1))) {
    start += strlen(delim1);
    if ((end = strstr(start, delim2))) {
      target = start;
      *end   = '\0';
    }
  }
  return target;
}

static int compare(const void* a, const void* b) {
  return (*(uint16_t*) a - *(uint16_t*) b);
}

static uint16_t collision(uint16_t* hashes, size_t sz) {
  bool dup = false;
  int  i   = 1;
  for (; i < sz; ++i) {
    if (hashes[i] && hashes[i] == hashes[i - 1]) {
      dup = true;
      break;
    }
  }
  return dup ? hashes[i] : 0;
}

static char* filetostr(const char* filename) {
  char* buffer = NULL;
  long  length;
  FILE* f = fopen(filename, "r");
  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer         = _malloc(length + 1);
    buffer[length] = 0;
    if (buffer) {
      fread(buffer, 1, length, f);
    }
    fclose(f);
  }
  return buffer;
}

void test_key_hash_collisions() {
  size_t    cap    = 10;
  uint16_t* hashes = _malloc(cap * sizeof(*hashes));
  TEST_ASSERT(hashes != NULL);

  const char* delim      = "\n";
  char*       keyfilestr = filetostr("../c/src/core/client/keys.h");
  TEST_ASSERT_MESSAGE(keyfilestr != NULL, "File keys.h not found!");
  // skip legal header
  char* keys = strstr(keyfilestr, "*/");
  keys       = keys ? keys + 3 : keyfilestr;

  char* tok = strtok(keys, delim);
  int   i   = 0;
  for (; tok != NULL; i++, tok = strtok(NULL, delim)) {
    if (i == cap) {
      hashes = _realloc(hashes, cap * 2 * sizeof(*hashes), cap);
      cap *= 2;
    }
    char* kstr = substr(tok, "key(\"", "\")");
    if (kstr) {
      hashes[i] = key_(kstr);
#ifdef DEBUG
      printf("\"%s\" => [%u]\n", kstr, hashes[i]);
#endif
    } else {
      hashes[i] = 0;
    }
  }
  uint16_t nc = -1;
  qsort(hashes, i, sizeof(uint16_t), compare);
  nc = collision(hashes, i);
  TEST_ASSERT_EQUAL(0, nc);
  _free(keyfilestr);
  _free(hashes);
}