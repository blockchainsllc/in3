#include "filter.h"
#include <client/context.h>
#include <client/keys.h>
#include <stdio.h>

static void filter_opt_release(in3_filter_opt_t* fopt) {
  if (fopt) {
    _free(fopt->from_block);
    _free(fopt->to_block);
    _free(fopt->addresses);
    _free(fopt->topics);
  }
  _free(fopt);
}

int filter_opt_from_json(struct in3_filter_opt_t_* fopt, d_token_t* tx_params) {
  int        ret = 0;
  char*      from_block;
  d_token_t* frmblk = d_get(tx_params + 1, K_FROM_BLOCK);
  if (!frmblk) {
    from_block = NULL;
  } else if (d_type(frmblk) == T_INTEGER || d_type(frmblk) == T_BYTES) {
    from_block = hexstru64(d_long(frmblk));
  } else if (d_type(frmblk) == T_STRING && (!strcmp(d_string(frmblk), "latest") || !strcmp(d_string(frmblk), "earliest") || !strcmp(d_string(frmblk), "pending"))) {
    from_block = _strdup(d_string(frmblk));
  } else {
    ret = -2;
    goto ERR_FLT;
  }

  char*      to_block;
  d_token_t* toblk = d_get(tx_params + 1, K_TO_BLOCK);
  if (!toblk) {
    to_block = NULL;
  } else if (d_type(toblk) == T_INTEGER || d_type(toblk) == T_BYTES) {
    to_block = hexstru64(d_long(toblk));
  } else if (d_type(toblk) == T_STRING && (!strcmp(d_string(toblk), "latest") || !strcmp(d_string(toblk), "earliest") || !strcmp(d_string(toblk), "pending"))) {
    to_block = _strdup(d_string(toblk));
  } else {
    ret = -2;
    goto ERR_FLT1;
  }

  char*      jaddr;
  d_token_t* addrs = d_get(tx_params + 1, K_ADDRESS);
  if (addrs == NULL) {
    jaddr = NULL;
  } else if (filter_addrs_valid(addrs)) {
    jaddr = d_create_json(addrs);
    if (jaddr == NULL) {
      ret = -1;
      goto ERR_FLT2;
    }
  } else {
    ret = -2;
    goto ERR_FLT2;
  }

  char*      jtopics;
  d_token_t* topics = d_get(tx_params + 1, K_TOPICS);
  if (topics == NULL) {
    jtopics = NULL;
  } else if (filter_topics_valid(topics)) {
    jtopics = d_create_json(topics);
    if (jtopics == NULL) {
      ret = -1;
      goto ERR_FLT3;
    }
  } else {
    ret = -2;
    goto ERR_FLT3;
  }

  if (!fopt) {
    ret = -1;
    goto ERR_FLT4;
  }
  fopt->from_block = from_block;
  fopt->to_block   = to_block;
  fopt->addresses  = jaddr;
  fopt->topics     = jtopics;
  return 0;

ERR_FLT4:
  free(jtopics);
ERR_FLT3:
  free(jaddr);
ERR_FLT2:
  free(to_block);
ERR_FLT1:
  free(from_block);
ERR_FLT:
  return ret;
}

static sb_t* filter_opt_to_json_str(in3_filter_opt_t* fopt, sb_t* sb) {
  if (sb != NULL) {
    sb_add_char(sb, '{');
    sb_add_chars(sb, "\"fromBlock\":\"");
    (fopt->from_block) ? sb_add_chars(sb, fopt->from_block) : sb_add_chars(sb, "latest");
    sb_add_chars(sb, "\",");
    sb_add_chars(sb, "\"toBlock\":\"");
    (fopt->to_block) ? sb_add_chars(sb, fopt->to_block) : sb_add_chars(sb, "latest");
    sb_add_char(sb, '"');

    if (fopt->addresses) {
      sb_add_chars(sb, ",\"address\": ");
      sb_add_chars(sb, fopt->addresses);
    }
    if (fopt->topics) {
      sb_add_chars(sb, ",\"topics\": ");
      sb_add_chars(sb, fopt->topics);
    }
    sb_add_char(sb, '}');
  }
  return sb;
}

static void filter_release(in3_filter_t* f) {
  if (f && f->options) f->options->release(f->options);
  _free(f);
}

in3_filter_opt_t* filter_opt_new() {
  in3_filter_opt_t* fopt = _calloc(1, sizeof *fopt);
  if (fopt) {
    fopt->addresses   = NULL;
    fopt->topics      = NULL;
    fopt->from_json   = filter_opt_from_json;
    fopt->to_json_str = filter_opt_to_json_str;
    fopt->release     = filter_opt_release;
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

bool filter_addrs_valid(d_token_t* addr) {
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

bool filter_topics_valid(d_token_t* topics) {
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
