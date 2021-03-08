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

#include "mem.h"
#include "debug.h"
#include "log.h"
#include <stdlib.h>
#ifdef __ZEPHYR__
void* k_realloc(void* ptr, size_t size, size_t oldsize) {
  void* new = NULL;

  new = k_malloc(size);
  if (!new)
    goto error;

  if (ptr && oldsize) {
    memcpy(new, ptr, oldsize);
    k_free(ptr);
  }

  return new;

error:
  return NULL;
}
#endif /* __ZEPHYR__ */

static void _exit_oom() {
#ifdef EXIT_OOM
  exit(EXIT_OOM);
#else
  exit(EXIT_FAILURE);
#endif
}

void* _malloc_(size_t size, char* file, const char* func, int line) {
#ifdef __ZEPHYR__
  void* ptr = k_malloc(size);
#else
  void* ptr = malloc(size);
#endif
  if (size && !ptr) {
    in3_log(LOG_FATAL, file, func, line, "Failed to allocate memory!\n");
    _exit_oom();
  }
  return ptr;
}

#ifndef TEST
void* _calloc_(size_t n, size_t size, char* file, const char* func, int line) {
#ifdef __ZEPHYR__
  void* ptr = k_calloc(n, size);
#else
  void* ptr = calloc(n, size);
#endif
  if (n && size && !ptr) {
    in3_log(LOG_FATAL, file, func, line, "Failed to allocate memory!\n");
    _exit_oom();
  }
  return ptr;
}
#endif

void* _realloc_(void* ptr, size_t size, size_t oldsize, char* file, const char* func, int line) {
#ifdef __ZEPHYR__
  ptr = k_realloc(ptr, size, oldsize);
#else
  UNUSED_VAR(oldsize);
  ptr = realloc(ptr, size);
#endif
  if (size && !ptr) {
    in3_log(LOG_FATAL, file, func, line, "Failed to allocate memory!\n");
    _exit_oom();
  }
  return ptr;
}

void _free_(void* ptr) {
#ifdef __ZEPHYR__
  k_free(ptr);
#else
  free(ptr);
#endif
}

#ifdef TEST

static int mem_count = 0;

void* t_malloc(size_t size, char* file, const char* func, int line) {
  mem_count++;
  return _malloc_(size, file, func, line);
}

void* t_calloc(size_t n, size_t size, char* file, const char* func, int line) {
  void* ptr = t_malloc(n * size, file, func, line);
  memset(ptr, 0, n * size);
  return ptr;
}

void t_free(void* ptr, char* file, const char* func, int line) {
  UNUSED_VAR(file);
  UNUSED_VAR(func);
  UNUSED_VAR(line);

  if (!ptr) return;
  mem_count--;

  //  printf("freeing a pointer which was not allocated anymore %s : %s : %i\n", file, func, line);
  _free_(ptr);
}
void* t_realloc(void* ptr, size_t size, size_t oldsize, char* file, const char* func, int line) {
  //  if (ptr == NULL)
  //    printf("trying to free a null-pointer in %s : %s : %i\n", file, func, line);

  return _realloc_(ptr, size, oldsize, file, func, line);
}

int mem_get_memleak_cnt() {
  return mem_count;
}

void mem_reset() {
  mem_count = 0;
}

#endif /* TEST */
