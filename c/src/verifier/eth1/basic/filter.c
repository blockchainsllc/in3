/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 * 
 * 
 * COMMERCIAL LICENSE USAGE
 * 
 * Licensees holding a valid commercial license may use this file in accordance 
 * with the commercial license agreement provided with the Software or, alternatively, 
 * in accordance with the terms contained in a written agreement between you and 
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
 * information please contact slock.it at in3@slock.it.
 * 	
 * Alternatively, this file may be used under the AGPL license as follows:
 *    
 * AGPL LICENSE USAGE
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

#include "filter.h"
#include "../../../core/client/context_internal.h"
#include "../../../core/client/keys.h"
#include "../../../core/util/log.h"
#include "../../../core/util/mem.h"
#include <inttypes.h>
#include <stdio.h>

static bool filter_addrs_valid(d_token_t* addr) {
  if (d_type(addr) == T_BYTES && d_len(addr) == 20)
    return true;
  else if (d_type(addr) != T_ARRAY)
    return false;

  int len = d_len(addr);
  addr += 1;
  for (int i = 0; i < len; i++, addr = d_next(addr))
    if (d_type(addr) != T_BYTES || d_len(addr) != 20)
      return false;
  return true;
}

static bool filter_topics_valid(d_token_t* topics) {
  if (!topics || d_type(topics) != T_ARRAY)
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

bool filter_opt_valid(d_token_t* tx_params) {
  d_token_t* frmblk = d_get(tx_params, K_FROM_BLOCK);
  if (!frmblk) { /* Optional */
  } else if (d_type(frmblk) == T_INTEGER || d_type(frmblk) == T_BYTES) {
  } else if (d_type(frmblk) == T_STRING && (!strcmp(d_string(frmblk), "latest") || !strcmp(d_string(frmblk), "earliest") || !strcmp(d_string(frmblk), "pending"))) {
  } else
    return false;

  d_token_t* toblk = d_get(tx_params, K_TO_BLOCK);
  if (!toblk) { /* Optional */
  } else if (d_type(toblk) == T_INTEGER || d_type(toblk) == T_BYTES) {
  } else if (d_type(toblk) == T_STRING && (!strcmp(d_string(toblk), "latest") || !strcmp(d_string(toblk), "earliest") || !strcmp(d_string(toblk), "pending"))) {
  } else
    return false;

  d_token_t* blockhash = d_getl(tx_params, K_BLOCK_HASH, 32);
  if (blockhash == NULL) { /* Optional */
  } else if ((d_type(blockhash) == T_BYTES && d_len(blockhash) == 32) && !frmblk && !toblk) {
    /* If blockHash is present, then neither fromBlock nor toBlock are allowed. */
  } else
    return false;

  d_token_t* addrs = d_getl(tx_params, K_ADDRESS, 20);
  if (addrs == NULL) { /* Optional */
  } else if (filter_addrs_valid(addrs)) {
  } else
    return false;

  d_token_t* topics = d_get(tx_params, K_TOPICS);
  if (topics == NULL) { /* Optional */
  } else if (filter_topics_valid(topics)) {
  } else
    return false;

  return true;
}

char* filter_opt_set_fromBlock(char* fopt, uint64_t fromBlock, bool should_overwrite) {
  size_t pos, len;
  char   blockstr[40]; // buffer to hold - "fromBlock": "<21 chars for hex repr (upto UINT64_MAX)>",
  char*  tok = str_find(fopt, "\"fromBlock\"");
  if (!tok) {
    sprintf(blockstr, "\"fromBlock\":\"0x%" PRIx64 "\"%c", fromBlock, str_find(fopt, "\"") ? ',' : '\0');
    tok = str_find(fopt, "{");
    pos = fopt - tok + 1;
    len = 0;
    return str_replace_pos(fopt, pos, len, blockstr);
  } else if (should_overwrite) {
    sprintf(blockstr, "0x%" PRIx64 "", fromBlock);
    tok = str_find(str_find(tok + 1, ":") + 1, "\"");
    pos = tok - fopt + 1;
    tok = str_find(tok + 1, "\"");
    len = tok - fopt - pos;
    return str_replace_pos(fopt, pos, len, blockstr);
  }

  char* tmp = _malloc(strlen(fopt) + 1);
  strcpy(tmp, fopt);
  return tmp;
}

static void filter_release(in3_filter_t* f) {
  if (f && f->options)
    _free(f->options);
  _free(f);
}

static in3_filter_t* filter_new(in3_filter_type_t ft) {
  in3_filter_t* f = _malloc(sizeof *f);
  if (f) {
    f->type           = ft;
    f->release        = filter_release;
    f->last_block     = 0;
    f->is_first_usage = true;
  }
  return f;
}

in3_ret_t filter_add(in3_t* in3, in3_filter_type_t type, char* options) {
  if (type == FILTER_PENDING)
    return IN3_ENOTSUP;
  else if (options == NULL && type != FILTER_BLOCK)
    return IN3_EINVAL;

  in3_ret_t  res = IN3_OK;
  in3_ctx_t* ctx = in3_client_rpc_ctx(in3, "eth_blockNumber", "[]");
  if (IN3_OK != (res = ctx_get_error(ctx, 0))) {
    ctx_free(ctx);
    return res;
  }
  uint64_t current_block = d_get_longk(ctx->responses[0], K_RESULT);
  ctx_free(ctx);

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
  in3_filter_t** arr_;
  if (fh->array)
    arr_ = _realloc(fh->array, sizeof(in3_filter_t*) * (fh->count + 1), sizeof(in3_filter_t*) * (fh->count));
  else
    arr_ = _malloc(sizeof(in3_filter_t*) * (fh->count + 1));

  if (arr_ == NULL) {
    return IN3_ENOMEM;
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
  if (!f) return false;
  f->release(f);
  in3->filters->array[id - 1] = NULL;
  return true;
}

in3_ret_t filter_get_changes(in3_ctx_t* ctx, size_t id, sb_t* result) {
  in3_t* in3 = ctx->client;
  if (in3->filters == NULL)
    return ctx_set_error(ctx, "no filters found", IN3_EUNKNOWN);
  if (id == 0 || id > in3->filters->count)
    return ctx_set_error(ctx, "filter with id does not exist", IN3_EUNKNOWN);

  in3_ctx_t* ctx_ = in3_client_rpc_ctx(in3, "eth_blockNumber", "[]");
  in3_ret_t  res  = ctx_get_error(ctx_, 0);
  if (res != IN3_OK) {
    ctx_set_error(ctx, ctx_->error, res);
    ctx_free(ctx_);
    return ctx_set_error(ctx, "internal error, call to eth_blockNumber failed", res);
  }
  uint64_t blkno = d_get_longk(ctx_->responses[0], K_RESULT);
  ctx_free(ctx_);

  in3_filter_t* f = in3->filters->array[id - 1];
  if (!f)
    return ctx_set_error(ctx, "filter with id does not exist", IN3_EUNKNOWN);

  char* fopt = f->options;
  switch (f->type) {
    case FILTER_EVENT: {
      if (f->last_block > blkno) {
        sb_add_chars(result, "[]");
      } else {
        sb_t* params = sb_new("[");
        char* fopt_  = filter_opt_set_fromBlock(fopt, f->last_block, !f->is_first_usage);
        sb_add_chars(params, fopt_);
        ctx_ = in3_client_rpc_ctx(in3, "eth_getLogs", sb_add_char(params, ']')->data);
        sb_free(params);
        _free(fopt_);
        if ((res = ctx_get_error(ctx_, 0)) != IN3_OK) {
          ctx_set_error(ctx, ctx_->error, res);
          ctx_free(ctx_);
          return ctx_set_error(ctx, "internal error, call to eth_getLogs failed", res);
        }
        d_token_t* r  = d_get(ctx_->responses[0], K_RESULT);
        char*      jr = d_create_json(r);
        sb_add_chars(result, jr);
        _free(jr);
        ctx_free(ctx_);
        f->last_block     = blkno + 1;
        f->is_first_usage = false;
      }
      return IN3_OK;
    }
    case FILTER_BLOCK:
      if (blkno > f->last_block) {
        char params[37] = {0};
        sb_add_char(result, '[');
        for (uint64_t i = f->last_block + 1, j = 0; i <= blkno; i++, j++) {
          sprintf(params, "[\"0x%" PRIx64 "\", false]", i);
          ctx_ = in3_client_rpc_ctx(in3, "eth_getBlockByNumber", params);
          if ((res = ctx_get_error(ctx_, 0)) != IN3_OK) {
            ctx->verification_state = res;
            // error or block doesn't exist (unlikely)
            in3_log_warn("Failed to get block by number!\n");
            continue;
          }
          d_token_t* hash  = d_getl(d_get(ctx_->responses[0], K_RESULT), K_HASH, 32);
          char       h[67] = "0x";
          bytes_to_hex(d_bytes(hash)->data, 32, h + 2);
          if (j != 0)
            sb_add_char(result, ',');
          sb_add_char(result, '"');
          sb_add_chars(result, h);
          sb_add_char(result, '"');
          ctx_free(ctx_);
        }
        sb_add_char(result, ']');
        f->last_block = blkno;
        return IN3_OK;
      } else {
        sb_add_chars(result, "[]");
        return IN3_OK;
      }
    default:
      return ctx_set_error(ctx, "unsupported filter type", IN3_ENOTSUP);
  }
  return IN3_OK;
}