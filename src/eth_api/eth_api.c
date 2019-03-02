#include "./eth_api.h"
#include "../core/client/context.h"
#include "../core/client/keys.h"
#include "../eth_nano/rlp.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define rpc_init sb_t* params = params_new();
#define rpc_exec(METHOD, RETURN_TYPE, HANDLE_RESULT)                             \
  errno              = 0;                                                        \
  in3_ctx_t*  ctx    = in3_client_rpc_ctx(in3, (METHOD), params_finish(params)); \
  d_token_t*  result = get_result(ctx);                                          \
  RETURN_TYPE res;                                                               \
  if (result)                                                                    \
    res = (HANDLE_RESULT);                                                       \
  else                                                                           \
    memset(&res, 0, sizeof(RETURN_TYPE));                                        \
  free_ctx(ctx);                                                                 \
  sb_free(params);                                                               \
  return res;

static char* last_error = NULL;
char*        eth_last_error() {
  return last_error;
}

static void set_errorn(int std_error, char* msg, int len) {
  errno = std_error;
  if (last_error) _free(last_error);
  last_error = _malloc(len + 1);
  memcpy(last_error, msg, len);
  last_error[len] = 0;
}
static void set_error(int std_error, char* msg) {
  set_errorn(std_error, msg, strlen(msg));
}

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

long double as_double(uint256_t d) {
  uint8_t* p = d.data;
  int      l = 32;
  optimize_len(p, l);
  if (l < 9)
    return bytes_to_long(p, l);
  else {
    long double val = 6277101735386680763835789423207666416102355444464034512896.0L * bytes_to_long(d.data, 8);
    val += 340282366920938463463374607431768211456.0L * bytes_to_long(d.data + 8, 8);
    val += 18446744073709551616.0L * bytes_to_long(d.data + 16, 8);
    return val + bytes_to_long(d.data + 24, 8);
  }
}
uint64_t as_long(uint256_t d) {
  return bytes_to_long(d.data + 32 - 8, 8);
}

static uint256_t uint256_from_bytes(bytes_t bytes) {
  uint256_t d;
  copy_fixed(d.data, 32, bytes);
  return d;
}

static void long_to_hex(uint64_t n, char* dst) {
  sprintf(dst, "0x%" PRIu64 "", n);
}

static d_token_t* get_result(in3_ctx_t* ctx) {
  d_token_t* res = d_get(ctx->responses[0], K_RESULT);
  if (res) return res;
  if (ctx->error)
    set_error(ETIMEDOUT, ctx->error);
  else {
    d_token_t* r = d_get(ctx->responses[0], K_ERROR);
    // the response was correct but contains a error-object, which we convert into a string
    if (d_type(r) == T_OBJECT) {
      str_range_t s = d_to_json(r);
      set_errorn(ETIMEDOUT, s.data, s.len);
    } else
      set_errorn(ETIMEDOUT, d_string(r), d_len(r));
  }
  return NULL;
}

static sb_t* params_new() {
  return sb_new("[");
}

static char* params_finish(sb_t* sb) {
  return sb_add_char(sb, ']')->data;
}
static void params_add_number(sb_t* sb, uint64_t num) {
  if (sb->len > 1) sb_add_char(sb, ',');
  char tmp[30];
  long_to_hex(num, tmp);
  sb_add_char(sb, '"');
  sb_add_chars(sb, tmp);
  sb_add_char(sb, '"');
}

static void params_add_blocknumber(sb_t* sb, uint64_t bn) {
  if (bn)
    params_add_number(sb, bn);
  else {
    if (sb->len > 1) sb_add_char(sb, ',');
    sb_add_chars(sb, "\"latest\"");
  }
}

static void params_add_bytes(sb_t* sb, bytes_t data) {
  if (sb->len > 1) sb_add_char(sb, ',');
  sb_add_bytes(sb, "", &data, 1, false);
}

static void params_add_bool(sb_t* sb, bool val) {
  if (sb->len > 1) sb_add_char(sb, ',');
  sb_add_chars(sb, val ? "true" : "false");
}

static uint32_t write_tx(d_token_t* t, eth_tx_t* tx) {
  bytes_t b = d_to_bytes(d_get(t, K_INPUT));
  memcpy(tx + sizeof(eth_tx_t), b.data, b.len);
  copy_fixed(tx->block_hash, 32, d_to_bytes(d_get(t, K_BLOCK_HASH)));
  copy_fixed(tx->from, 20, d_to_bytes(d_get(t, K_FROM)));
  copy_fixed(tx->to, 20, d_to_bytes(d_get(t, K_TO)));
  copy_fixed(tx->value.data, 32, d_to_bytes(d_get(t, K_VALUE)));
  copy_fixed(tx->hash, 32, d_to_bytes(d_get(t, K_HASH)));
  copy_fixed(tx->signature, 32, d_to_bytes(d_get(t, K_R)));
  copy_fixed(tx->signature + 32, 32, d_to_bytes(d_get(t, K_S)));
  tx->signature[64]     = d_get_intk(t, K_V);
  tx->block_number      = d_get_longk(t, K_NUMBER);
  tx->gas               = d_get_longk(t, K_GAS);
  tx->gas_price         = d_get_longk(t, K_GAS_PRICE);
  tx->nonce             = d_get_longk(t, K_NONCE);
  tx->data              = bytes((uint8_t*) tx + sizeof(eth_tx_t), b.len);
  tx->transaction_index = d_get_intk(t, K_TRANSACTION_INDEX);

  return sizeof(eth_tx_t) + b.len;
}
static uint32_t get_tx_size(d_token_t* tx) {
  return d_to_bytes(d_get(tx, K_INPUT)).len + sizeof(eth_tx_t);
}

static eth_block_t* eth_getBlock(d_token_t* result, bool include_tx) {
  eth_block_t* res = NULL;
  if (result) {
    if (d_type(result) == T_NULL)
      set_error(EAGAIN, "Block does not exist");
    else {
      d_token_t* sealed = d_get(result, K_SEAL_FIELDS);
      d_token_t* txs    = d_get(result, K_TRANSACTIONS);
      bytes_t    extra  = d_to_bytes(d_get(result, K_EXTRA_DATA));

      // calc size
      uint32_t s = sizeof(eth_block_t);
      if (include_tx) {
        for (d_iterator_t it = d_iter(txs); it.i; d_iter_next(&it))
          s += get_tx_size(it.token);
      } else
        s += 32 * d_len(txs);

      s += extra.len;
      for (d_iterator_t sf = d_iter(sealed); sf.i; d_iter_next(&sf)) {
        bytes_t t = d_to_bytes(sf.token);
        rlp_decode(&t, 0, &t);
        s += t.len + sizeof(bytes_t);
      }

      // copy data
      eth_block_t* b = malloc(s);
      if (!b) {
        set_error(ENOMEM, "Not enough memory");
        return NULL;
      }
      uint8_t* p = (uint8_t*) b + sizeof(eth_block_t);
      copy_fixed(b->author, 20, d_to_bytes(d_get(result, K_AUTHOR)));
      copy_fixed(b->difficulty.data, 32, d_to_bytes(d_get(result, K_DIFFICULTY)));
      copy_fixed(b->hash, 32, d_to_bytes(d_get(result, K_HASH)));
      copy_fixed(b->logsBloom, 256, d_to_bytes(d_get(result, K_LOGS_BLOOM)));
      copy_fixed(b->parent_hash, 32, d_to_bytes(d_get(result, K_PARENT_HASH)));
      copy_fixed(b->transaction_root, 32, d_to_bytes(d_get(result, K_TRANSACTIONS_ROOT)));
      copy_fixed(b->receipts_root, 32, d_to_bytes(d_get_or(result, K_RECEIPT_ROOT, K_RECEIPTS_ROOT)));
      copy_fixed(b->sha3_uncles, 32, d_to_bytes(d_get(result, K_SHA3_UNCLES)));
      copy_fixed(b->state_root, 32, d_to_bytes(d_get(result, K_STATE_ROOT)));
      b->gasLimit          = d_get_longk(result, K_GAS_LIMIT);
      b->gasUsed           = d_get_longk(result, K_GAS_USED);
      b->number            = d_get_longk(result, K_NUMBER);
      b->timestamp         = d_get_longk(result, K_TIMESTAMP);
      b->tx_count          = d_len(txs);
      b->seal_fields_count = d_len(sealed);
      b->extra_data        = bytes(p, extra.len);
      memcpy(p, extra.data, extra.len);
      p += extra.len;
      b->seal_fields = (void*) p;
      p += sizeof(bytes_t) * b->seal_fields_count;
      for (d_iterator_t sf = d_iter(sealed); sf.i; d_iter_next(&sf)) {
        bytes_t t = d_to_bytes(sf.token);
        rlp_decode(&t, 0, &t);
        b->seal_fields[b->seal_fields_count - sf.i] = bytes(p, t.len);
        memcpy(p, t.data, t.len);
        p += t.len;
      }

      b->tx_data   = include_tx ? (eth_tx_t*) p : NULL;
      b->tx_hashes = include_tx ? NULL : (bytes32_t*) p;

      for (d_iterator_t it = d_iter(txs); it.i; d_iter_next(&it)) {
        if (include_tx)
          p += write_tx(it.token, (eth_tx_t*) p);
        else {
          copy_fixed(p, 32, d_to_bytes(it.token));
          p += 32;
        }
      }
      res = b;
    }
  }
  return res;
}

eth_block_t* eth_getBlockByNumber(in3_t* in3, uint64_t number, bool include_tx) {
  rpc_init;
  params_add_blocknumber(params, number);
  params_add_bool(params, include_tx);
  rpc_exec("eth_getBlockByNumber", eth_block_t*, eth_getBlock(result, include_tx));
}

eth_block_t* eth_getBlockByHash(in3_t* in3, bytes32_t number, bool include_tx) {
  rpc_init;
  params_add_bytes(params, bytes(number, 32));
  params_add_bool(params, include_tx);
  rpc_exec("eth_getBlockByHash", eth_block_t*, eth_getBlock(result, include_tx));
}

uint256_t eth_getBalance(in3_t* in3, address_t account, uint64_t block) {
  rpc_init;
  params_add_bytes(params, bytes(account, 20));
  params_add_blocknumber(params, block);
  rpc_exec("eth_getBalance", uint256_t, uint256_from_bytes(d_to_bytes(result)));
}

uint64_t eth_blockNumber(in3_t* in3) {
  rpc_init;
  rpc_exec("eth_blockNumber", uint64_t, d_long(result));
}

uint64_t eth_gasPrice(in3_t* in3) {
  rpc_init;
  rpc_exec("eth_gasPrice", uint64_t, d_long(result));
}
