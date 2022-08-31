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
#include "../../../core/util/crypto.h"
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

// defines the default gas to be used if no gas is specified.
// default: 40000 gas
const uint64_t default_gas = 40000;

/** helper to get a key and convert it to bytes*/
static inline bytes_t get(d_token_t* t, uint16_t key) {
  return d_num_bytes(d_get(t, key));
}
/** helper to get a key and convert it to bytes with a specified length*/
static inline bytes_t getl(d_token_t* t, uint16_t key, size_t l) {
  return d_bytes(d_getl(t, key, l));
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
          *dst = d_bytes(r);
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
  bytes_t t = d_get_bytes(tx, K_FROM);
  if (!t.data) {
    t = d_get_bytes(tx, K_SENDER);
  }
  if (t.data) {
    // we only accept valid from addresses which need to be 20 bytes
    if (t.len != 20) return req_set_error(ctx, "invalid from or sender address in tx", IN3_EINVAL);
    memcpy(res, t.data, 20);
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

  // is the nonce set?
  if (!tx->nonce.data) {
    char* payload = sprintx("[\"%B\",\"latest\"]", bytes(tx->from, 20));
    ret           = get_from_nodes(ctx, "eth_getTransactionCount", payload, &tx->nonce);
    _free(payload);
  }

  if (tx->type < 2) {
    // legacy -tx
    if (!tx->gas_price) {
      in3_ret_t r = req_send_sub_request(ctx, "eth_gasPrice", "", NULL, &result, NULL);
      if (r == IN3_OK) {
        tx->gas_price = d_long(result);
        if (tx->gas_prio) tx->gas_price = (tx->gas_price * tx->gas_prio) / 100;
      }
      merge_result(&ret, r);
    }
  }
  else if (tx->max_fee_per_gas == 0 || tx->max_priority_fee_per_gas == 0) {
    // tx type 2

    // get gas_price
    uint64_t gas_price = merge_result(&ret, req_send_sub_request(ctx, "eth_gasPrice", "", NULL, &result, NULL)) == IN3_OK ? d_long(result) : 0;

    // get latest block
    if (merge_result(&ret, req_send_sub_request(ctx, "eth_getBlockByNumber", "\"latest\",false", NULL, &result, NULL)) == IN3_OK && gas_price) {
      uint64_t base_fee = d_get_long(result, K_BASE_GAS_FEE);
      tx->gas_price     = tx->gas_prio ? (gas_price * tx->gas_prio) / 100 : gas_price;
      if (!base_fee)
        tx->type = 0; // looks like we don't support tx type 2, so we use legacy tx
      else {
        tx->max_priority_fee_per_gas = 1000000000L;
        tx->max_fee_per_gas          = base_fee * 2 + tx->max_priority_fee_per_gas;
      }
    }
  }

  return ret;
}

/** gets the v-value from the chain_id */
static inline uint64_t get_v(chain_id_t chain) {
  uint64_t v = chain;
  if (v == CHAIN_ID_IPFS) v = 0; // this is only valid for ethereum chains.
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

static in3_ret_t transform_erc20(in3_req_t* req, d_token_t* tx, eth_tx_data_t* td) {
  char*   token  = d_get_string(tx, key("token"));
  bytes_t nft_id = d_get_bytes(tx, key("nft_id"));
  if (token && nft_id.data == NULL && token[0] == '0' && token[1] == 'x' && strlen(token) == 42) {
    if (td->to.len != 20) return req_set_error(req, "Invalid to address!", IN3_EINVAL);
    td->data = get_or_create_cached(req, key("pdata"), 68);
    memcpy(td->data.data, "\xa9\x05\x9c\xbb", 4);                                  // transfer (address, uint256)
    memcpy(td->data.data + 4 + 32 - 20, td->to.data, 20);                          // recipient
    memcpy(td->data.data + 4 + 64 - td->value.len, td->value.data, td->value.len); // value

    td->to = get_or_create_cached(req, key("pto"), 20);
    hex_to_bytes(token, -1, td->to.data, td->to.len);

    if (!td->gas_limit) td->gas_limit = default_gas + 100000;
    td->value.len = 0; // we don't need a value anymore, since it is encoded
  }
  else if (token && nft_id.data == NULL)
    return req_set_error(req, "Invalid Token. Only token-addresses are supported!", IN3_EINVAL);

  return IN3_OK;
}

static in3_ret_t transform_erc721(in3_req_t* req, d_token_t* tx, eth_tx_data_t* td) {
  char*   token    = d_get_string(tx, key("token"));
  bytes_t nft_id   = d_get_bytes(tx, key("nft_id"));
  bytes_t nft_from = d_get_bytes(tx, key("nft_from"));
  if (nft_from.data == NULL) nft_from = bytes(td->from, 20);
  if (token && nft_id.data != NULL && token[0] == '0' && token[1] == 'x' && strlen(token) == 42) {
    if (td->to.len != 20) return req_set_error(req, "Invalid to address!", IN3_EINVAL);
    if (nft_from.len != 20) return req_set_error(req, "Invalid to nft_from address!", IN3_EINVAL);
    td->data = get_or_create_cached(req, key("pdata"), 100);
    memcpy(td->data.data, "\x23\xb8\x72\xdd", 4);                         // transferFrom(address,address,uint256)
    memcpy(td->data.data + 4 + 32 - 20, nft_from.data, 20);               // from
    memcpy(td->data.data + 4 + 64 - 20, td->to.data, 20);                 // to
    memcpy(td->data.data + 4 + 96 - nft_id.len, nft_id.data, nft_id.len); // tokenID

    td->to = get_or_create_cached(req, key("pto"), 20);
    hex_to_bytes(token, -1, td->to.data, td->to.len);

    if (!td->gas_limit) td->gas_limit = default_gas + 100000;
    td->value.len = 0; // we don't need a value anymore, since it is encoded
  }
  else if (token && nft_id.data != NULL)
    return req_set_error(req, "Invalid Token. Only token-addresses are supported!", IN3_EINVAL);

  return IN3_OK;
}

static in3_ret_t transform_abi(in3_req_t* req, d_token_t* tx, eth_tx_data_t* td) {
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
    if (td->data.len) {
      // if this is a deployment transaction we concate it with the arguments without the functionhash
      bytes_t new_data = get_or_create_cached(req, key("deploy_data"), td->data.len + d_len(res) - 4);
      memcpy(new_data.data, td->data.data, td->data.len);
      memcpy(new_data.data + td->data.len, d_bytes(res).data + 4, d_len(res) - 4);
      td->data = new_data;
    }
    else
      td->data = d_bytes(res);
  }

  return IN3_OK;
}
/** based on the tx-entries the transaction is manipulated before creating the raw transaction. */
static in3_ret_t transform_tx(in3_req_t* req, d_token_t* tx, eth_tx_data_t* td) {
  // do we need to convert to the ERC20.transfer function?
  TRY(transform_erc20(req, tx, td))
  TRY(transform_erc721(req, tx, td))
  TRY(transform_abi(req, tx, td))
  return IN3_OK;
}

static in3_ret_t simulate_tx(in3_req_t* req, eth_tx_data_t* tx, bytes_t wallet, d_token_t** result, char* method, uint64_t init_gas) {
  sb_t sb = {.allocted = 100, .data = _malloc(100), .len = 0};
  sb_printx(&sb, "{\"gas\":\"0x%x\",\"data\":\"%B\",\"from\":\"%B\"", init_gas, tx->data, wallet.len == 20 ? wallet : bytes(tx->from, 20));
  if (tx->to.data) sb_printx(&sb, ",\"to\":\"%B\"", tx->to);
  sb_add_char(&sb, '}');
  if (strcmp(method, "eth_call") == 0) sb_add_chars(&sb, ",\"latest\"");
  TRY_FINAL(req_send_sub_request(req, method, sb.data, NULL, result, NULL), _free(sb.data))
  return IN3_OK;
}

static in3_ret_t determine_gas(in3_req_t* req, eth_tx_data_t* tx, bytes_t wallet) {
  if (tx->gas_limit) return IN3_OK;
  if (tx->data.len > 3) {
    uint64_t   init_gas = 20000000;
    d_token_t* result;
    TRY(simulate_tx(req, tx, wallet, &result, "eth_estimateGas", init_gas))
    uint64_t gas = d_long(result);
    if (gas == init_gas) return req_set_error(req, "The transaction would fail if being send this way", IN3_EINVAL);
    tx->gas_limit = (d_long(result) * 12) / 10; // we add 20% buffer
  }
  else
    tx->gas_limit = default_gas;
  return IN3_OK;
}

// print out the tx-input
static in3_ret_t print_input(sb_t* meta, d_token_t* tx, eth_tx_data_t* td) {
  if (!meta) return IN3_OK;
  sb_printx(meta, "\"input\":{\"to\":\"%B\",\"sender\":\"%B\",\"value\":\"%V\"", td->to, bytes(td->from, 20), td->value);
  sb_printx(meta, ",\"data\":\"%B\",\"gasPrice\":\"0x%x\"", td->data, td->gas_price);
  sb_printx(meta, ",\"nonce\":\"%V\",\"eth_tx_type\":%u", td->nonce, td->type);

  if (td->max_fee_per_gas) sb_printx(meta, ",\"maxFeePerGas\":\"0x%x\"", td->max_fee_per_gas);
  if (td->max_priority_fee_per_gas) sb_printx(meta, ",\"maxPriorityFeePerGas\":\"0x%x\"", td->max_fee_per_gas);

  sb_add_json(meta, ",\"accessList\":", td->access_list);
  sb_add_chars(meta, ",\"layer\":\"l1\"");
  sb_add_json(meta, ",\"fn_sig\":", d_get(tx, key("fn_sig")));
  sb_add_json(meta, ",\"fn_args\":", d_get(tx, key("fn_args")));
  sb_add_json(meta, ",\"token\":", d_get(tx, key("token")));
  sb_add_json(meta, ",\"wallet\":", d_get(tx, key("wallet")));
  sb_add_json(meta, ",\"url\":", d_get(tx, key("url")));
  sb_add_json(meta, ",\"delegate\":", d_get(tx, key("delegate")));
  return IN3_OK;
}

// calls all plugins to modify the raw-tx before signing
static in3_ret_t customize_transaction(d_token_t* tx, in3_req_t* ctx, bytes_t* dst, sb_t* meta, uint8_t* from) {
  if (in3_plugin_is_registered(ctx->client, PLGN_ACT_SIGN_PREPARE)) {
    in3_sign_prepare_ctx_t pctx = {.req = ctx, .old_tx = *dst, .new_tx = NULL_BYTES, .output = meta, .tx = tx};
    memcpy(pctx.account, from, 20);
    in3_ret_t prep_res = in3_plugin_execute_first_or_none(ctx, PLGN_ACT_SIGN_PREPARE, &pctx);

    if (prep_res) {
      _free(pctx.new_tx.data);
      return prep_res;
    }
    else if (pctx.new_tx.data) {
      if (dst->data) _free(dst->data);
      *dst = pctx.new_tx;
    }
  }
  return IN3_OK;
}

static in3_ret_t print_fees(in3_req_t* ctx, bytes_t raw, sb_t* meta) {
  if (!meta || !raw.data) return IN3_OK;
  sb_printx(meta, ",\"unsigned\":[\"%B\"]", raw);
  if (raw.data[0] >= 0xc0) { // this is a legacy-tx
    bytes_t  tmp       = {0};
    uint64_t gas_price = rlp_decode_in_list(&raw, 1, &tmp) == 1 && tmp.len < 9 ? bytes_to_long(tmp.data, tmp.len) : 0;
    uint64_t gas       = rlp_decode_in_list(&raw, 2, &tmp) == 1 && tmp.len < 9 ? bytes_to_long(tmp.data, tmp.len) : 0;
    sb_printx(meta, ",\"fee\":\"0x%x\"", gas_price * gas);
    if (gas < 21000) return req_set_error(ctx, "not enouogh gas!", IN3_EINVAL);
  }

  return IN3_OK;
}

/**
 * prepares a transaction and writes the data to the dst-bytes. In case of success, you MUST free only the data-pointer of the dst.
 */
in3_ret_t eth_prepare_unsigned_tx(d_token_t* tx, in3_req_t* ctx, bytes_t* dst, sb_t* meta) {
  eth_tx_data_t td = {0}; // transaction definition

  // read the values from json
  td.type                     = d_intd(d_get(tx, d_get(tx, K_ETH_TX_TYPE) ? K_ETH_TX_TYPE : K_TYPE), (ctx->client->flags & FLAGS_USE_TX_TYPE2) ? 2 : 0);
  td.access_list              = d_get(tx, K_ACCESS_LIST);
  td.gas_limit                = d_get(tx, K_GAS) ? d_get_long(tx, K_GAS) : (d_get(tx, K_GAS_LIMIT) ? d_get_long(tx, K_GAS_LIMIT) : 0);
  td.to                       = getl(tx, K_TO, 20);
  td.value                    = get(tx, K_VALUE);
  td.data                     = get(tx, K_DATA);
  td.nonce                    = get(tx, K_NONCE);
  td.gas_price                = d_get_long(tx, K_GAS_PRICE);
  td.max_fee_per_gas          = d_get_long(tx, K_MAX_FEE_PER_GAS);
  td.max_priority_fee_per_gas = d_get_long(tx, K_MAX_PRIORITY_FEE_PER_GAS);
  td.gas_prio                 = (uint_fast16_t) d_get_intd(tx, key("gasPrio"), ctx->client->gas_prio);

  TRY(in3_resolve_chain_id(ctx, &td.chain_id))                 // make sure, we have the correct chain_id
  TRY(get_from_address(tx, ctx, td.from))                      // if no from address is specified we take the first signer
  TRY(get_nonce_and_gasprice(&td, ctx))                        // determine the gas price
  TRY(print_input(meta, tx, &td))                              // if this is part of the wallet_exec, we add the input
  TRY(transform_tx(ctx, tx, &td))                              // do we need to transform the tx before we sign it?
  TRY(determine_gas(ctx, &td, d_get_bytes(tx, key("wallet")))) // how much gas are we going to need?

  // create raw without signature
  *dst = serialize_tx_raw(&td, td.type ? 0 : get_v(td.chain_id), NULL_BYTES, NULL_BYTES);

  // write state?
  if (meta) sb_printx(meta, ",\"gas\":\"0x%x\"},\"pre_unsigned\":\"%B\"", td.gas_limit, *dst);

  TRY_CATCH(customize_transaction(tx, ctx, dst, meta, td.from), _free(dst->data)) // in case of a wallet-tx, it will recreate the tx-data usinf execTransaction
  TRY_CATCH(print_fees(ctx, *dst, meta), _free(dst->data))                        // in case of a wallet-tx, it will recreate the tx-data usinf execTransaction

  // cleanup subcontexts
  TRY_CATCH(req_remove_required(ctx, req_find_required(ctx, "eth_getTransactionCount", NULL), false), _free(dst->data))
  TRY_CATCH(req_remove_required(ctx, req_find_required(ctx, "eth_gasPrice", NULL), false), _free(dst->data))

  return IN3_OK;
}

/**
 * signs a unsigned raw transaction and writes the raw data to the dst-bytes. In case of success, you MUST free only the data-pointer of the dst.
 */
in3_ret_t eth_sign_raw_tx(bytes_t raw_tx, in3_req_t* ctx, address_t from, bytes_t* dst, d_token_t* tx_data, sb_t* tx_output) {
  bytes_t    signature;
  chain_id_t chain_id;

  // make sure, we know the chain_id
  TRY(in3_resolve_chain_id(ctx, &chain_id))

  // get the signature from required
  TRY(req_require_signature(ctx, SIGN_EC_HASH, SIGN_CURVE_ECDSA, PL_SIGN_ETHTX, &signature, raw_tx, bytes(from, 20), tx_data ? tx_data : ctx->requests[0], tx_output));
  if (signature.len != 65) return req_set_error(ctx, "Transaction must be signed by a ECDSA-Signature!", IN3_EINVAL);

  // if we reached that point we have a valid signature in sig
  // create raw transaction with signature
  bytes_t  data, last;
  uint8_t  type      = raw_tx.len && raw_tx.data[0] < 10 ? raw_tx.data[0] : 0;
  uint32_t v         = type ? signature.data[64] : (27 + signature.data[64] + (get_v(chain_id) ? (get_v(chain_id) * 2 + 8) : 0));
  int      last_item = 5; // the last item int he rlp encoded tx before the signature starts
  if (type) {             // we skip the first byte, since it defines the type
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
  d_token_t* tx          = d_get_at(d_get(req, K_PARAMS), 0);
  bytes_t    unsigned_tx = NULL_BYTES, signed_tx = NULL_BYTES;
  address_t  from;
  if (d_type(tx) != T_OBJECT) return req_set_error(ctx, "invalid params", IN3_EINVAL);

  TRY(get_from_address(tx, ctx, from));

  // is there a pending signature?
  // we get the raw transaction from this request
  in3_req_t* sig_ctx = req_find_required(ctx, "sign_ec_hash", NULL);
  if (sig_ctx) {
    bytes_t raw = d_get_bytes_at(d_get(sig_ctx->requests[0], K_PARAMS), 0);
    unsigned_tx = bytes(_malloc(raw.len), raw.len);
    memcpy(unsigned_tx.data, raw.data, raw.len);
  }
  else
    TRY(eth_prepare_unsigned_tx(tx, ctx, &unsigned_tx, NULL));
  TRY_FINAL(eth_sign_raw_tx(unsigned_tx, ctx, from, &signed_tx, NULL, NULL), _free(unsigned_tx.data);)

  // build the RPC-request
  char* old_req     = ctx->request_context->c;
  char* raw_request = sprintx("{\"jsonrpc\":\"2.0\",\"method\":\"eth_sendRawTransaction\",\"params\":[\"%B\"]}", signed_tx);

  // now that we included the signature in the rpc-request, we can free it + the old rpc-request.
  _free(signed_tx.data);
  json_free(ctx->request_context);

  // set the new RPC-Request.
  ctx->request_context                               = parse_json(raw_request);
  ctx->requests[0]                                   = ctx->request_context->result;
  in3_cache_add_ptr(&ctx->cache, raw_request)->props = CACHE_PROP_MUST_FREE | CACHE_PROP_ONLY_EXTERNAL;     // we add the request-string to the cache, to make sure the request-string will be cleaned afterwards
  in3_cache_add_ptr(&ctx->cache, old_req)->props     = CACHE_PROP_MUST_FREE | CACHE_PROP_ONLY_NOT_EXTERNAL; // we add the request-string to the cache, to make sure the request-string will be cleaned afterwards, butt only for subrequests
  return IN3_OK;
}

/** minimum signer for the wallet, returns the signed message which needs to be freed **/
char* eth_wallet_sign(const char* key, const char* data) {
  int     data_l = strlen(data) / 2 - 1;
  uint8_t key_bytes[32], *data_bytes = alloca(data_l + 1), dst[65];
  hex_to_bytes(key + 2, -1, key_bytes, 32);
  bytes32_t hash;
  keccak(bytes(data_bytes, hex_to_bytes((char*) data + 2, -1, data_bytes, data_l + 1)), hash);
  char* res = _calloc(133, 1);

  if (crypto_sign_digest(ECDSA_SECP256K1, bytes(hash, 32), key_bytes, NULL, dst) == IN3_OK) {
    bytes_to_hex(dst, 65, res + 2);
    res[0] = '0';
    res[1] = 'x';
  }

  return res;
}
