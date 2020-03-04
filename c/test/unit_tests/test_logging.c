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
#define DEBUG
#endif

#include "../../src/core/util/log.h"
#include "../../src/core/util/mem.h"
#include "../test_utils.h"
#include <string.h>

static char* read_file(FILE* file) {
  if (file == NULL) {
    printf("File not found!");
    _Exit(1);
    return NULL;
  }

  size_t   allocated = 1024;
  size_t   len       = 0;
  uint8_t* buffer    = malloc(1025);
  size_t   r;

  while (1) {
    r = fread(buffer + len, 1, allocated - len, file);
    len += r;
    if (feof(file)) break;
    size_t new_alloc = allocated * 2 + 1;
    buffer           = _realloc(buffer, new_alloc, allocated);
    allocated        = new_alloc;
  }

  if (len && buffer[len - 1] == '\n') buffer[len - 1] = 0;

  buffer[len] = 0;
  return (char*) buffer;
}

static void lock_fn(void* udata, int lock_en) {
  bool* lock = udata;
  *lock      = lock_en;
}

static void test_locking(void) {
  bool lock = false;
  in3_log_set_udata(&lock);
  in3_log_set_lock(lock_fn);
  in3_log_trace("Test log\n");
  TEST_ASSERT_FALSE(lock);
}

static void test_prefix(void) {
  const char* prefix = "::<< >>";
  size_t      size;
  char*       log;

  in3_log_enable_prefix();
  in3_log_set_prefix(prefix);
  in3_log_set_quiet(false);
  in3_log_set_level(LOG_TRACE);

  FILE* fp = fopen("test.log", "w+");
  in3_log_set_fp(fp);
  in3_log_info("Testing prefix...");
  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  fclose(fp);

  fp        = fopen("test.log", "rb");
  log       = _malloc(size + 1);
  log[size] = 0;
  fread(log, sizeof(char), size, fp);
  TEST_ASSERT_EQUAL_STRING_LEN(log, prefix, strlen(prefix));
  fclose(fp);
}

/*
 * Main
 */
int main() {
  TESTS_BEGIN();
  RUN_TEST(test_locking);
  RUN_TEST(test_prefix);
  return TESTS_END();
}
