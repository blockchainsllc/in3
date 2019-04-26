#include "eth_api.h"
#include <stdio.h>

static void release_filter_opt(in3_filter_opt_t* fopt) {
  _free(fopt->topics);
  _free(fopt);
}

static void release_filter(in3_filter_t* f) {
  in3_filter_opt_t* fopt = f->options;
  fopt->release(fopt);
  _free(f);
}

static bool add_topic_to_fopt(in3_filter_opt_t* fopt, uint32_t topic) {
  uint32_t* t  = fopt->topics;
  uint32_t* t_ = _realloc(t, sizeof(*t) * (fopt->topic_count + 1), sizeof(*t) * (fopt->topic_count));
  if (t_ == NULL) {
    return false;
  }
  t                    = t_;
  t[fopt->topic_count] = topic;
  return true;
}

in3_filter_opt_t* new_filter_opt() {
  in3_filter_opt_t* fopt = _calloc(1, sizeof *fopt);
  if (fopt) {
    fopt->add_topic = add_topic_to_fopt;
    fopt->release   = release_filter_opt;
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
  in3_filter_handler_t* fh = in3->filters;
  if (type == FILTER_PENDING) {
    printf("Pending Transactions are not supported!");
    return 0;
  }

  in3_filter_t** arr_ = _realloc(fh->array, sizeof(in3_filter_t) * (fh->count + 1), sizeof in3_filter_t * (fh->count));
  if (arr_ == NULL) {
    return 0;
  }
  fh->array            = arr_;
  in3_filter_t* f      = new_filter(type);
  f->options           = options;
  f->last_block        = eth_blockNumber(in3);
  fh->array[fh->count] = f;
  fh->count += 1;
  return fh->count;
}

static bool remove_filter(in3_t* in3, size_t id) {
  in3_filter_handler_t* fh = in3->filters;
  if (id == 0 || id > fh->count) return false;

  in3_filter_t* f = fh->array[id - 1];
  f->release(f);
  f = NULL;
  for (size_t i = id; i < fh->count; i++)
    fh->array[i] = fh->array[i + 1];

  fh->array = _realloc(fh->array, sizeof(in3_filter_t) * (fh->count - 1), sizeof in3_filter_t * (fh->count));
  fh->count -= 1;
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
