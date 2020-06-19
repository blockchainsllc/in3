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

#include "../../../core/client/context_internal.h"
#include "../../../core/client/keys.h"
#include "../../../core/util/data.h"
#include "../../../core/util/mem.h"
#include "../../../core/util/utils.h"
#include "../../../verifier/eth1/basic/filter.h"
#include "../../../verifier/eth1/nano/eth_nano.h"
#include "../../../verifier/eth1/nano/merkle.h"
#include "../../../verifier/eth1/nano/rlp.h"
#include "../../../verifier/eth1/nano/serialize.h"
#include "eth_basic.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/** helper to get a key and convert it to bytes*/
static inline bytes_t get(d_token_t* t, uint16_t key) {
  return d_to_bytes(d_get(t, key));
}
/** helper to get a key and convert it to bytes with a specified length*/
static inline bytes_t getl(d_token_t* t, uint16_t key, size_t l) {
  return d_to_bytes(d_getl(t, key, l));
}

/**  return data from the client.*/
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

/** gets the from-fied from the tx or ask the signer */
static in3_ret_t get_from_address(d_token_t* tx, in3_ctx_t* ctx, address_t res) {
  d_token_t* t = d_get(tx, K_FROM);
  if (t) {
    // we only accept valid from addresses which need to be 20 bytes
    if (d_type(t) != T_BYTES || d_len(t) != 20) return ctx_set_error(ctx, "invalid from address in tx", IN3_EINVAL);
    memcpy(res, d_bytes(t)->data, 20);
    return IN3_OK;
  }

  // if it is not specified, we rely on the from-address of the signer.
  if (!ctx->client->signer) return ctx_set_error(ctx, "missing from address in tx", IN3_EINVAL);

  memcpy(res, ctx->client->signer->default_address, 20);
  return IN3_OK;
}

/** checks if the nonce and gas is set  or fetches it from the nodes */
static in3_ret_t get_nonce_and_gasprice(bytes_t* nonce, bytes_t* gas_price, in3_ctx_t* ctx, address_t from) {
  in3_ret_t ret = IN3_OK;
  if (!nonce->data) {
    bytes_t from_bytes = bytes(from, 20);
    sb_t*   sb         = sb_new("[");
    sb_add_bytes(sb, "", &from_bytes, 1, false);
    sb_add_chars(sb, ",\"latest\"]");
    ret = get_from_nodes(ctx, "eth_getTransactionCount", sb->data, nonce);
    sb_free(sb);
  }
  if (!gas_price->data) {
    in3_ret_t res = get_from_nodes(ctx, "eth_gasPrice", "[]", gas_price);
    if (res == IN3_WAITING)
      ret = (ret == IN3_WAITING || ret == IN3_OK) ? IN3_WAITING : ret;
    else if (res != IN3_OK)
      ret = res;
  }

  return ret;
}

/** adds the request id to the string if none was found it will generate one */
static void add_req_id(sb_t* sb, uint64_t id) {
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
}

/** gets the v-value from the chain_id */
static uint64_t get_v(in3_ctx_t* ctx) {
  uint64_t v = ctx->requests_configs->chain_id ? ctx->requests_configs->chain_id : ctx->client->chain_id;
  if (v > 0xFF) v = 0; // this is only valid for ethereum chains.
  return v;
}

/**
 * prepares a transaction and writes the data to the dst-bytes. In case of success, you MUST free only the data-pointer of the dst. 
 */
in3_ret_t eth_prepare_unsigned_tx(d_token_t* tx, in3_ctx_t* ctx, bytes_t* dst) {
  address_t from;

  // read the values
  bytes_t gas_limit = d_get(tx, K_GAS) ? get(tx, K_GAS) : (d_get(tx, K_GAS_LIMIT) ? get(tx, K_GAS_LIMIT) : bytes((uint8_t*) "\x52\x08", 2)),
          to        = getl(tx, K_TO, 20),
          value     = get(tx, K_VALUE),
          data      = get(tx, K_DATA),
          nonce     = get(tx, K_NONCE),
          gas_price = get(tx, K_GAS_PRICE);

  TRY(get_from_address(tx, ctx, from))
  TRY(get_nonce_and_gasprice(&nonce, &gas_price, ctx, from))

  // create raw without signature
  bytes_t* raw = serialize_tx_raw(nonce, gas_price, gas_limit, to, value, data, get_v(ctx), bytes(NULL, 0), bytes(NULL, 0));
  *dst         = *raw;
  _free(raw);

  // cleanup subcontexts
  TRY(ctx_remove_required(ctx, ctx_find_required(ctx, "eth_getTransactionCount")))
  TRY(ctx_remove_required(ctx, ctx_find_required(ctx, "eth_gasPrice")))

  return IN3_OK;
}

/**
 * signs a unsigned raw transaction and writes the raw data to the dst-bytes. In case of success, you MUST free only the data-pointer of the dst. 
 */
in3_ret_t eth_sign_raw_tx(bytes_t raw_tx, in3_ctx_t* ctx, address_t from, bytes_t* dst) {
  uint8_t sig[65];

  // get the signature from required
  in3_ctx_t* c = ctx_find_required(ctx, "sign_ec_hash");
  if (c)
    switch (in3_ctx_state(c)) {
      case CTX_ERROR:
        return ctx_set_error(ctx, c->error, IN3_ERPC);
      case CTX_WAITING_FOR_REQUIRED_CTX:
      case CTX_WAITING_FOR_RESPONSE:
        return IN3_WAITING;
      case CTX_SUCCESS: {
        if (c->raw_response && c->raw_response->result.len == 65)
          memcpy(sig, c->raw_response->result.data, 65);
        else if (c->raw_response && c->raw_response->error.data)
          return ctx_set_error(ctx, c->raw_response->error.data, IN3_EINVAL);
        else
          return ctx_set_error(ctx, "no data to sign", IN3_EINVAL);
      }
    }
  else {
    bytes_t from_b = bytes(from, 20);
    sb_t*   req    = sb_new("{\"method\":\"sign_ec_hash\",\"params\":[");
    sb_add_bytes(req, NULL, &raw_tx, 1, false);
    sb_add_chars(req, ",");
    sb_add_bytes(req, NULL, &from_b, 1, false);
    sb_add_chars(req, "]}");
    c             = ctx_new(ctx->client, req->data);
    c->type       = CT_SIGN;
    in3_ret_t res = ctx_add_required(ctx, c);
    _free(req); // we only free the builder, but  not the data, because for subcontextes the data will automaticly be freed.
    return res;
  }

  // if we reached that point we have a valid signature in sig
  // create raw transaction with signature
  bytes_t data, last;
  uint8_t v = 27 + sig[64] + (get_v(ctx) ? (get_v(ctx) * 2 + 8) : 0);
  EXPECT_EQ(rlp_decode(&raw_tx, 0, &data), 2)                           // the raw data must be a list(2)
  EXPECT_EQ(rlp_decode(&data, 5, &last), 1)                             // the last element (data) must be an item (1)
  bytes_builder_t* rlp = bb_newl(raw_tx.len + 68);                      // we try to make sure, we don't have to reallocate
  bb_write_raw_bytes(rlp, data.data, last.data + last.len - data.data); // copy the existing data without signature

  // add v
  data = bytes(&v, 1);
  rlp_encode_item(rlp, &data);

  // add r
  data = bytes(sig, 32);
  b_optimize_len(&data);
  rlp_encode_item(rlp, &data);

  // add s
  data = bytes(sig + 32, 32);
  b_optimize_len(&data);
  rlp_encode_item(rlp, &data);

  // finish up
  rlp_encode_to_list(rlp);
  *dst = rlp->b;

  _free(rlp);
  ctx_remove_required(ctx, c);
  return IN3_OK;
}

/** handle the sendTransaction internally */
in3_ret_t handle_eth_sendTransaction(in3_ctx_t* ctx, d_token_t* req) {
  // get the transaction-object
  d_token_t* tx_params   = d_get(req, K_PARAMS);
  bytes_t    unsigned_tx = bytes(NULL, 0), signed_tx = bytes(NULL, 0);
  address_t  from;
  if (!tx_params || d_type(tx_params + 1) != T_OBJECT) return ctx_set_error(ctx, "invalid params", IN3_EINVAL);

  // is there a pending signature?
  // we get the raw transaction from this request
  in3_ctx_t* sig_ctx = ctx_find_required(ctx, "sign_ec_hash");
  if (sig_ctx)
    unsigned_tx = *d_get_bytes_at(d_get(sig_ctx->requests[0], K_PARAMS), 0);

  TRY(get_from_address(tx_params + 1, ctx, from));
  TRY(unsigned_tx.data ? IN3_OK : eth_prepare_unsigned_tx(tx_params + 1, ctx, &unsigned_tx));

  // do we want to modify the transaction?
  if (ctx->client->signer && ctx->client->signer->prepare_tx && !sig_ctx && unsigned_tx.data) {
    bytes_t   new_tx = bytes(NULL, 0);
    in3_ret_t res    = ctx->client->signer->prepare_tx(ctx, unsigned_tx, &new_tx);

    if (res || new_tx.data) _free(unsigned_tx.data);
    TRY(res)

    if (new_tx.data) unsigned_tx = new_tx;
  }

  TRY_FINAL(eth_sign_raw_tx(unsigned_tx, ctx, from, &signed_tx),
            if (!sig_ctx && unsigned_tx.data) _free(unsigned_tx.data);)

  // build the RPC-request
  sb_t* sb = sb_new("{ \"jsonrpc\":\"2.0\", \"method\":\"eth_sendRawTransaction\", \"params\":[");
  sb_add_bytes(sb, "", &signed_tx, 1, false);
  sb_add_chars(sb, "]");
  add_req_id(sb, d_get_longk(req, K_ID));
  sb_add_chars(sb, "}");

  // now that we included the signature in the rpc-request, we can free it + the old rpc-request.
  _free(signed_tx.data);
  json_free(ctx->request_context);

  // set the new RPC-Request.
  ctx->request_context = parse_json(sb->data);
  ctx->requests[0]     = ctx->request_context->result;
  in3_cache_add_ptr(&ctx->cache, sb->data); // we add the request-string to the cache, to make sure the request-string will be cleaned afterwards
  _free(sb);                                // and we only free the stringbuilder, but not the data itself.
  return IN3_OK;
}
