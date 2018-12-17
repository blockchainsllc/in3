
#ifndef __MEM_H__
#define __MEM_H__

#ifdef ZEPHYR
#include <zephyr.h>

#define _malloc(s) k_malloc(s)
#define _calloc(n,s) k_calloc(n,s)
#define _free(p) k_free(p)

#else

#define _malloc(s) malloc(s)
#define _calloc(n,s) calloc(n,s)
#define _free(p) free(p)

#endif

void *k_realloc(void *ptr, size_t size, size_t oldsize);

#endif