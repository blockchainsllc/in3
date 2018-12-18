
#ifndef __MEM_H__
#define __MEM_H__

#ifdef ZEPHYR
#include <zephyr.h>

#define _malloc(s) k_malloc(s)
#define _calloc(n,s) k_calloc(n,s)
#define _free(p) k_free(p)
#define _time() k_uptime_get()
#define _time_t uint64_t
#define _atol(p) atoi(p)
#define _srand(p) ;
#define _rand() (uint32_t) k_uptime_get()

#else


#define _malloc(s) malloc(s)
#define _calloc(n,s) calloc(n,s)
#define _free(p) free(p)
#define _time() time(0)
#define _time_t time_t
#define _atol(p) atol(p)
#define _srand(p) srand(p)
#define _rand() (uint32_t) rand()

#endif

void *k_realloc(void *ptr, size_t size, size_t oldsize);

#endif