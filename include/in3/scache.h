// @PUBLIC_HEADER
/** @file
 * util helper on byte arrays.
 * */

#include "bytes.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#ifndef UTIL_SCACHE_H
#define UTIL_SCACHE_H

typedef struct cache_entry {
  bytes_t             key;
  bytes_t             value;
  uint8_t             must_free;
  uint8_t             buffer[4]; // the buffer is used to store extra data, which will be cleaned when freed.
  struct cache_entry* next;
} cache_entry_t;

bytes_t*       in3_cache_get_entry(cache_entry_t* cache, bytes_t* key);
cache_entry_t* in3_cache_add_entry(cache_entry_t* cache, bytes_t key, bytes_t value);
void           in3_cache_free(cache_entry_t* cache);

#endif
