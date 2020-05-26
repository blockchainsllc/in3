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

#include "eth_basic.h"
#include "../../../core/client/context_internal.h"
#include "../../../core/client/keys.h"
#include "../../../core/util/data.h"
#include "../../../core/util/mem.h"
#include "../../../core/util/utils.h"
#include "../../../signer/pk-signer/signer-priv.h"
#include "../../../third-party/crypto/ecdsa.h"
#include "../../../third-party/crypto/secp256k1.h"
#include "../../../verifier/eth1/basic/filter.h"
#include "../../../verifier/eth1/nano/eth_nano.h"
#include "../../../verifier/eth1/nano/merkle.h"
#include "../../../verifier/eth1/nano/rlp.h"
#include "../../../verifier/eth1/nano/serialize.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define RESPONSE_START()                                                             \
  do {                                                                               \
    *response = _malloc(sizeof(in3_response_t));                                     \
    sb_init(&response[0]->result);                                                   \
    sb_init(&response[0]->error);                                                    \
    sb_add_chars(&response[0]->result, "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":"); \
  } while (0)

#define RESPONSE_END() \
  do { sb_add_char(&response[0]->result, '}'); } while (0)

in3_ret_t in3_verify_eth_basic(in3_vctx_t* vc) {
  char* method = d_get_stringk(vc->request, K_METHOD);

  // make sure we want to verify
  if (vc->config->verification == VERIFICATION_NEVER) return IN3_OK;

  // do we have a result? if not it is a valid error-response
  if (!vc->result) {
    return IN3_OK;
  } else if (d_type(vc->result) == T_NULL) {
    // check if there's a proof for non-existence
    if (!strcmp(method, "eth_getTransactionByBlockHashAndIndex") || !strcmp(method, "eth_getTransactionByBlockNumberAndIndex")) {
      return eth_verify_eth_getTransactionByBlock(vc, d_get_at(d_get(vc->request, K_PARAMS), 0), d_get_int_at(d_get(vc->request, K_PARAMS), 1));
    }
    return IN3_OK;
  }

  // do we support this request?
  if (!method) return vc_err(vc, "No Method in request defined!");

  if (strcmp(method, "eth_getTransactionByHash") == 0)
    return eth_verify_eth_getTransaction(vc, d_get_bytes_at(d_get(vc->request, K_PARAMS), 0));
  else if (!strcmp(method, "eth_getTransactionByBlockHashAndIndex") || !strcmp(method, "eth_getTransactionByBlockNumberAndIndex")) {
    return eth_verify_eth_getTransactionByBlock(vc, d_get_at(d_get(vc->request, K_PARAMS), 0), d_get_int_at(d_get(vc->request, K_PARAMS), 1));
  } else if (strcmp(method, "eth_getBlockByNumber") == 0)
    return eth_verify_eth_getBlock(vc, NULL, d_get_long_at(d_get(vc->request, K_PARAMS), 0));
  else if (strcmp(method, "eth_getBlockTransactionCountByHash") == 0)
    return eth_verify_eth_getBlockTransactionCount(vc, d_get_bytes_at(d_get(vc->request, K_PARAMS), 0), 0);
  else if (strcmp(method, "eth_getBlockTransactionCountByNumber") == 0)
    return eth_verify_eth_getBlockTransactionCount(vc, NULL, d_get_long_at(d_get(vc->request, K_PARAMS), 0));
  else if (strcmp(method, "eth_getBlockByHash") == 0)
    return eth_verify_eth_getBlock(vc, d_get_bytes_at(d_get(vc->request, K_PARAMS), 0), 0);
  else if (strcmp(method, "eth_getBalance") == 0 || strcmp(method, "eth_getCode") == 0 || strcmp(method, "eth_getStorageAt") == 0 || strcmp(method, "eth_getTransactionCount") == 0)
    return eth_verify_account_proof(vc);
  else if (strcmp(method, "eth_gasPrice") == 0)
    return IN3_OK;
  else if (!strcmp(method, "eth_newFilter") || !strcmp(method, "eth_newBlockFilter") || !strcmp(method, "eth_newPendingFilter") || !strcmp(method, "eth_uninstallFilter") || !strcmp(method, "eth_getFilterChanges"))
    return IN3_OK;
  else if (strcmp(method, "eth_getLogs") == 0) // for txReceipt, we need the txhash
    return eth_verify_eth_getLog(vc, d_len(vc->result));
  else if (strcmp(method, "eth_sendRawTransaction") == 0) {
    bytes32_t hash;
    sha3_to(d_get_bytes_at(d_get(vc->request, K_PARAMS), 0), hash);
    return bytes_cmp(*d_bytes(vc->result), bytes(hash, 32)) ? IN3_OK : vc_err(vc, "the transactionHash of the response does not match the raw transaction!");
  } else
    return in3_verify_eth_nano(vc);
}

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
static in3_ret_t get_from_nodes(in3_ctx_t* parent, char* method, char* params, bytes_t* dst) {
  // check if the method is already existing
  in3_ctx_t* ctx = ctx_find_required(parent, method);
  if (ctx) {
    // found one - so we check if it is useable.
    switch (in3_ctx_state(ctx)) {
      // in case of an error, we report it back to the parent context
      case CTX_ERROR:
        return ctx_set_error(parent, ctx->error, IN3_EUNKNOWN);
      // if we are still waiting, we stop here and report it.
      case CTX_WAITING_FOR_REQUIRED_CTX:
      case CTX_WAITING_FOR_RESPONSE:
        return IN3_WAITING;

      // if it is useable, we can now handle the result.
      case CTX_SUCCESS: {
        d_token_t* r = d_get(ctx->responses[0], K_RESULT);
        if (r) {
          // we have a result, so write it back to the dst
          *dst = d_to_bytes(r);
          return IN3_OK;
        } else
          // or check the error and report it
          return ctx_check_response_error(parent, 0);
      }
    }
  }

  // no required context found yet, so we create one:

  // since this is a subrequest it will be freed when the parent is freed.
  // allocate memory for the request-string
  char* req = _malloc(strlen(method) + strlen(params) + 200);
  // create it
  sprintf(req, "{\"method\":\"%s\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":%s}", method, params);
  // and add the request context to the parent.
  return ctx_add_required(parent, ctx_new(parent->client, req));
}

bytes_t sign_tx(d_token_t* tx, in3_ctx_t* ctx) {
  address_t   from;
  bytes_t     tmp;
  uint8_t     sig[65];
  json_ctx_t* new_json = NULL;

  if (ctx->client->signer && ctx->client->signer->prepare_tx) {
    in3_ret_t r = ctx->client->signer->prepare_tx(ctx, tx, &new_json);
    if (r != IN3_OK) {
      if (new_json) json_free(new_json);
      ctx_set_error(ctx, "error tryting to prepare the tx", r);
      return bytes(NULL, 0);
    }
    tx = new_json->result;
  }

  // get the from-address
  if ((tmp = d_to_bytes(d_getl(tx, K_FROM, 20))).len == 0) {
    if (!d_get(tx, K_NONCE)) {
      // Derive the from-address from pk if no nonce is given.
      // Note: This works because the signer->wallet points to the pk in the current signer implementation
      // (see eth_set_pk_signer()), and may change in the future.
      // Also, other wallet implementations may differ - hence the check.
#if defined(LEDGER_NANO)
      if (!ctx->client->signer || (ctx->client->signer->sign != eth_sign_pk_ctx && ctx->client->signer->sign != eth_ledger_sign)) {
#else
      if (!ctx->client->signer || ctx->client->signer->sign != eth_sign_pk_ctx) {
#endif
        if (new_json) json_free(new_json);
        ctx_set_error(ctx, "you need to specify the from-address in the tx!", IN3_EINVAL);
        return bytes(NULL, 0);
      }

      uint8_t public_key[65], sdata[32];
      bytes_t pubkey_bytes = {.data = public_key + 1, .len = 64};

#if defined(LEDGER_NANO)
      if (ctx->client->signer->sign == eth_ledger_sign) {
        uint8_t bip32[32];
        memcpy(bip32, ctx->client->signer->wallet, 5);
        eth_ledger_get_public_key(bip32, public_key);

      } else {
        ecdsa_get_public_key65(&secp256k1, ctx->client->signer->wallet, public_key);
      }
#else
      ecdsa_get_public_key65(&secp256k1, ctx->client->signer->wallet, public_key);
#endif
      sha3_to(&pubkey_bytes, sdata);
      memcpy(from, sdata + 12, 20);
    } else
      memset(from, 0, 20);
  } else
    memcpy(from, tmp.data, 20);

  // build nonce-params
  tmp = bytes(from, 20);

  // read the values
  bytes_t gas_limit = d_get(tx, K_GAS) ? get(tx, K_GAS) : (d_get(tx, K_GAS_LIMIT) ? get(tx, K_GAS_LIMIT) : bytes((uint8_t*) "\x52\x08", 2)),
          to        = getl(tx, K_TO, 20),
          value     = get(tx, K_VALUE),
          data      = get(tx, K_DATA),
          nonce     = get(tx, K_NONCE),
          gas_price = get(tx, K_GAS_PRICE);

  in3_ret_t res = IN3_OK;
  if (!nonce.data) {
    sb_t* sb = sb_new("[");
    sb_add_bytes(sb, "", &tmp, 1, false);
    sb_add_chars(sb, ",\"latest\"]");
    in3_ret_t ret = get_from_nodes(ctx, "eth_getTransactionCount", sb->data, &nonce);
    sb_free(sb);
    if (ret < 0) res = ret;
  }
  if (!gas_price.data) {
    in3_ret_t ret = get_from_nodes(ctx, "eth_gasPrice", "[]", &gas_price);
    if (ret == IN3_WAITING)
      res = (res == IN3_WAITING || res == IN3_OK) ? ret : res;
    else if (ret != IN3_OK)
      res = ret;
  }
  if (res < 0) {
    if (new_json) json_free(new_json);
    ctx_set_error(ctx, "error preparing the tx", res);
    return bytes(NULL, 0);
  }

  uint64_t v = ctx->requests_configs->chain_id ? ctx->requests_configs->chain_id : ctx->client->chain_id;
  if (v > 0xFF) v = 0; // this is only valid for ethereum chains.

  // create raw without signature
  bytes_t* raw = serialize_tx_raw(nonce, gas_price, gas_limit, to, value, data, v, bytes(NULL, 0), bytes(NULL, 0));

  // this tells the static code analyser, that sig will not have gargabe values
  // since this will only be called if the sign-ctx was successfull.
#ifdef __clang_analyzer__
  memset(sig, 0, 65);
#endif

  bytes_t* nonce_cpy     = b_dup(&nonce);
  bytes_t* gas_price_cpy = b_dup(&gas_price);

  // sign the raw message
  if (nonce.data && gas_price.data && gas_limit.data) {
    in3_ctx_t* c = ctx_find_required(ctx, "sign_ec_hash");
    if (c)
      switch (in3_ctx_state(c)) {
        case CTX_ERROR:
          res = ctx_set_error(ctx, c->error, IN3_EUNKNOWN);
          break;
        case CTX_WAITING_FOR_REQUIRED_CTX:
        case CTX_WAITING_FOR_RESPONSE:
          res = IN3_WAITING;
          break;
        case CTX_SUCCESS: {
          if (c->raw_response && c->raw_response->result.len == 65) {
            memcpy(sig, c->raw_response->result.data, 65);
            res = IN3_OK;
          } else if (c->raw_response)
            res = ctx_set_error(ctx, c->raw_response->error.data, IN3_EINVAL);
          else
            res = ctx_set_error(ctx, "no data to sign", IN3_EINVAL);
          ctx_remove_required(ctx, c);
          break;
        }
      }
    else {
      bytes_t from_b = bytes(from, 20);
      sb_t*   req    = sb_new("{\"method\":\"sign_ec_hash\",\"params\":[");
      sb_add_bytes(req, NULL, raw, 1, false);
      sb_add_chars(req, ",");
      sb_add_bytes(req, NULL, &from_b, 1, false);
      sb_add_chars(req, "]}");
      c       = ctx_new(ctx->client, req->data);
      c->type = CT_SIGN;
      res     = ctx_add_required(ctx, c);
      _free(req); // we only free the builder, but  not the data
    }
  } else
    res = IN3_EINVAL;

  // free temp resources
  if (ctx->verification_state != IN3_WAITING && new_json) json_free(new_json);
  b_free(raw);
  if (res < 0) {
    b_free(nonce_cpy);
    b_free(gas_price_cpy);
    return bytes(NULL, 0);
  }

  // create raw transaction with signature
  raw            = serialize_tx_raw(*nonce_cpy, *gas_price_cpy, gas_limit, to, value, data, 27 + sig[64] + (v ? (v * 2 + 8) : 0), bytes(sig, 32), bytes(sig + 32, 32));
  bytes_t raw_tx = bytes(raw->data, raw->len);
  _free(raw); // we only free the struct, not the data!
  b_free(nonce_cpy);
  b_free(gas_price_cpy);

  return raw_tx;
}

in3_ret_t eth_handle_intern(in3_ctx_t* ctx, in3_response_t** response) {
  if (ctx->len > 1) return IN3_ENOTSUP; // internal handling is only possible for single requests (at least for now)
  d_token_t* req = ctx->requests[0];

  // check method
  if (strcmp(d_get_stringk(req, K_METHOD), "eth_sendTransaction") == 0) {
    // get the transaction-object
    d_token_t* tx_params = d_get(req, K_PARAMS);
    if (!tx_params || d_type(tx_params + 1) != T_OBJECT) return ctx_set_error(ctx, "invalid params", IN3_EINVAL);

    // sign it.
    bytes_t raw = sign_tx(tx_params + 1, ctx);
    if (!raw.len) {
      switch (in3_ctx_state(ctx->required)) {
        case CTX_ERROR:
          return IN3_EUNKNOWN;
        case CTX_WAITING_FOR_REQUIRED_CTX:
        case CTX_WAITING_FOR_RESPONSE:
          return IN3_WAITING;
        case CTX_SUCCESS:
          return ctx_set_error(ctx, "error signing the transaction", IN3_EINVAL);
      }
    }

    // build the RPC-request
    uint64_t id = d_get_longk(req, K_ID);
    sb_t*    sb = sb_new("{ \"jsonrpc\":\"2.0\", \"method\":\"eth_sendRawTransaction\", \"params\":[");
    sb_add_bytes(sb, "", &raw, 1, false);
    sb_add_chars(sb, "]");
    if (id) {
      char tmp[16];

#ifdef __ZEPHYR__
      char bufTmp[21];
      snprintk(tmp, sizeof(tmp), ", \"id\":%s", u64_to_str(id, bufTmp, sizeof(bufTmp)));
#else
      snprintf(tmp, sizeof(tmp), ", \"id\":%" PRId64 "", id);
      // sprintf(tmp, ", \"id\":%" PRId64 "", id);
#endif
      sb_add_chars(sb, tmp);
    }
    sb_add_chars(sb, "}");

    // now that we included the signature in the rpc-request, we can free it + the old rpc-request.
    _free(raw.data);
    json_free(ctx->request_context);

    // set the new RPC-Request.
    ctx->request_context = parse_json(sb->data);
    ctx->requests[0]     = ctx->request_context->result;
    in3_cache_add_ptr(&ctx->cache, sb->data); // we add the request-string to the cache, to make sure the request-string will be cleaned afterwards
    _free(sb);                                // and we only free the stringbuilder, but not the data itself.
  } else if (strcmp(d_get_stringk(req, K_METHOD), "eth_newFilter") == 0) {
    d_token_t* tx_params = d_get(req, K_PARAMS);
    if (!tx_params || d_type(tx_params + 1) != T_OBJECT)
      return ctx_set_error(ctx, "invalid type of params, expected object", IN3_EINVAL);
    else if (!filter_opt_valid(tx_params + 1))
      return ctx_set_error(ctx, "filter option parsing failed", IN3_EINVAL);
    if (!tx_params->data) return ctx_set_error(ctx, "binary request are not supported!", IN3_ENOTSUP);

    char*     fopt = d_create_json(tx_params + 1);
    in3_ret_t res  = filter_add(ctx->client, FILTER_EVENT, fopt);
    if (res < 0) {
      _free(fopt);
      return ctx_set_error(ctx, "filter creation failed", res);
    }

    RESPONSE_START();
    sb_add_char(&response[0]->result, '"');
    sb_add_hexuint(&response[0]->result, res);
    sb_add_char(&response[0]->result, '"');
    RESPONSE_END();
    return IN3_OK;
  } else if (strcmp(d_get_stringk(req, K_METHOD), "eth_chainId") == 0) {
    RESPONSE_START();
    sb_add_char(&response[0]->result, '"');
    sb_add_hexuint(&response[0]->result, ctx->client->chain_id);
    sb_add_char(&response[0]->result, '"');
    RESPONSE_END();
  } else if (strcmp(d_get_stringk(req, K_METHOD), "eth_newBlockFilter") == 0) {
    in3_ret_t res = filter_add(ctx->client, FILTER_BLOCK, NULL);
    if (res < 0) return ctx_set_error(ctx, "filter creation failed", res);

    RESPONSE_START();
    sb_add_char(&response[0]->result, '"');
    sb_add_hexuint(&response[0]->result, res);
    sb_add_char(&response[0]->result, '"');
    RESPONSE_END();
  } else if (strcmp(d_get_stringk(req, K_METHOD), "eth_newPendingTransactionFilter") == 0) {
    return ctx_set_error(ctx, "pending filter not supported", IN3_ENOTSUP);
  } else if (strcmp(d_get_stringk(req, K_METHOD), "eth_uninstallFilter") == 0) {
    d_token_t* tx_params = d_get(req, K_PARAMS);
    if (!tx_params || d_len(tx_params) == 0 || d_type(tx_params + 1) != T_INTEGER)
      return ctx_set_error(ctx, "invalid type of params, expected filter-id as integer", IN3_EINVAL);

    uint64_t id = d_get_long_at(tx_params, 0);
    RESPONSE_START();
    sb_add_chars(&response[0]->result, filter_remove(ctx->client, id) ? "true" : "false");
    RESPONSE_END();
  } else if (strcmp(d_get_stringk(req, K_METHOD), "eth_getFilterChanges") == 0 || strcmp(d_get_stringk(req, K_METHOD), "eth_getFilterLogs") == 0) {
    d_token_t* tx_params = d_get(req, K_PARAMS);
    if (!tx_params || d_len(tx_params) == 0 || d_type(tx_params + 1) != T_INTEGER)
      return ctx_set_error(ctx, "invalid type of params, expected filter-id as integer", IN3_EINVAL);

    uint64_t id = d_get_long_at(tx_params, 0);
    RESPONSE_START();
    in3_ret_t ret = filter_get_changes(ctx, id, &response[0]->result);
    if (ret != IN3_OK)
      return ctx_set_error(ctx, "failed to get filter changes", ret);
    RESPONSE_END();
  }
  return IN3_OK;
}

void in3_register_eth_basic() {
  in3_verifier_t* v = _calloc(1, sizeof(in3_verifier_t));
  v->type           = CHAIN_ETH;
  v->pre_handle     = eth_handle_intern;
  v->verify         = in3_verify_eth_basic;
  in3_register_verifier(v);
}
