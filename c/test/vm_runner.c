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
#include "vm_runner.h"
#include "../src/core/client/context.h"
#include "../src/core/client/keys.h"
#include "../src/core/util/data.h"
#include "../src/core/util/log.h"
#include "../src/core/util/mem.h"
#include "../src/verifier/eth1/evm/evm.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int use_color = 1;

void print_error(char* msg) {
  if (use_color)
    in3_log_debug("\x1B[31m%s\x1B[0m", msg);
  else
    in3_log_debug("!! %s", msg);
}
void print_success(char* msg) {
  if (use_color)
    in3_log_debug("\x1B[32m%s\x1B[0m", msg);
  else
    in3_log_debug(".. %s", msg);
}

char* readContent(char* name) {
  char temp[500];
  sprintf(temp, strchr(name, '.') == NULL ? "../test/testdata/%s.json" : "%s", name);
  FILE* file = fopen(temp, "r");
  if (file == NULL) {
    ERROR("could not open the file");
    return NULL;
  }

  size_t allocated = 1024, len = 0, r = 0;
  char*  buffer = malloc(1024);
  while (1) {
    r = fread(buffer + len, 1, allocated - len - 1, file);
    len += r;
    if (feof(file)) break;
    size_t new_alloc = allocated * 2;
    buffer           = _realloc(buffer, new_alloc, allocated);
    allocated        = new_alloc;
  }
  buffer[len] = 0;

  if (file)
    fclose(file);

  return buffer;
}

int run_test(d_token_t* test, int counter, char* name, uint32_t props) {
  char  temp[300];
  char* descr = NULL;
  int   i;

  char* sname = strstr(name, "/testdata/");
  if (sname) name = sname + 10;
  int l = strlen(name), fail = 0;
  if (name[l - 5] == '.') name[l - 5] = 0;
  char*    tname = d_get_keystr(test->key);
  uint64_t ms    = 0;

  // debug
  //  if (strcmp(tname, "dup1") == 0) props |= EVM_PROP_DEBUG;

  if (tname && strstr(name, tname))
    sprintf(temp, "%s", name);
  else if (tname)
    sprintf(temp, "%s : %s", name, tname);
  else
    sprintf(temp, "%s #%i", name, counter);
  in3_log_debug("\n%2i : %-80s ", counter, temp);
  fflush(stdout);

  d_token_t* exec        = d_get(test, key("exec"));
  d_token_t* transaction = d_get(test, key("transaction"));

  if (d_len(test) == 2 && d_get(test, key("in")) && d_get(test, key("out")))
    fail = test_rlp(test, props, &ms);
  else if (d_get(test, key("root")) && d_get(test, key("in")))
    fail = test_trie(test, props | (strstr(name, "secure") ? 2 : 0), &ms);
  else if (exec || transaction)
    fail = test_evm(test, props, &ms);
  else {
    fail = -1;
    print_error("Unknown TestType!");
  }

  if (mem_get_memleak_cnt()) {
    in3_log_debug(" -- Memory Leak detected by malloc #%i!", mem_get_memleak_cnt());
    if (!fail) fail = 1;
  }
  if (!fail) print_success("OK");

  in3_log_debug(" ( heap: %zu, %" PRIu64 " ms) ", mem_get_max_heap(), ms);

  return fail;
}

int runRequests(char** names, int test_index, int mem_track, uint32_t props) {
  int   res = 0, n = 0;
  char* name   = names[n];
  int   failed = 0, total = 0, count = 0;
  while (name) {
    char* content = readContent(name);
    char  tmp[300];
    if (content == NULL) {
      print_error("Filename not found!\n");
      return -1;
    }
    d_track_keynames(1);

    json_ctx_t* parsed = parse_json(content);
    if (!parsed) {
      free(content);
      ERROR("Error parsing the requests");
      return -1;
    }
    // parse the data;
    int        i;
    char*      str_proof = NULL;
    d_token_t *t = NULL, *tests = NULL, *test = NULL;
    d_token_t* tokens = NULL;

    if ((tests = parsed->result)) {
      for (i = 0, test = tests + 1; i < d_len(tests); i++, test = d_next(test)) {
        count++;
        if (test_index < 0 || count == test_index) {
          total++;
          mem_reset(mem_track);
          if (run_test(test, count, name, props)) failed++;
        }
      }
    }

    free(content);
    json_free(parsed);
    d_clear_keynames();
    name = names[++n];
  }
  in3_log_debug("\n( %i %%)  %2i of %2i successfully tested", total ? ((total - failed) * 100) / total : 0, total - failed, total);

  if (failed) {
    in3_log_debug("\n%2i tests failed", failed);
    res = failed;
  }
  in3_log_debug("\n");

  return failed;
}

int main(int argc, char* argv[]) {
  int    i = 0, size = 1;
  int    testIndex = -1, membrk = -1;
  char** names        = malloc(sizeof(char*));
  names[0]            = NULL;
  uint32_t props      = 0;
  char*    skip_tests = getenv("IN3_SKIPTESTS");
  if (skip_tests) {
    char* token = strtok(skip_tests, ",");
    while (token != NULL) {
      for (int i = 1; i < argc; i++) {
        if (strstr(argv[i], token))
          *argv[i] = 0;
      }
      token = strtok(NULL, ",");
    }
  }

  in3_log_set_level(LOG_DEBUG);
  in3_log_set_prefix("");

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-t") == 0)
      testIndex = atoi(argv[++i]);
    else if (strcmp(argv[i], "-m") == 0)
      membrk = atoi(argv[++i]);
    else if (strcmp(argv[i], "-d") == 0)
      in3_log_set_level(LOG_TRACE);
    else if (strcmp(argv[i], "-c") == 0)
      props |= EVM_PROP_CONSTANTINOPL;
    else if (strlen(argv[i])) {
      //      if (strstr(argv[i], "exp") || strstr(argv[i], "loop-mulmod")) {
      //        printf("\nskipping %s\n", argv[i]);
      //        continue;
      //      }

      char** t = malloc((size + 1) * sizeof(char*));
      memmove(t, names, size * sizeof(char*));
      free(names);
      names           = t;
      names[size - 1] = argv[i];
      names[size++]   = NULL;
    }
  }

  int ret = runRequests(names, testIndex, membrk, props);
  free(names);
  return ret;
}