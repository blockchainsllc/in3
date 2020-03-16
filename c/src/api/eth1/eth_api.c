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

#include "eth_api.h"
#include "../../core/client/context.h"
#include "../../core/client/keys.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "../../verifier/eth1/basic/filter.h"
#include "../../verifier/eth1/nano/rlp.h"
#include "../utils/api_utils_priv.h"
#include "abi.h"

/** copies bytes to a fixed length destination (leftpadding 0 if needed).*/
static void copy_fixed(uint8_t* dst, uint32_t len, bytes_t data) {
  if (data.len > len)
    memcpy(dst, data.data + data.len - len, len);
  else if (data.len == len)
    memcpy(dst, data.data, len);
  else if (data.len) {
    memcpy(dst + len - data.len, data.data, data.len);
    memset(dst, 0, len - data.len);
  } else
    memset(dst, 0, len);
}

/** creates a uin256_t from a flexible byte */
static uint256_t uint256_from_bytes(bytes_t bytes) {
  uint256_t d;
  copy_fixed(d.data, 32, bytes);
  return d;
}

/** adds a number as hex */
static void params_add_number(sb_t* sb, uint64_t num) {
  char tmp[30];
  if (sb->len > 1) sb_add_char(sb, ',');
  sprintf(tmp, "\"0x%" PRIx64 "\"", num);
  sb_add_chars(sb, tmp);
}

static void params_add_blk_num_t(sb_t* sb, eth_blknum_t bn) {
  if (bn.is_u64) {
    params_add_number(sb, bn.u64);
  } else {
    if (sb->len > 1) sb_add_chars(sb, ", \"");
    switch (bn.def) {
      case BLK_LATEST:
        sb_add_chars(sb, "latest");
        break;
      case BLK_EARLIEST:
        sb_add_chars(sb, "earliest");
        break;
      case BLK_PENDING:
        sb_add_chars(sb, "pending");
        break;
    }
    sb_add_char(sb, '\"');
  }
}

/** add ther raw bytes as hex*/
static void params_add_bytes(sb_t* sb, bytes_t data) {
  if (sb->len > 1) sb_add_char(sb, ',');
  sb_add_bytes(sb, "", &data, 1, false);
}

/** add a booleans as true or false */
static void params_add_bool(sb_t* sb, bool val) {
  if (sb->len > 1) sb_add_char(sb, ',');
  sb_add_chars(sb, val ? "true" : "false");
}
static size_t align(size_t val) {
  const size_t add = val % sizeof(void*);
  return add ? (sizeof(void*) - add + val) : val;
}

/** copy the data from the token to a eth_tx_t-object */
static uint32_t write_tx(d_token_t* t, eth_tx_t* tx) {
  bytes_t b = d_to_bytes(d_get(t, K_INPUT));

  tx->signature[64]     = d_get_intk(t, K_V);
  tx->block_number      = d_get_longk(t, K_BLOCK_NUMBER);
  tx->gas               = d_get_longk(t, K_GAS);
  tx->gas_price         = d_get_longk(t, K_GAS_PRICE);
  tx->nonce             = d_get_longk(t, K_NONCE);
  tx->data              = bytes((uint8_t*) tx + sizeof(eth_tx_t), b.len);
  tx->transaction_index = d_get_intk(t, K_TRANSACTION_INDEX);
  memcpy((uint8_t*) tx + sizeof(eth_tx_t), b.data, b.len); // copy the data right after the tx-struct.
  copy_fixed(tx->block_hash, 32, d_to_bytes(d_getl(t, K_BLOCK_HASH, 32)));
  copy_fixed(tx->from, 20, d_to_bytes(d_getl(t, K_FROM, 20)));
  copy_fixed(tx->to, 20, d_to_bytes(d_getl(t, K_TO, 20)));
  copy_fixed(tx->value.data, 32, d_to_bytes(d_getl(t, K_VALUE, 32)));
  copy_fixed(tx->hash, 32, d_to_bytes(d_getl(t, K_HASH, 32)));
  copy_fixed(tx->signature, 32, d_to_bytes(d_getl(t, K_R, 32)));
  copy_fixed(tx->signature + 32, 32, d_to_bytes(d_getl(t, K_S, 32)));

  return align(sizeof(eth_tx_t) + b.len);
}
/** calculate the tx size as struct+data */
static uint32_t get_tx_size(d_token_t* tx) { return align(d_to_bytes(d_get(tx, K_INPUT)).len) + sizeof(eth_tx_t); }

/** 
 * allocates memory for the block and all required lists like the transactions and copies the data.
 * 
 * this allocates more memory than just the block-struct, but does it with one malloc!
 * so the structure looksm like this:
 * 
 * struct {
 *   eth_block_t block;
 *   uint8_t[]   extra_data;
 *   bytes_t[]   seal_fields;
 *   uint8_t[]   seal_fields bytes-datas;
 *   eth_tx_t[]  transactions including their data;
 * }
 */
static eth_block_t* eth_getBlock(d_token_t* result, bool include_tx) {
  if (result) {
    if (d_type(result) == T_NULL)
      api_set_error(EAGAIN, "Block does not exist");
    else {
      d_token_t* sealed = d_get(result, K_SEAL_FIELDS);
      d_token_t* txs    = d_get(result, K_TRANSACTIONS);
      bytes_t    extra  = d_to_bytes(d_get(result, K_EXTRA_DATA));

      // calc size
      uint32_t s = align(sizeof(eth_block_t));
      if (include_tx) {
        for (d_iterator_t it = d_iter(txs); it.left; d_iter_next(&it))
          s += get_tx_size(it.token); // add all struct-size for each transaction
      } else                          // or
        s += 32 * d_len(txs);         // just the transaction hashes
      s += align(extra.len);          // extra-data
      for (d_iterator_t sf = d_iter(sealed); sf.left; d_iter_next(&sf)) {
        bytes_t t = d_to_bytes(sf.token);
        rlp_decode(&t, 0, &t);
        s += align(t.len) + align(sizeof(bytes_t)); // for each field in the selad-fields we need a bytes_t-struct in the array + the data itself
      }

      // copy data
      eth_block_t* b = _calloc(1, s);
      if (!b) {
        api_set_error(ENOMEM, "Not enough memory");
        return NULL;
      }
      uint8_t* p = (uint8_t*) b + align(sizeof(eth_block_t)); // pointer where we add the next data after the block-struct
      copy_fixed(b->author, 20, d_to_bytes(d_getl(result, K_AUTHOR, 20)));
      copy_fixed(b->difficulty.data, 32, d_to_bytes(d_get(result, K_DIFFICULTY)));
      copy_fixed(b->hash, 32, d_to_bytes(d_getl(result, K_HASH, 32)));
      copy_fixed(b->logsBloom, 256, d_to_bytes(d_getl(result, K_LOGS_BLOOM, 256)));
      copy_fixed(b->parent_hash, 32, d_to_bytes(d_getl(result, K_PARENT_HASH, 32)));
      copy_fixed(b->transaction_root, 32, d_to_bytes(d_getl(result, K_TRANSACTIONS_ROOT, 32)));

      d_token_t* t = NULL;
      if ((t = d_getl(result, K_RECEIPT_ROOT, 32)) || (t = d_getl(result, K_RECEIPTS_ROOT, 32)))
        copy_fixed(b->receipts_root, 32, d_to_bytes(t));

      copy_fixed(b->sha3_uncles, 32, d_to_bytes(d_getl(result, K_SHA3_UNCLES, 32)));
      copy_fixed(b->state_root, 32, d_to_bytes(d_getl(result, K_STATE_ROOT, 32)));
      b->gasLimit          = d_get_longk(result, K_GAS_LIMIT);
      b->gasUsed           = d_get_longk(result, K_GAS_USED);
      b->number            = d_get_longk(result, K_NUMBER);
      b->timestamp         = d_get_longk(result, K_TIMESTAMP);
      b->tx_count          = d_len(txs);
      b->seal_fields_count = d_len(sealed);
      b->extra_data        = bytes(p, extra.len);
      memcpy(p, extra.data, extra.len);
      p += align(extra.len);
      b->seal_fields = (void*) p;
      p += align(sizeof(bytes_t)) * b->seal_fields_count;
      for (d_iterator_t sfitr = d_iter(sealed); sfitr.left; d_iter_next(&sfitr)) {
        bytes_t sf = d_to_bytes(sfitr.token);
        rlp_decode(&sf, 0, &sf);
        b->seal_fields[b->seal_fields_count - sfitr.left] = bytes(p, sf.len);
        memcpy(p, sf.data, sf.len);
        p += align(sf.len);
      }

      b->tx_data   = include_tx ? (eth_tx_t*) p : NULL;
      b->tx_hashes = include_tx ? NULL : (bytes32_t*) p;

      for (d_iterator_t it = d_iter(txs); it.left; d_iter_next(&it)) {
        if (include_tx)
          p += write_tx(it.token, (eth_tx_t*) p);
        else {
          copy_fixed(p, 32, d_to_bytes(it.token));
          p += 32;
        }
      }
      return b;
    }
  }
  return NULL;
}

eth_block_t* eth_getBlockByNumber(in3_t* in3, eth_blknum_t number, bool include_tx) {
  rpc_init;
  params_add_blk_num_t(params, number);
  params_add_bool(params, include_tx);
  rpc_exec("eth_getBlockByNumber", eth_block_t*, eth_getBlock(result, include_tx));
}

eth_block_t* eth_getBlockByHash(in3_t* in3, bytes32_t number, bool include_tx) {
  rpc_init;
  params_add_bytes(params, bytes(number, 32));
  params_add_bool(params, include_tx);
  rpc_exec("eth_getBlockByHash", eth_block_t*, eth_getBlock(result, include_tx));
}

uint256_t eth_getBalance(in3_t* in3, address_t account, eth_blknum_t block) {
  rpc_init;
  params_add_bytes(params, bytes(account, 20));
  params_add_blk_num_t(params, block);
  rpc_exec("eth_getBalance", uint256_t, uint256_from_bytes(d_to_bytes(result)));
}

bytes_t eth_getCode(in3_t* in3, address_t account, eth_blknum_t block) {
  rpc_init;
  params_add_bytes(params, bytes(account, 20));
  params_add_blk_num_t(params, block);
  rpc_exec("eth_getCode", bytes_t, cloned_bytes(d_to_bytes(result)));
}

uint256_t eth_getStorageAt(in3_t* in3, address_t account, bytes32_t key, eth_blknum_t block) {
  rpc_init;
  params_add_bytes(params, bytes(account, 20));
  params_add_bytes(params, bytes(key, 32));
  params_add_blk_num_t(params, block);
  rpc_exec("eth_getStorageAt", uint256_t, uint256_from_bytes(d_to_bytes(result)));
}

uint64_t eth_blockNumber(in3_t* in3) {
  rpc_init;
  rpc_exec("eth_blockNumber", uint64_t, d_long(result));
}

uint64_t eth_gasPrice(in3_t* in3) {
  rpc_init;
  rpc_exec("eth_gasPrice", uint64_t, d_long(result));
}

static eth_log_t* parse_logs(d_token_t* result) {
  eth_log_t *prev, *first;
  prev = first = NULL;
  for (d_iterator_t it = d_iter(result); it.left; d_iter_next(&it)) {
    eth_log_t* log         = _calloc(1, sizeof(*log));
    log->removed           = d_get_intk(it.token, K_REMOVED);
    log->log_index         = d_get_intk(it.token, K_LOG_INDEX);
    log->transaction_index = d_get_intk(it.token, K_TRANSACTION_INDEX);
    log->block_number      = d_get_longk(it.token, K_BLOCK_NUMBER);
    log->data.len          = d_len(d_get(it.token, K_DATA));
    log->data.data         = _malloc(sizeof(uint8_t) * log->data.len);
    log->topics            = _malloc(sizeof(bytes32_t) * d_len(d_get(it.token, K_TOPICS)));
    copy_fixed(log->address, 20, d_to_bytes(d_getl(it.token, K_ADDRESS, 20)));
    copy_fixed(log->transaction_hash, 32, d_to_bytes(d_getl(it.token, K_TRANSACTION_HASH, 32)));
    copy_fixed(log->block_hash, 32, d_to_bytes(d_getl(it.token, K_BLOCK_HASH, 32)));
    copy_fixed(log->data.data, log->data.len, d_to_bytes(d_get(it.token, K_DATA)));
    size_t i = 0;
    for (d_iterator_t t = d_iter(d_getl(it.token, K_TOPICS, 32)); t.left; d_iter_next(&t), i++) {
      copy_fixed(log->topics[i], 32, d_to_bytes(t.token));
      log->topic_count += 1;
    }
    log->next = NULL;
    if (first == NULL)
      first = log;
    else if (prev != NULL)
      prev->next = log;
    prev = log;
  }
  return first;
}

eth_log_t* eth_getLogs(in3_t* in3, char* fopt) {
  rpc_init;
  sb_add_chars(params, fopt);
  rpc_exec("eth_getLogs", eth_log_t*, parse_logs(result));
}

static json_ctx_t* parse_call_result(call_request_t* req, d_token_t* result) {
  json_ctx_t* res = req_parse_result(req, d_to_bytes(result));
  req_free(req);
  return res;
}

static uint64_t* d_to_u64ptr(d_token_t* res) {
  uint64_t* p = _malloc(sizeof(uint64_t));
  *p          = d_long(res);
  return p;
}

static void* eth_call_fn_intern(in3_t* in3, address_t contract, eth_blknum_t block, bool only_estimate, char* fn_sig, va_list ap) {
  rpc_init;
  int             res = 0;
  call_request_t* req = parseSignature(fn_sig);
  if (req->in_data->type == A_TUPLE) {
    json_ctx_t* in_data = json_create();
    d_token_t*  args    = json_create_array(in_data);
    var_t*      p       = req->in_data + 1;
    for (int i = 0; i < req->in_data->type_len; i++, p = t_next(p)) {
      switch (p->type) {
        case A_BOOL:
          json_array_add_value(args, json_create_bool(in_data, va_arg(ap, int)));
          break;
        case A_ADDRESS:
          json_array_add_value(args, json_create_bytes(in_data, bytes(va_arg(ap, uint8_t*), 20)));
          break;
        case A_BYTES:
          json_array_add_value(args, json_create_bytes(in_data, va_arg(ap, bytes_t)));
          break;
        case A_STRING:
        case A_INT:
          json_array_add_value(args, json_create_string(in_data, va_arg(ap, char*)));
          break;
        case A_UINT: {
          if (p->type_len <= 4)
            json_array_add_value(args, json_create_int(in_data, va_arg(ap, uint32_t)));
          else if (p->type_len <= 8)
            json_array_add_value(args, json_create_int(in_data, va_arg(ap, uint64_t)));
          else
            json_array_add_value(args, json_create_bytes(in_data, bytes(va_arg(ap, uint256_t).data, 32)));
          break;
        }
        default:
          req->error = "unsuported token-type!";
          res        = -1;
      }
    }

    if (res >= 0 && (res = set_data(req, args, req->in_data)) < 0) req->error = "could not set the data";
    json_free(in_data);
  }
  if (res >= 0) {
    bytes_t to = bytes(contract, 20);
    sb_add_chars(params, "{\"to\":");
    sb_add_bytes(params, "", &to, 1, false);
    sb_add_chars(params, ", \"data\":");
    sb_add_bytes(params, "", &req->call_data->b, 1, false);
    sb_add_char(params, '}');
    params_add_blk_num_t(params, block);
  } else {
    api_set_error(0, req->error ? req->error : "Error parsing the request-data");
    sb_free(params);
    req_free(req);
    return NULL;
  }

  if (res >= 0) {
    if (only_estimate) {
      rpc_exec("eth_estimateGas", uint64_t*, d_to_u64ptr(result));
    } else {
      rpc_exec("eth_call", json_ctx_t*, parse_call_result(req, result));
    }
  }
  return NULL;
}

static char* wait_for_receipt(in3_t* in3, char* params, int timeout, int count) {
  errno             = 0;
  in3_ctx_t* ctx    = in3_client_rpc_ctx(in3, "eth_getTransactionReceipt", params);
  d_token_t* result = get_result(ctx);
  if (result) {
    if (d_type(result) == T_NULL) {
      ctx_free(ctx);
      if (count) {
#if defined(_WIN32) || defined(WIN32)
        Sleep(timeout);
#elif defined(__ZEPHYR__)
        k_sleep(timeout);
#else
        nanosleep((const struct timespec[]){{timeout / 1000, ((long) timeout % 1000) * 1000000L}}, NULL);
#endif
        return wait_for_receipt(in3, params, timeout + timeout, count - 1);
      } else {
        api_set_error(1, "timeout waiting for the receipt");
        return NULL;
      }
    } else {
      //
      char* c = d_create_json(result);
      ctx_free(ctx);
      return c;
    }
  }
  api_set_error(3, ctx->error ? ctx->error : "Error getting the Receipt!");
  ctx_free(ctx);
  return NULL;
}

char* eth_wait_for_receipt(in3_t* in3, bytes32_t tx_hash) {
  rpc_init;
  params_add_bytes(params, bytes(tx_hash, 32));
  char* data = wait_for_receipt(in3, sb_add_char(params, ']')->data, 1000, 8);
  sb_free(params);
  return data;
}

in3_ret_t eth_newFilter(in3_t* in3, json_ctx_t* options) {
  if (options == NULL || !filter_opt_valid(&options->result[0])) return IN3_EINVAL;
  char*     fopt = d_create_json(&options->result[0]);
  in3_ret_t res  = filter_add(in3, FILTER_EVENT, fopt);
  if (res < 0) _free(fopt);
  return res;
}

in3_ret_t eth_newBlockFilter(in3_t* in3) {
  return filter_add(in3, FILTER_BLOCK, NULL);
}

in3_ret_t eth_newPendingTransactionFilter(in3_t* in3) {
  return filter_add(in3, FILTER_PENDING, NULL);
}

bool eth_uninstallFilter(in3_t* in3, size_t id) {
  return filter_remove(in3, id);
}

in3_ret_t eth_getFilterChanges(in3_t* in3, size_t id, bytes32_t** block_hashes, eth_log_t** logs) {
  if (in3->filters == NULL)
    return IN3_EFIND;
  if (id == 0 || id > in3->filters->count)
    return IN3_EINVAL;

  in3_filter_t* f = in3->filters->array[id - 1];
  if (!f)
    return IN3_EFIND;

  uint64_t blkno = eth_blockNumber(in3);
  switch (f->type) {
    case FILTER_EVENT: {
      char* fopt_ = filter_opt_set_fromBlock(f->options, f->last_block, !f->is_first_usage);
      *logs       = eth_getLogs(in3, fopt_);
      _free(fopt_);
      f->last_block     = blkno + 1;
      f->is_first_usage = false;
      return 0;
    }
    case FILTER_BLOCK:
      if (blkno > f->last_block) {
        uint64_t blkcount = blkno - f->last_block;
        *block_hashes     = _malloc(sizeof(bytes32_t) * blkcount);
        for (uint64_t i = f->last_block + 1, j = 0; i <= blkno; i++, j++) {
          eth_block_t* blk = eth_getBlockByNumber(in3, BLKNUM(i), false);
          if (blk) {
            memcpy((*block_hashes)[j], blk->hash, 32);
            free(blk);
          } else
            return IN3_EFIND;
        }
        f->last_block = blkno;
        return (int) blkcount;
      } else {
        *block_hashes = NULL;
        return 0;
      }
    default:
      return IN3_ENOTSUP;
  }
}

in3_ret_t eth_getFilterLogs(in3_t* in3, size_t id, eth_log_t** logs) {
  if (in3->filters == NULL)
    return IN3_EFIND;
  if (id == 0 || id > in3->filters->count)
    return IN3_EINVAL;

  in3_filter_t* f = in3->filters->array[id - 1];
  if (!f)
    return IN3_EFIND;

  switch (f->type) {
    case FILTER_EVENT:
      *logs = eth_getLogs(in3, f->options);
      return (*logs) ? IN3_OK : IN3_EUNKNOWN;
    default:
      return IN3_ENOTSUP;
  }
}

void eth_log_free(eth_log_t* log) {
  _free(log->data.data);
  _free(log->topics);
  _free(log);
}

uint64_t eth_chainId(in3_t* in3) {
  rpc_init;
  rpc_exec("eth_chainId", uint64_t, d_long(result));
}

uint64_t eth_getBlockTransactionCountByHash(in3_t* in3, bytes32_t hash) {
  rpc_init;
  params_add_bytes(params, bytes(hash, 32));
  rpc_exec("eth_getBlockTransactionCountByHash", uint64_t, d_long(result));
}

uint64_t eth_getBlockTransactionCountByNumber(in3_t* in3, eth_blknum_t block) {
  rpc_init;
  params_add_blk_num_t(params, block);
  rpc_exec("eth_getBlockTransactionCountByNumber", uint64_t, d_long(result));
}

json_ctx_t* eth_call_fn(in3_t* in3, address_t contract, eth_blknum_t block, char* fn_sig, ...) {
  va_list ap;
  va_start(ap, fn_sig);
  json_ctx_t* response = eth_call_fn_intern(in3, contract, block, false, fn_sig, ap);
  va_end(ap);
  return response;
}

uint64_t eth_estimate_fn(in3_t* in3, address_t contract, eth_blknum_t block, char* fn_sig, ...) {
  va_list ap;
  va_start(ap, fn_sig);
  uint64_t* response = eth_call_fn_intern(in3, contract, block, true, fn_sig, ap);
  va_end(ap);
  uint64_t tmp = *response;
  _free(response);
  return tmp;
}

static eth_tx_t* parse_tx(d_token_t* result) {
  if (result) {
    if (d_type(result) == T_NULL)
      api_set_error(EAGAIN, "Transaction does not exist");
    else {
      uint32_t  s  = get_tx_size(result);
      eth_tx_t* tx = malloc(s);
      if (!tx) {
        api_set_error(ENOMEM, "Not enough memory");
        return NULL;
      }
      write_tx(result, tx);
      return tx;
    }
  }
  return NULL;
}

eth_tx_t* eth_getTransactionByHash(in3_t* in3, bytes32_t tx_hash) {
  rpc_init;
  params_add_bytes(params, bytes(tx_hash, 32));
  rpc_exec("eth_getTransactionByHash", eth_tx_t*, parse_tx(result));
}

eth_tx_t* eth_getTransactionByBlockHashAndIndex(in3_t* in3, bytes32_t block_hash, size_t index) {
  rpc_init;
  params_add_bytes(params, bytes(block_hash, 32));
  params_add_number(params, index);
  rpc_exec("eth_getTransactionByBlockHashAndIndex", eth_tx_t*, parse_tx(result));
}

eth_tx_t* eth_getTransactionByBlockNumberAndIndex(in3_t* in3, eth_blknum_t block, size_t index) {
  rpc_init;
  params_add_blk_num_t(params, block);
  params_add_number(params, index);
  rpc_exec("eth_getTransactionByBlockNumberAndIndex", eth_tx_t*, parse_tx(result));
}

uint64_t eth_getTransactionCount(in3_t* in3, address_t address, eth_blknum_t block) {
  rpc_init;
  params_add_bytes(params, bytes(address, 20));
  params_add_blk_num_t(params, block);
  rpc_exec("eth_getTransactionCount", uint64_t, d_long(result));
}

static eth_tx_receipt_t* parse_tx_receipt(d_token_t* result) {
  if (result) {
    if (d_type(result) == T_NULL)
      api_set_error(EAGAIN, "Error getting the Receipt!");
    else {
      eth_tx_receipt_t* txr = _malloc(sizeof(*txr));
      if (!txr) {
        api_set_error(ENOMEM, "Not enough memory");
        return NULL;
      }
      txr->transaction_index   = d_get_intk(result, K_TRANSACTION_INDEX);
      txr->block_number        = d_get_longk(result, K_BLOCK_NUMBER);
      txr->cumulative_gas_used = d_get_longk(result, K_CUMULATIVE_GAS_USED);
      txr->gas_used            = d_get_longk(result, K_GAS_USED);
      txr->status              = (d_get_intk(result, K_STATUS) == 1);
      txr->contract_address    = b_dup(d_get_byteskl(result, K_CONTRACT_ADDRESS, 20));
      txr->logs                = parse_logs(d_get(result, K_LOGS));
      copy_fixed(txr->transaction_hash, 32, d_to_bytes(d_getl(result, K_TRANSACTION_HASH, 32)));
      copy_fixed(txr->block_hash, 32, d_to_bytes(d_getl(result, K_BLOCK_HASH, 32)));
      return txr;
    }
  }
  return NULL;
}

void eth_tx_receipt_free(eth_tx_receipt_t* txr) {
  eth_log_t *curr = txr->logs, *next = NULL;
  while (curr != NULL) {
    next = curr->next;
    eth_log_free(curr);
    curr = next;
  }
  _free(txr);
}

eth_tx_receipt_t* eth_getTransactionReceipt(in3_t* in3, bytes32_t tx_hash) {
  rpc_init;
  params_add_bytes(params, bytes(tx_hash, 32));
  rpc_exec("eth_getTransactionReceipt", eth_tx_receipt_t*, parse_tx_receipt(result));
}

eth_block_t* eth_getUncleByBlockNumberAndIndex(in3_t* in3, eth_blknum_t block, size_t index) {
  rpc_init;
  params_add_blk_num_t(params, block);
  params_add_number(params, index);
  rpc_exec("eth_getUncleByBlockNumberAndIndex", eth_block_t*, eth_getBlock(result, true));
}

uint64_t eth_getUncleCountByBlockHash(in3_t* in3, bytes32_t hash) {
  rpc_init;
  params_add_bytes(params, bytes(hash, 32));
  rpc_exec("eth_getUncleCountByBlockHash", uint64_t, d_long(result));
}

uint64_t eth_getUncleCountByBlockNumber(in3_t* in3, eth_blknum_t block) {
  rpc_init;
  params_add_blk_num_t(params, block);
  rpc_exec("eth_getUncleCountByBlockNumber", uint64_t, d_long(result));
}

bytes_t* eth_sendTransaction(in3_t* in3, address_t from, address_t to, OPTIONAL_T(uint64_t) gas, OPTIONAL_T(uint64_t) gas_price, OPTIONAL_T(uint256_t) value, OPTIONAL_T(bytes_t) data, OPTIONAL_T(uint64_t) nonce) {
  rpc_init;
  sb_add_char(params, '{');
  bytes_t tmp = bytes(from, 20);
  params_add_first_pair(params, "from", sb_add_bytes(params, "", &tmp, 1, false), false);
  if (to) {
    tmp = bytes(to, 20);
    params_add_next_pair(params, "to", sb_add_bytes(params, "", &tmp, 1, false), false);
  }
  if (gas.defined) {
    params_add_next_pair(params, "gas", sb_add_hexuint(params, gas.value), true);
  }
  if (gas_price.defined) params_add_next_pair(params, "gasPrice", sb_add_hexuint(params, gas_price.value), true);
  if (value.defined) {
    tmp = bytes(value.value.data, 32);
    params_add_next_pair(params, "value", sb_add_bytes(params, "", &tmp, 1, false), false);
  }
  if (data.defined) {
    tmp = bytes(data.value.data, data.value.len);
    params_add_next_pair(params, "data", sb_add_bytes(params, "", &tmp, 1, false), false);
  }
  if (nonce.defined) params_add_next_pair(params, "nonce", sb_add_hexuint(params, nonce.value), true);
  sb_add_char(params, '}');
  rpc_exec("eth_sendTransaction", bytes_t*, b_dup(d_bytes(result)));
}

bytes_t* eth_sendRawTransaction(in3_t* in3, bytes_t data) {
  rpc_init;
  params_add_bytes(params, data);
  rpc_exec("eth_sendRawTransaction", bytes_t*, b_dup(d_bytes(result)));
}

in3_ret_t to_checksum(address_t adr, chain_id_t chain_id, char out[43]) {
  char tmp[64], msg[41], *hexadr;
  int  p = chain_id ? sprintf(tmp, "%i0x", (uint32_t) chain_id) : 0;
  bytes_to_hex(adr, 20, tmp + p);
  bytes_t hash_data = bytes((uint8_t*) tmp, p + 40);
  hexadr            = tmp + p;
  bytes32_t hash;
  sha3_to(&hash_data, hash);
  bytes_to_hex(hash, 20, msg);
  out[0]  = '0';
  out[1]  = 'x';
  out[42] = 0;
  for (int i = 0; i < 40; i++)
    out[i + 2] = hexchar_to_int(msg[i]) >= 8 ? (hexadr[i] > 0x60 ? (hexadr[i] - 0x20) : hexadr[i]) : hexadr[i];
  return IN3_OK;
}