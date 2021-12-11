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

#include "../../../core/client/keys.h"
#include "../../../core/client/request_internal.h"
#include "../../../core/util/data.h"
#include "../../../core/util/mem.h"
#include "../../../core/util/utils.h"
#include "../../../third-party/crypto/ecdsa.h"
#include "../../../third-party/crypto/secp256k1.h"
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
static in3_ret_t get_from_nodes(in3_req_t* parent, char* method, char* params, bytes_t* dst) {
  // check if the method is already existing
  in3_req_t* ctx = req_find_required(parent, method, NULL);
  if (ctx) {
    // found one - so we check if it is useable.
    switch (in3_req_state(ctx)) {
      // in case of an error, we report it back to the parent context
      case REQ_ERROR:
        return req_set_error(parent, ctx->error, IN3_EUNKNOWN);
      // if we are still waiting, we stop here and report it.
      case REQ_WAITING_FOR_RESPONSE:
      case REQ_WAITING_TO_SEND:
        return IN3_WAITING;

      // if it is useable, we can now handle the result.
      case REQ_SUCCESS: {
        d_token_t* r = d_get(ctx->responses[0], K_RESULT);
        if (r) {
          // we have a result, so write it back to the dst
          *dst = d_to_bytes(r);
          return IN3_OK;
        }
        else
          // or check the error and report it
          return req_check_response_error(ctx, 0);
      }
    }
  }

  // no required context found yet, so we create one:

  // since this is a subrequest it will be freed when the parent is freed.
  // allocate memory for the request-string
  char* req = _malloc(strlen(method) + strlen(params) + 200);
  // create it
  sprintf(req, "{\"method\":\"%s\",\"jsonrpc\":\"2.0\",\"params\":%s}", method, params);
  // and add the request context to the parent.
  return req_add_required(parent, req_new(parent->client, req));
}

/** gets the from-fied from the tx or ask the signer */
in3_ret_t get_from_address(d_token_t* tx, in3_req_t* ctx, address_t res) {
  d_token_t* t = d_get(tx, K_FROM);
  if (t) {
    // we only accept valid from addresses which need to be 20 bytes
    if (d_type(t) != T_BYTES || d_len(t) != 20) return req_set_error(ctx, "invalid from address in tx", IN3_EINVAL);
    memcpy(res, d_bytes(t)->data, 20);
    return IN3_OK;
  }

  // if it is not specified, we rely on the from-address of the signer.
  if (!in3_plugin_is_registered(ctx->client, PLGN_ACT_SIGN_ACCOUNT)) return req_set_error(ctx, "missing from address in tx", IN3_EINVAL);

  // find the first account which is able to sign
  in3_sign_account_ctx_t actx = {.req = ctx, .accounts = NULL, .accounts_len = 0};
  TRY(in3_plugin_execute_first(ctx, PLGN_ACT_SIGN_ACCOUNT, &actx))
  if (!actx.accounts || !actx.accounts_len) return req_set_error(ctx, "no from address found", IN3_EINVAL);
  memcpy(res, actx.accounts, 20);
  _free(actx.accounts);
  return IN3_OK;
}

static in3_ret_t merge_result(in3_ret_t* prev, in3_ret_t res) {
  if (res == IN3_WAITING)
    *prev = (*prev == IN3_WAITING || *prev == IN3_OK) ? IN3_WAITING : *prev;
  else if (res != IN3_OK)
    *prev = res;
  return res;
}

/** checks if the nonce and gas is set  or fetches it from the nodes */
static in3_ret_t get_nonce_and_gasprice(eth_tx_data_t* tx, in3_req_t* ctx) {
  d_token_t* result;
  in3_ret_t  ret = IN3_OK;
  if (!tx->nonce.data) {
    char* payload = sprintx("[\"%B\",\"latest\"]", bytes(tx->from, 20));
    ret           = get_from_nodes(ctx, "eth_getTransactionCount", payload, &tx->nonce);
    _free(payload);
  }

  // fix gas_price
  if (tx->type < 2 && !tx->gas_price.data)
    merge_result(&ret, get_from_nodes(ctx, "eth_gasPrice", "[]", &tx->gas_price));

  // fill access_list if this is a call
  if (tx->type > 0 && tx->data.len >= 4 && !tx->access_list) {
    sb_t       sb = {0};
    in3_req_t* child;
    sb_printx(&sb, "{\"data\":\"%B\",\"from\":\"%B\"", tx->data, bytes(tx->from, 20));
    if (tx->to.data) sb_printx(&sb, ",\"to\":\"%B\"", tx->to);
    sb_add_chars(&sb, "}");
    if (merge_result(&ret, req_send_sub_request(ctx, "eth_call", sb.data, "{\"verification\":\"proof\"}", &result, &child)) == IN3_OK)
      tx->access_list = child ? d_get(d_get(d_get(child->responses[0], K_IN3), K_PROOF), K_ACCOUNTS) : NULL;
    _free(sb.data);
  }

  // fill priority_gas
  if (tx->type == 2 && (!tx->max_fee_per_gas.data || !tx->max_priority_fee_per_gas.data) && merge_result(&ret, req_send_sub_request(ctx, "eth_feeHistory", "\"0x4\",\"latest\",[50]", NULL, &result, NULL)) == IN3_OK) {
    d_token_t* fees     = d_get(result, K_BASE_GAS_FEE);
    uint64_t   base_fee = 0;
    uint64_t   prio_fee = 0;
    if (fees && d_len(fees) && d_type(fees) == T_ARRAY)
      base_fee = d_get_long_at(fees, d_len(fees) - 1);
    fees = d_get(result, key("reward"));
    if (fees && d_len(fees) && d_type(fees) == T_ARRAY) {
      for (int i = d_len(fees) - 1; i >= 0 && tx->max_priority_fee_per_gas.len < 2; i--)
        tx->max_priority_fee_per_gas = d_to_bytes(d_get_at(d_get_at(fees, i), 0));
      prio_fee = tx->max_priority_fee_per_gas.data ? bytes_to_long(tx->max_priority_fee_per_gas.data, tx->max_priority_fee_per_gas.len) : 0;
    }
    if (!prio_fee) return req_set_error(ctx, "Could not determine the max priority fees!", IN3_EFIND);
    uint64_t max_fees = ((base_fee + prio_fee) * 14) / 10; // we add 40% buffer
    uint8_t  tmp[8];
    bytes_t  fee_data = bytes(tmp, 8);
    long_to_bytes(max_fees, tmp);
    b_optimize_len(&fee_data);
    bytes_t* cached     = in3_cache_get_entry(ctx->cache, &fee_data);
    tx->max_fee_per_gas = cached ? *cached : in3_cache_add_entry(&ctx->cache, bytes_dup(fee_data), bytes_dup(fee_data))->value;
  }

  return ret;
}

/** gets the v-value from the chain_id */
static inline uint64_t get_v(chain_id_t chain) {
  uint64_t v = chain;
  if (v > 0xFF && v != 1337) v = 0; // this is only valid for ethereum chains.
  return v;
}

/** creates a memory in heap which will automatilcy be freed when the context is freed.
 * if called twice with the same key, the same memory is returned
 */
static bytes_t get_or_create_cached(in3_req_t* req, d_key_t k, int size) {
  cache_props_t  p     = (((uint32_t) k) << 16) | CACHE_PROP_MUST_FREE;
  cache_entry_t* cache = in3_cache_get_entry_by_prop(req->cache, p);
  if (!cache) {
    cache        = in3_cache_add_entry(&req->cache, NULL_BYTES, bytes(_calloc(1, size), size));
    cache->props = p;
  }
  return cache->value;
}

static in3_ret_t transform_erc20(in3_req_t* req, d_token_t* tx, bytes_t* to, bytes_t* value, bytes_t* data, bytes_t* gas_limit) {
  char* token = d_get_string(tx, key("token"));
  if (token && token[0] == '0' && token[1] == 'x' && strlen(token) == 42) {
    if (to->len != 20) return req_set_error(req, "Invalid to address!", IN3_EINVAL);
    *data = get_or_create_cached(req, key("pdata"), 68);
    memcpy(data->data, "\xa9\x05\x9c\xbb", 4);                         // transfer (address, uint256)
    memcpy(data->data + 4 + 32 - 20, to->data, 20);                    // recipient
    memcpy(data->data + 4 + 64 - value->len, value->data, value->len); // value

    *to = get_or_create_cached(req, key("pto"), 20);
    hex_to_bytes(token, -1, to->data, to->len);

    uint64_t gas = bytes_to_long(gas_limit->data, gas_limit->len) + 100000; // we add 100000 gas for using transfer
    *gas_limit   = get_or_create_cached(req, key("pgas"), 8);
    long_to_bytes(gas, gas_limit->data);
    b_optimize_len(gas_limit);

    value->len = 0; // we don't need a value anymore, since it is encoded
  }
  else if (token)
    return req_set_error(req, "Invalid Token. Only token-addresses are supported!", IN3_EINVAL);

  return IN3_OK;
}

static in3_ret_t transform_abi(in3_req_t* req, d_token_t* tx, bytes_t* data) {
  char* fn = d_get_string(tx, key("fn_sig"));

  if (fn) {
    d_token_t* args = d_get(tx, key("fn_args"));
    if (args && d_type(args) != T_ARRAY) return req_set_error(req, "Invalid argument type for tx", IN3_EINVAL);

    sb_t params = {0};
    sb_add_char(&params, '\"');
    sb_add_chars(&params, fn);

    if (args)
      sb_add_json(&params, "\",", args);
    else
      sb_add_chars(&params, "\",[]");

    d_token_t* res;
    TRY_FINAL(req_send_sub_request(req, "in3_abiEncode", params.data, NULL, &res, NULL), _free(params.data))

    if (d_type(res) != T_BYTES || d_len(res) < 4) return req_set_error(req, "abi encoded data", IN3_EINVAL);
    if (data->data) {
      // if this is a deployment transaction we concate it with the arguments without the functionhash
      bytes_t new_data = get_or_create_cached(req, key("deploy_data"), data->len + d_len(res) - 4);
      memcpy(new_data.data, data->data, data->len);
      memcpy(new_data.data + data->len, d_bytes(res)->data + 4, d_len(res) - 4);
      *data = new_data;
    }
    else
      *data = d_to_bytes(res);
  }

  return IN3_OK;
}
/** based on the tx-entries the transaction is manipulated before creating the raw transaction. */
static in3_ret_t transform_tx(in3_req_t* req, d_token_t* tx, bytes_t* to, bytes_t* value, bytes_t* data, bytes_t* gas_limit) {
  // do we need to convert to the ERC20.transfer function?
  TRY(transform_erc20(req, tx, to, value, data, gas_limit))
  TRY(transform_abi(req, tx, data))
  return IN3_OK;
}

/**
 * prepares a transaction and writes the data to the dst-bytes. In case of success, you MUST free only the data-pointer of the dst.
 */
in3_ret_t eth_prepare_unsigned_tx(d_token_t* tx, in3_req_t* ctx, bytes_t* dst, sb_t* meta) {
  eth_tx_data_t td = {0};

  // read the values
  td.type                     = d_get_int(tx, d_get(tx, K_ETH_TX_TYPE) ? K_ETH_TX_TYPE : K_TYPE);
  td.access_list              = d_get(tx, K_ACCESS_LIST);
  td.gas_limit                = d_get(tx, K_GAS) ? get(tx, K_GAS) : (d_get(tx, K_GAS_LIMIT) ? get(tx, K_GAS_LIMIT) : bytes((uint8_t*) "\x9c\x40", 2)); // default: 40000 gas
  td.to                       = getl(tx, K_TO, 20);
  td.value                    = get(tx, K_VALUE);
  td.data                     = get(tx, K_DATA);
  td.nonce                    = get(tx, K_NONCE);
  td.gas_price                = get(tx, K_GAS_PRICE);
  td.max_fee_per_gas          = get(tx, K_MAX_FEE_PER_GAS);
  td.max_priority_fee_per_gas = get(tx, K_MAX_PRIORITY_FEE_PER_GAS);

  // make sure, we have the correct chain_id
  chain_id_t chain_id = in3_chain_id(ctx);
  if (chain_id == CHAIN_ID_LOCAL) {
    d_token_t* r = NULL;
    TRY(req_send_sub_request(ctx, "eth_chainId", "", NULL, &r, NULL))
    chain_id = d_long(r);
  }
  TRY(get_from_address(tx, ctx, td.from))

  // write state?
  if (meta) {
    sb_add_rawbytes(meta, "\"input\":{\"to\":\"0x", td.to, 0);
    sb_add_rawbytes(meta, "\",\"sender\":\"0x", bytes(td.from, 20), 0);
    sb_add_rawbytes(meta, "\",\"value\":\"0x", td.value, 0);
    sb_add_rawbytes(meta, "\",\"data\":\"0x", td.data, 0);
    sb_add_rawbytes(meta, "\",\"gas\":\"0x", td.gas_limit, 0);
    sb_add_rawbytes(meta, "\",\"gasPrice\":\"0x", td.gas_price, 0);
    sb_add_rawbytes(meta, "\",\"nonce\":\"0x", td.nonce, 0);
    sb_add_chars(meta, "\",\"eth_tx_type\":");
    sb_add_int(meta, (int64_t) td.type);
    if (td.max_fee_per_gas.data) sb_add_rawbytes(meta, "\",\"maxFeePerGas\":\"0x", td.max_fee_per_gas, -1);
    if (td.max_priority_fee_per_gas.data) sb_add_rawbytes(meta, "\",\"maxPriorityFeePerGas\":\"0x", td.max_fee_per_gas, -1);
    sb_add_json(meta, ",\"accessList\":", td.access_list);
    sb_add_chars(meta, "\",\"layer\":\"l1\"");
    sb_add_json(meta, ",\"fn_sig\":", d_get(tx, key("fn_sig")));
    sb_add_json(meta, ",\"fn_args\":", d_get(tx, key("fn_args")));
    sb_add_json(meta, ",\"token\":", d_get(tx, key("token")));
    sb_add_json(meta, ",\"wallet\":", d_get(tx, key("wallet")));
    sb_add_json(meta, ",\"url\":", d_get(tx, key("url")));
    sb_add_json(meta, ",\"delegate\":", d_get(tx, key("delegate")));
  }

  // do we need to transform the tx before we sign it?
  TRY(transform_tx(ctx, tx, &td.to, &td.value, &td.data, &td.gas_limit));
  TRY(get_nonce_and_gasprice(&td, ctx))

  // create raw without signature
  bytes_t* raw = serialize_tx_raw(&td, chain_id, td.type ? 0 : get_v(chain_id), NULL_BYTES, NULL_BYTES);
  *dst         = *raw;
  _free(raw);

  // write state?
  if (meta) {
    sb_add_rawbytes(meta, "},\"pre_unsigned\":\"0x", *dst, 0);
    sb_add_chars(meta, "\"");
  }

  // do we need to change it?
  if (in3_plugin_is_registered(ctx->client, PLGN_ACT_SIGN_PREPARE)) {
    in3_sign_prepare_ctx_t pctx = {.req = ctx, .old_tx = *dst, .new_tx = {0}, .output = meta, .tx = tx};
    memcpy(pctx.account, td.from, 20);
    in3_ret_t prep_res = in3_plugin_execute_first_or_none(ctx, PLGN_ACT_SIGN_PREPARE, &pctx);

    if (prep_res) {
      if (dst->data) _free(dst->data);
      if (pctx.new_tx.data) _free(pctx.new_tx.data);
      return prep_res;
    }
    else if (pctx.new_tx.data) {
      if (dst->data) _free(dst->data);
      *dst = pctx.new_tx;
    }
  }

  if (meta) {
    sb_add_rawbytes(meta, ",\"unsigned\":\"0x", *dst, 0);
    sb_add_chars(meta, "\"");
  }

  // cleanup subcontexts
  TRY(req_remove_required(ctx, req_find_required(ctx, "eth_getTransactionCount", NULL), false))
  TRY(req_remove_required(ctx, req_find_required(ctx, "eth_gasPrice", NULL), false))

  return IN3_OK;
}

/**
 * signs a unsigned raw transaction and writes the raw data to the dst-bytes. In case of success, you MUST free only the data-pointer of the dst.
 */
in3_ret_t eth_sign_raw_tx(bytes_t raw_tx, in3_req_t* ctx, address_t from, bytes_t* dst) {
  bytes_t signature;

  // make sure, we have the correct chain_id
  chain_id_t chain_id = in3_chain_id(ctx);
  if (chain_id == CHAIN_ID_LOCAL) {
    d_token_t* r = NULL;
    TRY(req_send_sub_request(ctx, "eth_chainId", "", NULL, &r, NULL))
    chain_id = d_long(r);
  }

  TRY(req_require_signature(ctx, SIGN_EC_HASH, PL_SIGN_ETHTX, &signature, raw_tx, bytes(from, 20), ctx->requests[0]));
  if (signature.len != 65) return req_set_error(ctx, "Transaction must be signed by a ECDSA-Signature!", IN3_EINVAL);

  // get the signature from required

  // if we reached that point we have a valid signature in sig
  // create raw transaction with signature
  uint8_t  type = raw_tx.len && raw_tx.data[0] < 10 ? raw_tx.data[0] : 0;
  bytes_t  data, last;
  uint32_t v         = type ? signature.data[64] : (27 + signature.data[64] + (get_v(chain_id) ? (get_v(chain_id) * 2 + 8) : 0));
  int      last_item = 5;
  if (type) {
    raw_tx.data++;
    raw_tx.len--;
    if (type == 1) last_item = 7;
    if (type == 1) last_item = 8;
  }
  EXPECT_EQ(rlp_decode(&raw_tx, 0, &data), 2)                           // the raw data must be a list(2)
  EXPECT_EQ(rlp_decode(&data, last_item, &last), (type ? 2 : 1))        // the last element (data) must be an item (1) for type=0 otherwise it is a list (accessList)
  bytes_builder_t* rlp = bb_newl(raw_tx.len + 68);                      // we try to make sure, we don't have to reallocate
  bb_write_raw_bytes(rlp, data.data, last.data + last.len - data.data); // copy the existing data without signature

  // add v
  uint8_t vdata[sizeof(v)];
  data = bytes(vdata, sizeof(vdata));
  int_to_bytes(v, vdata);
  b_optimize_len(&data);
  rlp_encode_item(rlp, &data);

  // add r
  data = bytes(signature.data, 32);
  b_optimize_len(&data);
  rlp_encode_item(rlp, &data);

  // add s
  data = bytes(signature.data + 32, 32);
  b_optimize_len(&data);
  rlp_encode_item(rlp, &data);

  // finish up
  rlp_encode_to_list(rlp);
  if (type) bb_replace(rlp, 0, 0, &type, 1); // we insert the type
  *dst = rlp->b;

  _free(rlp);
  return IN3_OK;
}

/** handle the sendTransaction internally */
in3_ret_t handle_eth_sendTransaction(in3_req_t* ctx, d_token_t* req) {
  // get the transaction-object
  d_token_t* tx_params   = d_get(req, K_PARAMS);
  bytes_t    unsigned_tx = NULL_BYTES, signed_tx = NULL_BYTES;
  address_t  from;
  if (!tx_params || d_type(tx_params + 1) != T_OBJECT) return req_set_error(ctx, "invalid params", IN3_EINVAL);

  TRY(get_from_address(tx_params + 1, ctx, from));

  // is there a pending signature?
  // we get the raw transaction from this request
  in3_req_t* sig_ctx = req_find_required(ctx, "sign_ec_hash", NULL);
  if (sig_ctx) {
    bytes_t raw = *d_get_bytes_at(d_get(sig_ctx->requests[0], K_PARAMS), 0);
    unsigned_tx = bytes(_malloc(raw.len), raw.len);
    memcpy(unsigned_tx.data, raw.data, raw.len);
  }
  else
    TRY(eth_prepare_unsigned_tx(tx_params + 1, ctx, &unsigned_tx, NULL));
  TRY_FINAL(eth_sign_raw_tx(unsigned_tx, ctx, from, &signed_tx),
            if (unsigned_tx.data) _free(unsigned_tx.data);)

  // build the RPC-request
  char* old_req = ctx->request_context->c;
  sb_t  sb      = {0};
  sb_add_rawbytes(&sb, "{ \"jsonrpc\":\"2.0\", \"method\":\"eth_sendRawTransaction\", \"params\":[\"0x", signed_tx, 0);
  sb_add_chars(&sb, "\"]");
  sb_add_chars(&sb, "}");

  // now that we included the signature in the rpc-request, we can free it + the old rpc-request.
  _free(signed_tx.data);
  json_free(ctx->request_context);

  // set the new RPC-Request.
  ctx->request_context                           = parse_json(sb.data);
  ctx->requests[0]                               = ctx->request_context->result;
  in3_cache_add_ptr(&ctx->cache, sb.data)->props = CACHE_PROP_MUST_FREE | CACHE_PROP_ONLY_EXTERNAL;     // we add the request-string to the cache, to make sure the request-string will be cleaned afterwards
  in3_cache_add_ptr(&ctx->cache, old_req)->props = CACHE_PROP_MUST_FREE | CACHE_PROP_ONLY_NOT_EXTERNAL; // we add the request-string to the cache, to make sure the request-string will be cleaned afterwards, butt only for subrequests
  return IN3_OK;
}

/** minimum signer for the wallet, returns the signed message which needs to be freed **/
char* eth_wallet_sign(const char* key, const char* data) {
  int     data_l = strlen(data) / 2 - 1;
  uint8_t key_bytes[32], *data_bytes = alloca(data_l + 1), dst[65];

  hex_to_bytes((char*) key + 2, -1, key_bytes, 32);
  data_l    = hex_to_bytes((char*) data + 2, -1, data_bytes, data_l + 1);
  char* res = _malloc(133);

  if (ecdsa_sign(&secp256k1, HASHER_SHA3K, key_bytes, data_bytes, data_l, dst, dst + 64, NULL) >= 0) {
    bytes_to_hex(dst, 65, res + 2);
    res[0] = '0';
    res[1] = 'x';
  }

  return res;
}
