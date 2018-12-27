#include <string.h>
#ifndef __MEM_H__
#define __MEM_H__

#ifndef UNUSED
 #define UNUSED(x) (void)(x)
#endif
#ifdef __ZEPHYR__
#include <zephyr.h>

void *k_realloc(void *ptr, size_t size, size_t oldsize);

#define _malloc(s) k_malloc(s)
#define _calloc(n,s) k_calloc(n,s)
#define _free(p) k_free(p)
#define _realloc(p,s,o) k_realloc(p,s,o); 

#define _time() k_uptime_get()
#define _time_t uint64_t
#define _atol(p) atoi(p)
#define _srand(p) ;
#define _rand() (uint32_t) k_uptime_get()

#else

#ifdef TEST
size_t mem_get_max_heap();
void* t_malloc(size_t size, char *file, const char *func, int line);
void* t_realloc(void* ptr,size_t size, char *file, const char *func, int line);
void* t_calloc(size_t n,size_t size, char *file, const char *func, int line);
void t_free(void* ptr, char *file, const char *func, int line);
int mem_get_memleak_cnt();
void mem_reset(int cnt);

#define _malloc(s) t_malloc(s,__FILE__, __func__, __LINE__)
#define _calloc(n,s) t_calloc(n,s,__FILE__, __func__, __LINE__)
#define _free(p) t_free(p,__FILE__, __func__, __LINE__)
#define _realloc(p,s,o) t_realloc(p,s,__FILE__, __func__, __LINE__); 

#else

#define _malloc(s) malloc(s)
#define _calloc(n,s) calloc(n,s)
#define _free(p) free(p)
#define _realloc(p,s,o) realloc(p,s); 

#endif


#define _time() time(0)
#define _time_t time_t
#define _atol(p) atol(p)
#define _srand(p) srand(p)
#define _rand() (uint32_t) rand()

#endif

#endif