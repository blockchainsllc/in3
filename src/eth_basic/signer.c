#include "signer.h"
#include "../eth_nano/serialize.h"
#include "filter.h"
#include <client/client.h>
#include <client/context.h>
#include <client/keys.h>
#include <crypto/ecdsa.h>
#include <crypto/secp256k1.h>
#include <inttypes.h>
#include <stdio.h>
#include <util/data.h>

#define RESPONSE_START()                                                             \
  do {                                                                               \
    *response = _malloc(sizeof(in3_response_t));                                     \
    sb_init(&response[0]->result);                                                   \
    sb_init(&response[0]->error);                                                    \
    sb_add_chars(&response[0]->result, "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":"); \
  } while (0)

#define RESPONSE_END() \
  do { sb_add_char(&response[0]->result, '}'); } while (0)

static inline bytes_t get(d_token_t* t, uint16_t key) {
  return d_to_bytes(d_get(t, key));
}

/**
 * return data from the client.
 * 
 * In case of an error report this tol parent and return an empty bytes
 */
static bytes_t get_from_nodes(in3_ctx_t* parent, char* method, char* params, bytes32_t dst) {
  in3_ctx_t* ctx = in3_client_rpc_ctx(parent->client, method, params);
  bytes_t    b   = bytes(NULL, 0);
  int        res = 0;
  if (ctx->error)
    res = ctx_set_error(parent, ctx->error, -1);
  else {
    d_token_t* result = d_get(ctx->responses[0], K_RESULT);
    if (!result)
      res = ctx_set_error(parent, "No result found when fetching data for tx", -1);
    else {
      b = d_to_bytes(result);
      if (b.len)
        memcpy(dst, b.data, b.len);
      b.data = dst;
    }
  }
  free_ctx(ctx);
  return res < 0 ? bytes(NULL, 0) : b;
}

/** signs the given data */
int eth_sign(void* pk, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst) {
  UNUSED_VAR(account); // at least for now
  switch (type) {
    case SIGN_EC_RAW:
      if (ecdsa_sign_digest(&secp256k1, pk, message.data, dst, dst + 64, NULL) < 0)
        return -1;
      break;
    case SIGN_EC_HASH:
      if (ecdsa_sign(&secp256k1, HASHER_SHA3K, pk, message.data, message.len, dst, dst + 64, NULL) < 0)
        return -1;
      break;

    default:
      return -2;
  }
  return 65;
}

/** sets the signer and a pk to the client*/
int eth_set_pk_signer(in3_t* in3, bytes32_t pk) {
  if (in3->signer) _free(in3->signer);
  in3->signer         = _malloc(sizeof(in3_signer_t));
  in3->signer->sign   = eth_sign;
  in3->signer->wallet = pk;
  return 0;
}

bytes_t sign_tx(d_token_t* tx, in3_ctx_t* ctx) {
  address_t from;
  bytes32_t nonce_data, gas_price_data;
  bytes_t   tmp;
  uint8_t   sig[65];

  // get the from-address
  if ((tmp = d_to_bytes(d_get(tx, K_FROM))).len == 0) {
    if (!d_get(tx, K_NONCE)) {
      // Derive the from-address from pk if no nonce is given.
      // Note: This works because the signer->wallet points to the pk in the current signer implementation
      // (see eth_set_pk_signer()), and may change in the future.
      // Also, other wallet implementations may differ - hence the check.
      if (ctx->client->signer->sign != eth_sign) {
        ctx_set_error(ctx, "you need to specify the from-address in the tx!", -1);
        return bytes(NULL, 0);
      }

      uint8_t public_key[65], sdata[32];
      bytes_t pubkey_bytes = {.data = public_key + 1, .len = 64};
      ecdsa_get_public_key65(&secp256k1, ctx->client->signer->wallet, public_key);
      sha3_to(&pubkey_bytes, sdata);
      memcpy(from, sdata + 12, 20);
    } else
      memset(from, 0, 20);
  } else
    memcpy(from, tmp.data, 20);

  // build nonce-params
  tmp      = bytes(from, 20);
  sb_t* sb = sb_new("[");
  sb_add_bytes(sb, "", &tmp, 1, false);
  sb_add_chars(sb, ",\"latest\"]");

  // read the values
  bytes_t nonce     = d_get(tx, K_NONCE) ? get(tx, K_NONCE) : get_from_nodes(ctx, "eth_getTransactionCount", sb->data, nonce_data),
          gas_price = d_get(tx, K_GAS_PRICE) ? get(tx, K_GAS_PRICE) : get_from_nodes(ctx, "eth_gasPrice", "[]", gas_price_data),
          gas_limit = d_get(tx, K_GAS_LIMIT) ? get(tx, K_GAS_LIMIT) : bytes((uint8_t*) "\x52\x08", 2),
          to        = get(tx, K_TO),
          value     = get(tx, K_VALUE),
          data      = get(tx, K_DATA);

  // create raw without signature
  bytes_t* raw = serialize_tx_raw(nonce, gas_price, gas_limit, to, value, data, ctx->requests_configs->chainId, bytes(NULL, 0), bytes(NULL, 0));

  // sign the raw message
  int res = ctx->client->signer->sign(ctx->client->signer->wallet, SIGN_EC_HASH, *raw, bytes(NULL, 0), sig);

  // free temp resources
  b_free(raw);
  sb_free(sb);
  if (res < 0) return bytes(NULL, 0);

  // create raw transaction with signature
  raw            = serialize_tx_raw(nonce, gas_price, gas_limit, to, value, data, 27 + sig[64] + ctx->requests_configs->chainId * 2 + 8, bytes(sig, 32), bytes(sig + 32, 32));
  bytes_t raw_tx = bytes(raw->data, raw->len);
  _free(raw); // we only free the struct, not the data!

  return raw_tx;
}

// this is called before a request is send
int eth_handle_intern(in3_ctx_t* ctx, in3_response_t** response) {
  if (ctx->len > 1) return 0; // internal handling is only possible for single requests (at least for now)
  d_token_t* req = ctx->requests[0];

  // check method
  if (strcmp(d_get_stringk(req, K_METHOD), "eth_sendTransaction") == 0) {
    // get the transaction-object
    d_token_t* tx_params = d_get(req, K_PARAMS);
    if (!tx_params || d_type(tx_params + 1) != T_OBJECT) return ctx_set_error(ctx, "invalid params", -1);
    if (!ctx->client->signer) return ctx_set_error(ctx, "no signer set", -1);

    // sign it.
    bytes_t raw = sign_tx(tx_params + 1, ctx);
    if (!raw.len) return ctx_set_error(ctx, "error signing the transaction", -1);

    // build the RPC-request
    uint64_t id = d_get_longk(req, K_ID);
    sb_t*    sb = sb_new("{ \"jsonrpc\":\"2.0\", \"method\":\"eth_sendRawTransaction\", \"params\":[");
    sb_add_bytes(sb, "", &raw, 1, false);
    sb_add_chars(sb, "]");
    if (id) {
      char tmp[16];
      sprintf(tmp, ", \"id\":%" PRId64 "", id);
      sb_add_chars(sb, tmp);
    }
    sb_add_chars(sb, "}");

    // now that we included the signature in the rpc-request, we can free it + the old rpc-request.
    _free(raw.data);
    free_json(ctx->request_context);

    // set the new RPC-Request.
    ctx->request_context = parse_json(sb->data);
    ctx->requests[0]     = ctx->request_context->result;

    // we add the request-string to the cache, to make sure the request-string will be cleaned afterwards
    ctx->cache = in3_cache_add_entry(ctx->cache, bytes(NULL, 0), bytes((uint8_t*) sb->data, sb->len));
  } else if (strcmp(d_get_stringk(req, K_METHOD), "eth_newFilter") == 0) {
    int        ret       = 0;
    d_token_t* tx_params = d_get(req, K_PARAMS);
    if (!tx_params || d_type(tx_params + 1) != T_OBJECT)
      return ctx_set_error(ctx, "invalid params", -1);

    in3_filter_opt_t* fopt = filter_opt_new();
    if (!fopt) return ctx_set_error(ctx, "filter option creation failed", -1);

    ret = fopt->from_json(fopt, tx_params);
    if (ret != 0) {
      fopt->release(fopt);
      return ctx_set_error(ctx, "filter option parsing failed", -1);
    }

    size_t id = filter_add(ctx->client, FILTER_EVENT, fopt);
    if (!id) {
      fopt->release(fopt);
      return ctx_set_error(ctx, "filter creation failed", -1);
    }

    RESPONSE_START();
    sb_add_char(&response[0]->result, '"');
    char* strid = stru64(id);
    sb_add_chars(&response[0]->result, strid);
    free(strid);
    sb_add_char(&response[0]->result, '"');
    RESPONSE_END();
    return 0;
  } else if (strcmp(d_get_stringk(req, K_METHOD), "eth_newBlockFilter") == 0) {
    size_t id = filter_add(ctx->client, FILTER_BLOCK, NULL);
    if (!id) return ctx_set_error(ctx, "filter creation failed", -1);

    RESPONSE_START();
    sb_add_char(&response[0]->result, '"');
    char* strid = stru64(id);
    sb_add_chars(&response[0]->result, strid);
    free(strid);
    sb_add_char(&response[0]->result, '"');
    RESPONSE_END();
  } else if (strcmp(d_get_stringk(req, K_METHOD), "eth_newPendingTransactionFilter") == 0) {
    return ctx_set_error(ctx, "pending filter not supported", -1);
  } else if (strcmp(d_get_stringk(req, K_METHOD), "eth_uninstallFilter") == 0) {
    d_token_t* tx_params = d_get(req, K_PARAMS);
    if (!tx_params || d_len(tx_params) == 0 || d_type(tx_params + 1) != T_INTEGER)
      return ctx_set_error(ctx, "invalid params", -1);

    uint64_t id = d_get_long_at(tx_params, 0);
    if (id == 0 || id > ctx->client->filters->count)
      return ctx_set_error(ctx, "invalid params (id)", -1);

    RESPONSE_START();
    filter_remove(ctx->client, id) ? sb_add_chars(&response[0]->result, "true") : sb_add_chars(&response[0]->result, "false");
    RESPONSE_END();
  } else if (strcmp(d_get_stringk(req, K_METHOD), "eth_getFilterChanges") == 0) {
    d_token_t* tx_params = d_get(req, K_PARAMS);
    if (!tx_params || d_len(tx_params) == 0 || d_type(tx_params + 1) != T_INTEGER)
      return ctx_set_error(ctx, "invalid params", -1);

    uint64_t id = d_get_long_at(tx_params, 0);
    if (id == 0 || id > ctx->client->filters->count)
      return ctx_set_error(ctx, "invalid params (id)", -1);

    in3_ctx_t* ctx_ = in3_client_rpc_ctx(ctx->client, "eth_blockNumber", "[]");
    if (ctx_->error || !ctx_->responses || !ctx_->responses[0] || !d_get(ctx_->responses[0], K_RESULT)) {
      free_ctx(ctx_);
      return ctx_set_error(ctx, "internal error (eth_blockNumber)", -1);
    }
    uint64_t blkno = d_get_longk(ctx_->responses[0], K_RESULT);
    free_ctx(ctx_);

    in3_filter_t*     f    = ctx->client->filters->array[id - 1];
    in3_filter_opt_t* fopt = f->options;
    switch (f->type) {
      case FILTER_EVENT: {
        sb_t* params = sb_new("[");
        params       = fopt->to_json_str(fopt, params);
        ctx_         = in3_client_rpc_ctx(ctx->client, "eth_getLogs", sb_add_char(params, ']')->data);
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

        RESPONSE_START();
        char* jr = d_create_json(r);
        sb_add_chars(&response[0]->result, jr);
        _free(jr);
        RESPONSE_END();

        free_ctx(ctx_);
        f->last_block = blkno + 1;
        return 0;
      }
      case FILTER_BLOCK:
        if (blkno > f->last_block) {
          char params[37] = {0};
          RESPONSE_START();
          sb_add_char(&response[0]->result, '[');
          for (uint64_t i = f->last_block + 1, j = 0; i <= blkno; i++, j++) {
            sprintf(params, "[\"0x%" PRIx64 "\", true]", i);
            ctx_ = in3_client_rpc_ctx(ctx->client, "eth_getBlockByNumber", params);
            if (ctx_->error || !ctx_->responses || !ctx_->responses[0] || !d_get(ctx_->responses[0], K_RESULT)) {
              free_ctx(ctx_);
              return ctx_set_error(ctx, "internal error (eth_getBlockByNumber)", -1);
            }
            d_token_t* res = d_get(ctx_->responses[0], K_RESULT);
            if (res == NULL || d_type(res) == T_NULL) {
              // error or block doesn't exist
              continue;
            }
            d_token_t* hash  = d_get(res, K_HASH);
            char       h[67] = "0x";
            bytes_to_hex(d_bytes(hash)->data, 32, h + 2);
            if (j != 0)
              sb_add_char(&response[0]->result, ',');
            sb_add_char(&response[0]->result, '"');
            sb_add_chars(&response[0]->result, h);
            sb_add_char(&response[0]->result, '"');
            free_ctx(ctx_);
          }
          sb_add_char(&response[0]->result, ']');
          RESPONSE_END();
          f->last_block = blkno;
          return 0;
        } else {
          RESPONSE_START();
          sb_add_chars(&response[0]->result, "[]");
          RESPONSE_END();
          return 0;
        }
      default:
        return ctx_set_error(ctx, "internal error", -1);
    }
  }
  return 0;
}
