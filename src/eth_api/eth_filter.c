#include "eth_filter.h"
#include <stdio.h>

static void release_filter_opt(in3_filter_opt_t* fopt) {
  if (fopt) {
    _free(fopt->addresses);
    for (size_t i = 0; i < fopt->topic_count; i++)
      _free(fopt->topics[i]);
    _free(fopt->topics);
  }
  _free(fopt);
}

static void release_filter(in3_filter_t* f) {
  if (f && f->options) f->options->release(f->options);
  _free(f);
}

static bool add_address_to_fopt(in3_filter_opt_t* fopt, address_t address) {
  address_t* a_ = _realloc(fopt->addresses, sizeof(*a_) * (fopt->address_count + 1),
                           sizeof(*a_) * (fopt->address_count));
  if (a_ == NULL) {
    return false;
  }
  fopt->addresses = a_;
  memcpy(fopt->addresses[fopt->address_count], &address, 32);
  fopt->address_count += 1;
  return true;
}

static bool add_topic_to_fopt(in3_filter_opt_t* fopt, bytes32_t* topic) {
  bytes32_t** t_ = _realloc(fopt->topics, fopt->topic_count + 1, fopt->topic_count);
  if (t_ == NULL) {
    return false;
  }
  fopt->topics = t_;
  if (topic != NULL) {
    fopt->topics[fopt->topic_count] = _malloc(sizeof(bytes32_t));
    memcpy(fopt->topics[fopt->topic_count], topic, 32);
  } else {
    fopt->topics[fopt->topic_count] = NULL;
  }
  fopt->topic_count += 1;
  return true;
}

in3_filter_opt_t* new_filter_opt() {
  in3_filter_opt_t* fopt = _calloc(1, sizeof *fopt);
  if (fopt) {
    fopt->add_address = add_address_to_fopt;
    fopt->add_topic   = add_topic_to_fopt;
    fopt->release     = release_filter_opt;
  }
  return fopt;
}

in3_filter_t* new_filter(in3_filter_type_t ft) {
  in3_filter_t* f = _malloc(sizeof *f);
  if (f) {
    f->type       = ft;
    f->release    = release_filter;
    f->last_block = 0;
  }
  return f;
}

static size_t add_filter(in3_t* in3, in3_filter_type_t type, in3_filter_opt_t* options) {
  if (type == FILTER_PENDING) {
    printf("Pending Transactions are not supported!");
    return 0;
  }

  in3_filter_t* f = new_filter(type);
  f->options      = options;
  f->last_block   = eth_blockNumber(in3);

  in3_filter_handler_t* fh = in3->filters;
  for (size_t i = 0; i < fh->count; i++) {
    if (fh->array[i] == NULL) {
      fh->array[i] = f;
      return i + 1;
    }
  }

  in3_filter_t** arr_ = _realloc(fh->array, sizeof(in3_filter_t*) * (fh->count + 1),
                                 sizeof(in3_filter_t*) * (fh->count));
  if (arr_ == NULL) {
    return 0;
  }
  fh->array            = arr_;
  fh->array[fh->count] = f;
  fh->count += 1;
  return fh->count;
}

static bool remove_filter(in3_t* in3, size_t id) {
  in3_filter_handler_t* fh = in3->filters;
  if (id == 0 || id > fh->count) return false;

  in3_filter_t* f = fh->array[id - 1];
  f->release(f);
  fh->array[id - 1] = NULL;
  return true;
}

size_t eth_newFilter(in3_t* in3, in3_filter_opt_t* options) {
  return add_filter(in3, FILTER_EVENT, options);
}

size_t eth_newBlockFilter(in3_t* in3, in3_filter_opt_t* options) {
  return add_filter(in3, FILTER_BLOCK, options);
}

size_t eth_newPendingTransactionFilter(in3_t* in3, in3_filter_opt_t* options) {
  return add_filter(in3, FILTER_PENDING, options);
}

bool eth_uninstallFilter(in3_t* in3, size_t id) {
  return remove_filter(in3, id);
}

int eth_getFilterChanges(in3_t* in3, size_t id, bytes32_t** block_hashes, eth_log_t** logs) {
  if (id == 0 || id > in3->filters->count)
    return -1;

  uint64_t      blkno = eth_blockNumber(in3);
  in3_filter_t* f     = in3->filters->array[id - 1];
  switch (f->type) {
    case FILTER_EVENT:
      *logs = eth_getLogs(in3, f->options);
      return 0;
    case FILTER_BLOCK:
      if (blkno > f->last_block) {
        block_hashes = malloc(sizeof(bytes32_t*) * (blkno - f->last_block));
        for (uint64_t i = f->last_block + 1, j = 0; i <= blkno; i++, j++) {
          eth_block_t* blk = eth_getBlockByNumber(in3, i, false);
          memcpy(block_hashes[j], blk->hash, 32);
        }
        return (blkno - f->last_block);
      } else {
        *block_hashes = NULL;
        return 0;
        default:
          return -2;
      }
  }
}
