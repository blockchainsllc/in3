#include "../../../core/client/client.h"
#include "../../../core/client/keys.h"
#include "../../../core/util/mem.h"
#include "../../../third-party/crypto/ecdsa.h"
#include "../../../third-party/crypto/secp256k1.h"
#include "../../../verifier/eth1/nano/serialize.h"
#include "signer-priv.h"

static inline bytes_t get(d_token_t* t, uint16_t key) {
  return d_to_bytes(d_get(t, key));
}
static inline bytes_t getl(d_token_t* t, uint16_t key, size_t l) {
  return d_to_bytes(d_getl(t, key, l));
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
    res = ctx_set_error(parent, ctx->error, IN3_ERPC);
  else {
    d_token_t* result = d_get(ctx->responses[0], K_RESULT);
    if (!result)
      res = ctx_set_error(parent, "No result found when fetching data for tx", IN3_ERPCNRES);
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
in3_ret_t eth_sign(void* pk, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst) {
  UNUSED_VAR(account); // at least for now
  switch (type) {
    case SIGN_EC_RAW:
      if (ecdsa_sign_digest(&secp256k1, pk, message.data, dst, dst + 64, NULL) < 0)
        return IN3_EUNKNOWN;
      break;
    case SIGN_EC_HASH:
      if (ecdsa_sign(&secp256k1, HASHER_SHA3K, pk, message.data, message.len, dst, dst + 64, NULL) < 0)
        return IN3_EUNKNOWN;
      break;

    default:
      return IN3_ENOTSUP;
  }
  return 65;
}

/** sets the signer and a pk to the client*/
in3_ret_t eth_set_pk_signer(in3_t* in3, bytes32_t pk) {
  if (in3->signer) _free(in3->signer);
  in3->signer         = _malloc(sizeof(in3_signer_t));
  in3->signer->sign   = eth_sign;
  in3->signer->wallet = pk;
  return IN3_OK;
}

bytes_t sign_tx(d_token_t* tx, in3_ctx_t* ctx) {
  address_t from;
  bytes32_t nonce_data, gas_price_data;
  bytes_t   tmp;
  uint8_t   sig[65];

  // get the from-address
  if ((tmp = d_to_bytes(d_getl(tx, K_FROM, 20))).len == 0) {
    if (!d_get(tx, K_NONCE)) {
      // Derive the from-address from pk if no nonce is given.
      // Note: This works because the signer->wallet points to the pk in the current signer implementation
      // (see eth_set_pk_signer()), and may change in the future.
      // Also, other wallet implementations may differ - hence the check.
      if (ctx->client->signer->sign != eth_sign) {
        ctx_set_error(ctx, "you need to specify the from-address in the tx!", IN3_EINVAL);
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
          gas_limit = d_get(tx, K_GAS) ? get(tx, K_GAS) : (d_get(tx, K_GAS_LIMIT) ? get(tx, K_GAS_LIMIT) : bytes((uint8_t*) "\x52\x08", 2)),
          to        = getl(tx, K_TO, 20),
          value     = get(tx, K_VALUE),
          data      = get(tx, K_DATA);
  uint64_t v        = ctx->requests_configs->chainId > 0xFF ? 0 : ctx->requests_configs->chainId;

  // create raw without signature
  bytes_t* raw = serialize_tx_raw(nonce, gas_price, gas_limit, to, value, data, v, bytes(NULL, 0), bytes(NULL, 0));

  // sign the raw message
  int res = ctx->client->signer->sign(ctx->client->signer->wallet, SIGN_EC_HASH, *raw, bytes(NULL, 0), sig);

  // free temp resources
  b_free(raw);
  sb_free(sb);
  if (res < 0) return bytes(NULL, 0);

  // create raw transaction with signature
  raw            = serialize_tx_raw(nonce, gas_price, gas_limit, to, value, data, 27 + sig[64] + (v ? (v * 2 + 8) : 0), bytes(sig, 32), bytes(sig + 32, 32));
  bytes_t raw_tx = bytes(raw->data, raw->len);
  _free(raw); // we only free the struct, not the data!

  return raw_tx;
}