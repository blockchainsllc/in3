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

#ifndef __MEM_H__
#define __MEM_H__

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <malloc.h> // alloca
#else
#ifndef __ZEPHYR__
#include <alloca.h> // alloca
#endif
#endif

#ifdef _MSC_VER
#define _NOINLINE_ __declspec(noinline)
#else
#define _NOINLINE_ __attribute__((noinline))
#endif
#if defined(__clang_analyzer__) || defined(GCC_ANALYZER)
#define NONULL_FOR(args) __attribute__((nonnull args))
#define NONULL           __attribute__((nonnull))
#define RETURNS_NONULL   __attribute__((returns_nonnull))
#else
#define NONULL_FOR(args)
#define NONULL
#define RETURNS_NONULL
#endif

#ifndef UNUSED_VAR
#define UNUSED_VAR(x) (void) (x)
#endif /* UNUSED_VAR */

#ifdef __ZEPHYR__
#include <zephyr.h>
#define _atol(p) atoi(p)
#define _localtime(b__)                          \
  do {                                           \
    sprintf(b__, "%" PRId32, k_uptime_get_32()); \
  } while (0)
#define fflush(f_) \
  do {             \
  } while (0) // FIXME: Fix for zephyr
#else         /* __ZEPHYR__ */
#define _atol(p) atol(p)
#define _localtime(b__)                                              \
  do {                                                               \
    time_t     t                                    = time(NULL);    \
    struct tm* lt                                   = localtime(&t); \
    b__[strftime(buf, sizeof(buf), "%H:%M:%S", lt)] = '\0';          \
  } while (0)
#endif /* __ZEPHYR__ */

#ifdef TEST
#define _malloc(s)        t_malloc(s, __FILE__, __func__, __LINE__)
#define _calloc(n, s)     t_calloc(n, s, __FILE__, __func__, __LINE__)
#define _free(p)          t_free(p, __FILE__, __func__, __LINE__)
#define _realloc(p, s, o) t_realloc(p, s, o, __FILE__, __func__, __LINE__)
void* t_malloc(size_t size, char* file, const char* func, int line);
void* t_realloc(void* ptr, size_t size, size_t oldsize, char* file, const char* func, int line);
void* t_calloc(size_t n, size_t size, char* file, const char* func, int line);
void  t_free(void* ptr, char* file, const char* func, int line);
int   mem_get_memleak_cnt();
void  mem_reset();
#else /* TEST */
#ifdef LOGGING
#define _malloc(s)        _malloc_(s, __FILE__, __func__, __LINE__)
#define _calloc(n, s)     _calloc_(n, s, __FILE__, __func__, __LINE__)
#define _free(p)          _free_(p)
#define _realloc(p, s, o) _realloc_(p, s, o, __FILE__, __func__, __LINE__)
#else
#define _malloc(s)        _malloc_(s, "F", "F", 0)
#define _calloc(n, s)     _calloc_(n, s, "F", "F", 0)
#define _free(p)          _free_(p)
#define _realloc(p, s, o) _realloc_(p, s, o, "F", "F", 0)
#endif
RETURNS_NONULL void* _malloc_(size_t size, char* file, const char* func, int line);
RETURNS_NONULL void* _realloc_(void* ptr, size_t size, size_t oldsize, char* file, const char* func, int line);
RETURNS_NONULL void* _calloc_(size_t n, size_t size, char* file, const char* func, int line);
void                 _free_(void* ptr);
#endif /* TEST */

// these function are used for cleaning up allocated memory
typedef void (*cleanup_t)(void* data);
typedef struct tmp_mem {
  void*           ptr;
  cleanup_t       fn;
  struct tmp_mem* next;
} safe_mem_t;

static inline void* _tmp_add(void* ptr, cleanup_t fn, safe_mem_t** list, safe_mem_t* next) {
  if (!ptr) return NULL;
  next->ptr  = ptr;
  next->next = *list;
  next->fn   = fn;
  *list      = next;
  return ptr;
}
#define SAFE_DEFER(ptr, fn) _tmp_add(ptr, fn, &_tmp_mem_list, alloca(sizeof(safe_mem_t)))
#define SAFE_INIT           safe_mem_t* _tmp_mem_list = NULL;
#define SAFE_MALLOC(size)   (size > 100 ? _tmp_add(_malloc(size), free, &_tmp_mem_list, alloca(sizeof(safe_mem_t))) : alloca(size))
#define SAFE_RETURN(val)                                                                  \
  {                                                                                       \
    for (; _tmp_mem_list; _tmp_mem_list = _tmp_mem_list->next) _free(_tmp_mem_list->ptr); \
    return val;                                                                           \
  }
#define TRY_SAFE(exp)                \
  {                                  \
    in3_ret_t r = (exp);             \
    if (r != IN3_OK) SAFE_RETURN(r); \
  }
/*
in3_ret_t tf() {
  SAFE_INIT;
  json_ctx* ctx = parse_json(data);
  SAFE_DEFER(ctx, json_free);
  if (!ctx || d_type(cxt->result)!=T_ARRAY) SAFE_RETURN(IN3_EINVAL);

  SAFE_RETURN();
}
*/

// size_t _strnlen(const char* str, size_t maxlen);
#define _strnlen   strnlen
#define _strlen(d) strnlen(d, 0xFFFFFFFF)

#endif /* __MEM_H__ */