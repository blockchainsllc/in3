/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
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
#include "../../../core/client/keys.h"
#include "../../../core/client/request_internal.h"
#include "../../../core/util/log.h"
#include "../../../core/util/mem.h"
#include "eth_basic.h"
#include <inttypes.h>
#include <stdio.h>

static bool filter_addrs_valid(d_token_t* addr) {
  bytes_t b = d_bytes(addr);
  if (b.len == 20)
    return true;
  else if (d_type(addr) != T_ARRAY)
    return false;

  for_children_of(it, addr) {
    if (d_bytes(it.token).len != 20) return false;
  }
  return true;
}

static bool filter_topics_valid(d_token_t* topics) {
  if (!topics || d_type(topics) != T_ARRAY)
    return false;

  for (d_iterator_t it1 = d_iter(topics); it1.left; d_iter_next(&it1)) {
    if (d_is_bytes(it1.token) && d_bytes(it1.token).len == 32)
      continue;
    else if (d_type(it1.token) == T_NULL)
      continue;
    else if (d_type(it1.token) == T_ARRAY) {
      d_token_t* t = it1.token;
      for (d_iterator_t it2 = d_iter(t); it2.left; d_iter_next(&it2)) {
        if (d_is_bytes(it2.token) && d_bytes(it2.token).len == 32)
          continue;
        else if (d_type(it2.token) == T_NULL)
          continue;
        else
          return false;
      }
    }
    else
      return false;
  }
  return true;
}

bool filter_opt_valid(d_token_t* tx_params) {
  d_token_t* frmblk = d_get(tx_params, K_FROM_BLOCK);
  d_bytes(frmblk);
  if (!frmblk) { /* Optional */
  }
  else if (d_type(frmblk) == T_INTEGER || d_type(frmblk) == T_BYTES) {
  }
  else if (d_type(frmblk) == T_STRING && (!strcmp(d_string(frmblk), "latest") || !strcmp(d_string(frmblk), "earliest") || !strcmp(d_string(frmblk), "pending"))) {
  }
  else
    return false;

  d_token_t* toblk = d_get(tx_params, K_TO_BLOCK);
  d_bytes(toblk);
  if (!toblk) { /* Optional */
  }
  else if (d_type(toblk) == T_INTEGER || d_type(toblk) == T_BYTES) {
  }
  else if (d_type(toblk) == T_STRING && (!strcmp(d_string(toblk), "latest") || !strcmp(d_string(toblk), "earliest") || !strcmp(d_string(toblk), "pending"))) {
  }
  else
    return false;

  d_token_t* blockhash = d_getl(tx_params, K_BLOCK_HASH, 32);
  d_bytes(blockhash);
  if (blockhash == NULL) { /* Optional */
  }
  else if ((d_type(blockhash) == T_BYTES && d_len(blockhash) == 32) && !frmblk && !toblk) {
    /* If blockHash is present, then neither fromBlock nor toBlock are allowed. */
  }
  else
    return false;

  d_token_t* addrs = d_getl(tx_params, K_ADDRESS, 20);
  if (addrs == NULL) { /* Optional */
  }
  else if (filter_addrs_valid(addrs)) {
  }
  else
    return false;

  d_token_t* topics = d_get(tx_params, K_TOPICS);
  if (topics == NULL) { /* Optional */
  }
  else if (filter_topics_valid(topics)) {
  }
  else
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
  }
  else if (should_overwrite) {
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

in3_ret_t filter_add(in3_filter_handler_t* filters, in3_req_t* ctx, in3_filter_type_t type, char* options) {
  if (type == FILTER_PENDING)
    return IN3_ENOTSUP;
  else if (options == NULL && type != FILTER_BLOCK)
    return IN3_EINVAL;

  in3_log_debug("add filter %s : %options", type == FILTER_BLOCK ? "BLOCK" : "EVENTS", options);

  in3_ret_t  res           = IN3_OK;
  uint64_t   current_block = 0;
  in3_req_t* block_ctx     = req_find_required(ctx, "eth_blockNumber", NULL);
  if (!block_ctx)
    return req_add_required(ctx, req_new(ctx->client, _strdupn("{\"method\":\"eth_blockNumber\",\"params\":[]}", -1)));
  else {
    switch (in3_req_state(block_ctx)) {
      case REQ_ERROR:
        return req_set_error(block_ctx, block_ctx->error ? block_ctx->error : "Error fetching the blocknumber", block_ctx->verification_state ? block_ctx->verification_state : IN3_ERPC);
      case REQ_WAITING_FOR_RESPONSE:
      case REQ_WAITING_TO_SEND:
        return IN3_WAITING;
      case REQ_SUCCESS:
        if (IN3_OK != (res = req_get_error(block_ctx, 0)))
          return req_set_error(block_ctx, block_ctx->error ? block_ctx->error : "Error fetching the blocknumber", res);
        current_block = d_get_long(req_get_response(block_ctx, 0), K_RESULT);
        TRY(req_remove_required(ctx, block_ctx, false));
    }
  }

  in3_filter_t* f = filter_new(type);
  f->options      = options;
  f->last_block   = current_block;

  // Reuse filter ids that have been uninstalled
  // Note: filter ids are 1 indexed, and the associated in3_filter_t object is stored
  // at pos (id - 1) internally in in3->filters->array
  for (size_t i = 0; i < filters->count; i++) {
    if (filters->array[i] == NULL) {
      filters->array[i] = f;
      return i + 1;
    }
  }
  in3_filter_t** arr_;
  if (filters->array)
    arr_ = _realloc(filters->array, sizeof(in3_filter_t*) * (filters->count + 1), sizeof(in3_filter_t*) * (filters->count));
  else
    arr_ = _malloc(sizeof(in3_filter_t*) * (filters->count + 1));

  if (arr_ == NULL) {
    return IN3_ENOMEM;
  }
  filters->array                 = arr_;
  filters->array[filters->count] = f;
  filters->count += 1;
  return filters->count;
}

bool filter_remove(in3_filter_handler_t* filters, size_t id) {
  if (id == 0 || id > filters->count)
    return false;

  // We don't realloc the array here, instead we simply set this slot to NULL to indicate
  // that it has been removed and reuse it in add_filter()
  in3_filter_t* f = filters->array[id - 1];
  if (!f) return false;
  f->release(f);
  filters->array[id - 1] = NULL;
  return true;
}

static in3_req_t* req_find_required_for_block(in3_req_t* ctx, uint64_t block_number) {
  // find the subctx for the current blocknumber
  for (in3_req_t* sub_ctx = ctx->required; sub_ctx; sub_ctx = sub_ctx->required) {
    if (!sub_ctx->request) continue;
    d_token_t*  req             = req_get_request(sub_ctx, 0);
    const char* required_method = d_get_string(req, K_METHOD);
    if (required_method && strcmp(required_method, "eth_getBlockByNumber")) continue;
    if (block_number == d_get_long_at(d_get(req, K_PARAMS), 0)) return sub_ctx;
  }
  return NULL;
}

in3_ret_t filter_get_changes(in3_filter_handler_t* filters, in3_req_t* ctx, size_t id, sb_t* result) {
  in3_ret_t res = IN3_OK;
  if (id == 0 || id > filters->count)
    return req_set_error(ctx, "filter with id does not exist", IN3_EUNKNOWN);

  // fetch the current block number
  in3_req_t* block_ctx = req_find_required(ctx, "eth_blockNumber", NULL);
  if (!block_ctx)
    return req_add_required(ctx, req_new(ctx->client, _strdupn("{\"method\":\"eth_blockNumber\",\"params\":[]}", -1)));
  else {
    switch (in3_req_state(block_ctx)) {
      case REQ_ERROR:
        return req_set_error(block_ctx, block_ctx->error ? block_ctx->error : "Error fetching the blocknumber", block_ctx->verification_state ? block_ctx->verification_state : IN3_ERPC);
      case REQ_WAITING_FOR_RESPONSE:
      case REQ_WAITING_TO_SEND:
        return IN3_WAITING;
      case REQ_SUCCESS:
        if (IN3_OK != (res = req_get_error(block_ctx, 0)))
          return req_set_error(block_ctx, block_ctx->error ? block_ctx->error : "Error fetching the blocknumber", res);
    }
  }
  uint64_t blkno = d_get_long(req_get_response(block_ctx, 0), K_RESULT);

  in3_filter_t* f = filters->array[id - 1];
  if (!f)
    return req_set_error(ctx, "filter with id does not exist", IN3_EUNKNOWN);

  char* fopt = f->options;
  switch (f->type) {
    case FILTER_EVENT: {
      if (f->last_block > blkno) {
        // still same block -> no events!
        sb_add_chars(result, "[]");
        return IN3_OK;
      }

      in3_req_t* logs_ctx = req_find_required(ctx, "eth_getLogs", NULL);
      if (!logs_ctx) {
        char* fopt_ = filter_opt_set_fromBlock(fopt, f->last_block, !f->is_first_usage);
        char* req   = sprintx("{\"method\":\"eth_getLogs\",\"params\":[%s]}", fopt_);
        _free(fopt_);
        return req_add_required(ctx, req_new(ctx->client, req));
      }
      // check existing ctx
      switch (in3_req_state(logs_ctx)) {
        case REQ_ERROR:
          return req_set_error(logs_ctx, logs_ctx->error ? logs_ctx->error : "Error fetching logs", logs_ctx->verification_state ? logs_ctx->verification_state : IN3_ERPC);
        case REQ_WAITING_FOR_RESPONSE:
        case REQ_WAITING_TO_SEND:
          return IN3_WAITING;
        case REQ_SUCCESS:
          if (IN3_OK != (res = req_get_error(logs_ctx, 0)))
            return req_set_error(logs_ctx, logs_ctx->error ? logs_ctx->error : "Error fetching logs", res);
      }
      d_token_t* r = d_get(req_get_response(logs_ctx, 0), K_RESULT);
      if (!r) return req_set_error(logs_ctx, "no result in filter response", IN3_ERPC);
      char* jr = d_create_json(logs_ctx->response, r);
      sb_add_chars(result, jr);
      _free(jr);
      f->last_block     = blkno + 1;
      f->is_first_usage = false;
      return IN3_OK;
    }
    case FILTER_BLOCK:
      if (blkno <= f->last_block) {
        // no new blocks
        sb_add_chars(result, "[]");
        return IN3_OK;
      }
      else {
        sb_add_char(result, '[');
        for (uint64_t i = f->last_block + 1; i <= blkno; i++) {
          in3_req_t* block_ctx = req_find_required_for_block(ctx, i);

          if (!block_ctx) {
            char* req = _malloc(150);
            sprintf(req, "{\"method\":\"eth_getBlockByNumber\",\"params\":[\"0x%" PRIx64 "\", false]}", i);
            res = req_add_required(ctx, req_new(ctx->client, req));
          }
          else {
            // check existing ctx
            switch (in3_req_state(block_ctx)) {
              case REQ_ERROR:
                return req_set_error(block_ctx, block_ctx->error ? block_ctx->error : "Error fetching blocks", block_ctx->verification_state ? block_ctx->verification_state : IN3_ERPC);
              case REQ_WAITING_FOR_RESPONSE:
              case REQ_WAITING_TO_SEND:
                return IN3_WAITING;
              case REQ_SUCCESS:
                if (IN3_OK != (res = req_get_error(block_ctx, 0)))
                  return req_set_error(block_ctx, block_ctx->error ? block_ctx->error : "Error fetching blocks", res);
            }

            d_token_t* hash = d_getl(d_get(req_get_response(block_ctx, 0), K_RESULT), K_HASH, 32);
            if (i > f->last_block + 1)
              sb_add_char(result, ',');
            sb_add_bytes(result, NULL, d_as_bytes(hash), 1, false);
          }
        }

        sb_add_char(result, ']');
        if (res) return res; // if this was not complete, we still need to wait
        f->last_block = blkno;
        return IN3_OK;
      }
    default: break;
  }
  return req_set_error(ctx, "unsupported filter type", IN3_ENOTSUP);
}