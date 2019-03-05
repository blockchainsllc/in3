#include "signer.h"
#include "../eth_nano/serialize.h"
#include <client/context.h>
#include <client/keys.h>
#include <crypto/ecdsa.h>
#include <crypto/secp256k1.h>
#include <inttypes.h>
#include <stdio.h>
#include <util/data.h>

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
  return b;
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
      ctx_set_error(ctx, "you need to specify the from-address in the tx!", -1);
      return bytes(NULL, 0);
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
  UNUSED_VAR(response);
  if (ctx->len > 1) return 0; // internal handling is only possible for single requests (at least for now)
  d_token_t* req = ctx->requests[0];

  // check method
  if (strcmp(d_get_stringk(req, K_METHOD), "eth_sendTransaction") == 0) {
    // get the transaction-object
    d_token_t* tx = d_get(req, K_PARAMS) + 1;
    if ((int) tx == 1 || d_type(tx) != T_OBJECT) return ctx_set_error(ctx, "invalid params", -1);
    if (!ctx->client->signer) return ctx_set_error(ctx, "no signer set", -1);

    // sign it.
    bytes_t raw = sign_tx(tx, ctx);
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
    ctx->requests[0]     = ctx->request_context->items;

    // we add the request-string to the cache, to make sure the request-string will be cleaned afterwards
    ctx->cache = in3_cache_add_entry(ctx->cache, bytes(NULL, 0), bytes((uint8_t*) sb->data, sb->len));
  }
  return 0;
}
