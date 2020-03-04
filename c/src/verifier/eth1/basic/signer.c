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

/** signs the given data */
in3_ret_t eth_sign(void* ctx, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst) {
  UNUSED_VAR(account); // at least for now
  uint8_t* pk = ((in3_ctx_t*) ctx)->client->signer->wallet;
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
  in3->signer             = _malloc(sizeof(in3_signer_t));
  in3->signer->sign       = eth_sign;
  in3->signer->prepare_tx = NULL;
  in3->signer->wallet     = pk;
  return IN3_OK;
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
      if (!ctx->client->signer || ctx->client->signer->sign != eth_sign) {
        if (new_json) json_free(new_json);
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