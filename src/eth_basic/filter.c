#include "filter.h"
#include <client/context.h>
#include <client/keys.h>
#include <stdio.h>

static void filter_release_opt(in3_filter_opt_t* fopt) {
  if (fopt) {
    free(fopt->from_block);
    free(fopt->to_block);
    free(fopt->addresses);
    free(fopt->topics);
  }
  _free(fopt);
}

static void filter_release(in3_filter_t* f) {
  if (f && f->options) f->options->release(f->options);
  _free(f);
}

in3_filter_opt_t* filter_new_opt() {
  in3_filter_opt_t* fopt = _calloc(1, sizeof *fopt);
  if (fopt) {
    fopt->release   = filter_release_opt;
    fopt->addresses = NULL;
    fopt->topics    = NULL;
  }
  return fopt;
}

in3_filter_t* filter_new(in3_filter_type_t ft) {
  in3_filter_t* f = _malloc(sizeof *f);
  if (f) {
    f->type       = ft;
    f->release    = filter_release;
    f->last_block = 0;
  }
  return f;
}

size_t filter_add(in3_t* in3, in3_filter_type_t type, in3_filter_opt_t* options) {
  if (type == FILTER_PENDING) {
    // printf("Pending Transactions are not supported!");
    return 0;
  }

  in3_ctx_t* ctx = in3_client_rpc_ctx(in3, "eth_blockNumber", "[]");
  if (ctx->error || !ctx->responses || !ctx->responses[0] || !d_get(ctx->responses[0], K_RESULT)) {
    free_ctx(ctx);
    return 0;
  }
  uint64_t current_block = d_get_longk(ctx->responses[0], K_RESULT);
  free_ctx(ctx);

  in3_filter_t* f = filter_new(type);
  f->options      = options;
  f->last_block   = current_block;

  // Reuse filter ids that have been uninstalled
  // Note: filter ids are 1 indexed, and the associated in3_filter_t object is stored
  // at pos (id - 1) internally in in3->filters->array
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

bool filter_remove(in3_t* in3, size_t id) {
  in3_filter_handler_t* fh = in3->filters;
  if (id == 0 || id > fh->count) return false;

  // We don't realloc the array here, instead we simply set this slot to NULL to indicate
  // that it has been removed and reuse it in add_filter()
  in3_filter_t* f = fh->array[id - 1];
  f->release(f);
  fh->array[id - 1] = NULL;
  return true;
}

bool filter_valid_addrs(d_token_t* addr) {
  if (d_type(addr) == T_BYTES && d_len(addr) == 20)
    return true;
  else if (d_type(addr) != T_ARRAY)
    return false;

  int len = d_len(addr);
  for (int i = 0; i < len; i++, addr = d_next(addr))
    if (d_type(addr) != T_BYTES || d_len(addr) != 20)
      return false;
  return true;
}

bool filter_valid_topics(d_token_t* topics) {
  if (d_type(topics) == T_BYTES && d_len(topics) == 32)
    return true;
  else if (!topics || d_type(topics) != T_ARRAY)
    return false;

  for (d_iterator_t it1 = d_iter(topics); it1.left; d_iter_next(&it1)) {
    if (d_type(it1.token) == T_BYTES && d_len(it1.token) == 32)
      continue;
    else if (d_type(it1.token) == T_NULL)
      continue;
    else if (d_type(it1.token) == T_ARRAY) {
      d_token_t* t = it1.token;
      for (d_iterator_t it2 = d_iter(t); it2.left; d_iter_next(&it2)) {
        if (d_type(it2.token) == T_BYTES && d_len(it2.token) == 32)
          continue;
        else if (d_type(it2.token) == T_NULL)
          continue;
        else
          return false;
      }
    } else
      return false;
  }
  return true;
}
