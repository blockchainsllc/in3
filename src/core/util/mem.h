#include <stdio.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

#ifndef __MEM_H__
#define __MEM_H__

#ifndef UNUSED_VAR
#define UNUSED_VAR(x) (void) (x)
#endif /* UNUSED_VAR */

#ifdef __ZEPHYR__
#include <zephyr.h>
#define _time() k_uptime_get()
#define _time_t uint64_t
#define _atol(p) atoi(p)
#define _srand(p) ;
#define _rand() (uint32_t) k_uptime_get()
#define _localtime(b__)                          \
  do {                                           \
    sprintf(b__, "%" PRId32, k_uptime_get_32()); \
  } while (0)
#else /* __ZEPHYR__ */
#define _time() time(0)
#define _time_t time_t
#define _atol(p) atol(p)
#define _srand(p) srand(p)
#define _rand() (uint32_t) rand()
#define _localtime(b__)                                              \
  do {                                                               \
    time_t     t                                    = time(NULL);    \
    struct tm* lt                                   = localtime(&t); \
    b__[strftime(buf, sizeof(buf), "%H:%M:%S", lt)] = '\0';          \
  } while (0)
#endif /* __ZEPHYR__ */

#ifdef TEST
#define _malloc(s) t_malloc(s, __FILE__, __func__, __LINE__)
#define _calloc(n, s) t_calloc(n, s, __FILE__, __func__, __LINE__)
#define _free(p) t_free(p, __FILE__, __func__, __LINE__)
#define _realloc(p, s, o) t_realloc(p, s, o, __FILE__, __func__, __LINE__)
size_t mem_get_max_heap();
void*  t_malloc(size_t size, char* file, const char* func, int line);
void*  t_realloc(void* ptr, size_t size, size_t oldsize, char* file, const char* func, int line);
void*  t_calloc(size_t n, size_t size, char* file, const char* func, int line);
void   t_free(void* ptr, char* file, const char* func, int line);
int    mem_get_memleak_cnt();
void   mem_reset(int cnt);
void   memstack();
int    mem_stack_size();
#else /* TEST */
#define _malloc(s) _malloc_(s, __FILE__, __func__, __LINE__)
#define _calloc(n, s) _calloc_(n, s, __FILE__, __func__, __LINE__)
#define _free(p) _free_(p)
#define _realloc(p, s, o) _realloc_(p, s, o, __FILE__, __func__, __LINE__)
void* _malloc_(size_t size, char* file, const char* func, int line);
void* _realloc_(void* ptr, size_t size, size_t oldsize, char* file, const char* func, int line);
void* _calloc_(size_t n, size_t size, char* file, const char* func, int line);
void  _free_(void* ptr);
#endif /* TEST */

#endif /* __MEM_H__ */