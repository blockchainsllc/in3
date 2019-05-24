#include "scache.h"
#include "mem.h"

bytes_t* in3_cache_get_entry(cache_entry_t* cache, bytes_t* key) {
  while (cache) {
    if (cache->key.data && b_cmp(key, &cache->key)) return &cache->value;
    cache = cache->next;
  }
  return NULL;
}
void in3_cache_free(cache_entry_t* cache) {
  cache_entry_t* p = NULL;
  while (cache) {
    if (cache->key.data) _free(cache->key.data);
    if (cache->must_free)
      _free(cache->value.data);
    p     = cache;
    cache = cache->next;
    _free(p);
  }
}

cache_entry_t* in3_cache_add_entry(cache_entry_t* cache, bytes_t key, bytes_t value) {
  cache_entry_t* entry = _malloc(sizeof(cache_entry_t));
  entry->key           = key;
  entry->value         = value;
  entry->must_free     = 1;
  entry->next          = cache;
  return entry;
}
