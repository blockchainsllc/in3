#include "filter.h"
#include <client/context.h>
#include <client/keys.h>
#include <inttypes.h>
#include <stdio.h>

static bool filter_addrs_valid(d_token_t* addr) {
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

static bool filter_topics_valid(d_token_t* topics) {
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

int filter_opt_from_json(in3_filter_opt_t** fopt, d_token_t* tx_params) {
  d_token_t* frmblk = d_get(tx_params, K_FROM_BLOCK);
  if (!frmblk) { /* Optional */
  } else if (d_type(frmblk) == T_INTEGER || d_type(frmblk) == T_BYTES) {
  } else if (d_type(frmblk) == T_STRING && (!strcmp(d_string(frmblk), "latest") || !strcmp(d_string(frmblk), "earliest") || !strcmp(d_string(frmblk), "pending"))) {
  } else {
    return -1;
  }

  d_token_t* toblk = d_get(tx_params, K_TO_BLOCK);
  if (!toblk) { /* Optional */
  } else if (d_type(toblk) == T_INTEGER || d_type(toblk) == T_BYTES) {
  } else if (d_type(toblk) == T_STRING && (!strcmp(d_string(toblk), "latest") || !strcmp(d_string(toblk), "earliest") || !strcmp(d_string(toblk), "pending"))) {
  } else {
    return -1;
  }

  d_token_t* addrs = d_get(tx_params, K_ADDRESS);
  if (addrs == NULL) { /* Optional */
  } else if (filter_addrs_valid(addrs)) {
  } else {
    return -1;
  }

  d_token_t* topics = d_get(tx_params, K_TOPICS);
  if (topics == NULL) { /* Optional */
  } else if (filter_topics_valid(topics)) {
  } else {
    return -1;
  }

  *fopt = d_clone(tx_params);
  if (*fopt == NULL) return -1;
  return 0;
}

sb_t* filter_opt_to_json_str(in3_filter_opt_t* fopt, sb_t* sb) {
  if (sb != NULL) {
    if (fopt) {
      char* jfopt = d_create_json(fopt);
      if (jfopt) sb_add_chars(sb, jfopt);
      _free(jfopt);
    }
  }
  return sb;
}

static void filter_release(in3_filter_t* f) {
  if (f && f->options) {
    _free(f->options->data);
    _free(f->options);
  }
  _free(f);
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
  if (in3->filters == NULL)
    in3->filters = _calloc(1, sizeof *(in3->filters));
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
  if (in3->filters == NULL)
    return false;
  if (id == 0 || id > in3->filters->count)
    return false;

  // We don't realloc the array here, instead we simply set this slot to NULL to indicate
  // that it has been removed and reuse it in add_filter()
  in3_filter_t* f = in3->filters->array[id - 1];
  f->release(f);
  in3->filters->array[id - 1] = NULL;
  return true;
}

int filter_get_changes(in3_ctx_t* ctx, size_t id, sb_t* result) {
  in3_t* in3 = ctx->client;
  if (in3->filters == NULL)
    return -1;
  if (id == 0 || id > in3->filters->count)
    return -2;

  in3_ctx_t* ctx_ = in3_client_rpc_ctx(in3, "eth_blockNumber", "[]");
  if (ctx_->error || !ctx_->responses || !ctx_->responses[0] || !d_get(ctx_->responses[0], K_RESULT)) {
    free_ctx(ctx_);
    return ctx_set_error(ctx, "internal error (eth_blockNumber)", -1);
  }
  uint64_t blkno = d_get_longk(ctx_->responses[0], K_RESULT);
  free_ctx(ctx_);

  in3_filter_t*     f    = in3->filters->array[id - 1];
  in3_filter_opt_t* fopt = f->options;
  switch (f->type) {
    case FILTER_EVENT: {
      sb_t* params = sb_new("[");
      params       = filter_opt_to_json_str(fopt, params);
      ctx_         = in3_client_rpc_ctx(in3, "eth_getLogs", sb_add_char(params, ']')->data);
      sb_free(params);
      if (ctx_->error || !ctx_->responses || !ctx_->responses[0] || !d_get(ctx_->responses[0], K_RESULT)) {
        free_ctx(ctx_);
        return ctx_set_error(ctx, "internal error (eth_getLogs)", -1);
      }
      d_token_t* r = d_get(ctx_->responses[0], K_RESULT);
      if (!r) {
        free_ctx(ctx_);
        return ctx_set_error(ctx, "internal error (eth_getLogs)", -1);
      }

      char* jr = d_create_json(r);
      sb_add_chars(result, jr);
      _free(jr);
      free_ctx(ctx_);
      f->last_block = blkno + 1;
      return 0;
    }
    case FILTER_BLOCK:
      if (blkno > f->last_block) {
        char params[37] = {0};
        sb_add_char(result, '[');
        for (uint64_t i = f->last_block + 1, j = 0; i <= blkno; i++, j++) {
          sprintf(params, "[\"0x%" PRIx64 "\", false]", i);
          ctx_ = in3_client_rpc_ctx(in3, "eth_getBlockByNumber", params);
          if (ctx_->error || !ctx_->responses || !ctx_->responses[0] || !d_get(ctx_->responses[0], K_RESULT)) {
            // error or block doesn't exist (unlikely)
            continue;
          }
          d_token_t* res = d_get(ctx_->responses[0], K_RESULT);
          if (res == NULL || d_type(res) == T_NULL) {
            // error or block doesn't exist (unlikely)
            continue;
          }
          d_token_t* hash  = d_get(res, K_HASH);
          char       h[67] = "0x";
          bytes_to_hex(d_bytes(hash)->data, 32, h + 2);
          if (j != 0)
            sb_add_char(result, ',');
          sb_add_char(result, '"');
          sb_add_chars(result, h);
          sb_add_char(result, '"');
          free_ctx(ctx_);
        }
        sb_add_char(result, ']');
        f->last_block = blkno;
        return 0;
      } else {
        sb_add_chars(result, "[]");
        return 0;
      }
    default:
      return ctx_set_error(ctx, "internal error", -1);
  }
  return 0;
}