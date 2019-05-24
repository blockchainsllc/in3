#include "cache.h"
#include "../util/mem.h"
#include "../util/utils.h"
#include "context.h"
#include "nodelist.h"
#include "stdio.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#define NODE_LIST_KEY ("nodelist_%" PRIx64)

int in3_cache_init(in3_t* c) {
  int i;
  // the reason why we ignore the result here, is because we want to ignore errors if the cache is able to update.
  for (i = 0; i < c->chainsCount; i++)
    in3_cache_update_nodelist(c, c->chains + i);

  return 0;
}

int in3_cache_update_nodelist(in3_t* c, in3_chain_t* chain) {
  // it is ok not to have a storage
  if (!c->cacheStorage) return 0;

  // define the key to use
  char key[200];
  sprintf(key, NODE_LIST_KEY, (unsigned long long) chain->chainId);

  // get from cache
  bytes_t* b = c->cacheStorage->get_item(c->cacheStorage->cptr, key);
  if (b) {
    int    i, count;
    size_t p = 0;

    // version check
    if (b_read_byte(b, &p) != 1) {
      b_free(b);
      return -1;
    }

    count = b_read_int(b, &p);

    in3_node_t* nl = _calloc(count, sizeof(in3_node_t));
    if (nl == NULL) {
      b_free(b);
      return -1;
    }
    in3_node_weight_t* nw = _calloc(count, sizeof(in3_node_weight_t));
    if (nw == NULL) {
      b_free(b);
      _free(nl);
      return -1;
    }

    // clean up old
    in3_nodelist_clear(chain);

    // fill data
    chain->contract       = b_new_fixed_bytes(b, &p, 20);
    chain->lastBlock      = b_read_long(b, &p);
    chain->nodeListLength = count;
    chain->nodeList       = nl;
    chain->weights        = nw;
    chain->needsUpdate    = false;
    memcpy(chain->weights, b->data + p, count * sizeof(in3_node_weight_t));
    p += count * sizeof(in3_node_weight_t);

    for (i = 0; i < count; i++) {
      in3_node_t* n = chain->nodeList + i;
      n->capacity   = b_read_int(b, &p);
      n->index      = b_read_int(b, &p);
      n->deposit    = b_read_long(b, &p);
      n->props      = b_read_long(b, &p);
      n->address    = b_new_fixed_bytes(b, &p, 20);
      n->url        = b_new_chars(b, &p);
    }
    b_free(b);
  }
  return 0;
}

int in3_cache_store_nodelist(in3_ctx_t* ctx, in3_chain_t* chain) {
  int i;

  // write to bytes_buffer
  bytes_builder_t* bb = bb_new();
  bb_write_byte(bb, 1);                      // Version flag
  bb_write_fixed_bytes(bb, chain->contract); // 20 bytes fixed
  bb_write_long(bb, chain->lastBlock);
  bb_write_int(bb, chain->nodeListLength);
  bb_write_raw_bytes(bb, chain->weights, chain->nodeListLength * sizeof(in3_node_weight_t));

  for (i = 0; i < chain->nodeListLength; i++) {
    in3_node_t* n = chain->nodeList + i;
    bb_write_int(bb, n->capacity);
    bb_write_int(bb, n->index);
    bb_write_long(bb, n->deposit);
    bb_write_long(bb, n->props);
    bb_write_fixed_bytes(bb, n->address);
    bb_write_chars(bb, n->url, strlen(n->url));
  }

  // create key
  char key[200];
  sprintf(key, NODE_LIST_KEY, (unsigned long long) chain->chainId);

  // store it and ignore return value since failing when writing cache should not stop us.
  ctx->client->cacheStorage->set_item(ctx->client->cacheStorage->cptr, key, &bb->b);

  // clear buffer
  bb_free(bb);
  return 0;
}
