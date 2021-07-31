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

#include "eth_basic.h"
#include "../../../core/client/keys.h"
#include "../../../core/client/request_internal.h"
#include "../../../core/util/data.h"
#include "../../../core/util/debug.h"
#include "../../../core/util/mem.h"
#include "../../../core/util/utils.h"
#include "../../../verifier/eth1/basic/filter.h"
#include "../../../verifier/eth1/nano/eth_nano.h"
#include "../../../verifier/eth1/nano/merkle.h"
#include "../../../verifier/eth1/nano/rlp.h"
#include "../../../verifier/eth1/nano/serialize.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

in3_ret_t in3_verify_eth_basic(in3_vctx_t* vc) {
  if (vc->chain->type != CHAIN_ETH) return IN3_EIGNORE;

  // make sure we want to verify
  if (in3_req_get_proof(vc->req, vc->index) == PROOF_NONE) return IN3_OK;

  // do we have a result? if not it is a valid error-response
  if (!vc->result) return IN3_OK;

  if (d_type(vc->result) == T_NULL) {
    // check if there's a proof for non-existence
#if !defined(RPC_ONLY) || defined(RPC_ETH_GETTRANSACTIONBYBLOCKHASHANDINDEX) || defined(RPC_ETH_GETTRANSACTIONBYBLOCKNUMBERANDINDEX)
    if (VERIFY_RPC("eth_getTransactionByBlockHashAndIndex") || VERIFY_RPC("eth_getTransactionByBlockNumberAndIndex"))
      return eth_verify_eth_getTransactionByBlock(vc, d_get_at(d_get(vc->request, K_PARAMS), 0), d_get_int_at(d_get(vc->request, K_PARAMS), 1));
#endif
    return IN3_OK;
  }

#if !defined(RPC_ONLY) || defined(RPC_ETH_GETTRANSACTIONBYHASH)
  if (VERIFY_RPC("eth_getTransactionByHash")) return eth_verify_eth_getTransaction(vc, d_get_bytes_at(d_get(vc->request, K_PARAMS), 0));
#endif
#if !defined(RPC_ONLY) || defined(RPC_ETH_GETTRANSACTIONBYBLOCKHASHANDINDEX) || defined(RPC_ETH_GETTRANSACTIONBYBLOCKNUMBERANDINDEX)
  if (VERIFY_RPC("eth_getTransactionByBlockHashAndIndex") || VERIFY_RPC("eth_getTransactionByBlockNumberAndIndex"))
    return eth_verify_eth_getTransactionByBlock(vc, d_get_at(d_get(vc->request, K_PARAMS), 0), d_get_int_at(d_get(vc->request, K_PARAMS), 1));
#endif
#if !defined(RPC_ONLY) || defined(RPC_ETH_GETBLOCKBYNUMBER)
  if (VERIFY_RPC("eth_getBlockByNumber"))
    return eth_verify_eth_getBlock(vc, NULL, d_get_long_at(d_get(vc->request, K_PARAMS), 0));
#endif
#if !defined(RPC_ONLY) || defined(RPC_ETH_GETTRANSACTIONCOUNTBYHASH)
  if (VERIFY_RPC("eth_getBlockTransactionCountByHash"))
    return eth_verify_eth_getBlockTransactionCount(vc, d_get_bytes_at(d_get(vc->request, K_PARAMS), 0), 0);
#endif
#if !defined(RPC_ONLY) || defined(RPC_ETH_GETTRANSACTIONCOUNTBYNUMBER)
  if (VERIFY_RPC("eth_getBlockTransactionCountByNumber"))
    return eth_verify_eth_getBlockTransactionCount(vc, NULL, d_get_long_at(d_get(vc->request, K_PARAMS), 0));
#endif
#if !defined(RPC_ONLY) || defined(RPC_ETH_GETBLOCKBYHASH)
  if (VERIFY_RPC("eth_getBlockByHash"))
    return eth_verify_eth_getBlock(vc, d_get_bytes_at(d_get(vc->request, K_PARAMS), 0), 0);
#endif
#if !defined(RPC_ONLY) || defined(RPC_ETH_GETBALANCE) || defined(RPC_ETH_GETCODE) || defined(RPC_ETH_GETSTORAGEAT) || defined(RPC_ETH_GETTRANSACTIONCOUNT)
  if (VERIFY_RPC("eth_getBalance") || VERIFY_RPC("eth_getCode") || VERIFY_RPC("eth_getStorageAt") || VERIFY_RPC("eth_getTransactionCount"))
    return eth_verify_account_proof(vc);
#endif
  if (VERIFY_RPC("eth_gasPrice") || VERIFY_RPC("eth_newFilter") || VERIFY_RPC("eth_newBlockFilter") || VERIFY_RPC("eth_newPendingFilter") || VERIFY_RPC("eth_uninstallFilter") || VERIFY_RPC("eth_getFilterChanges"))
    return IN3_OK;
#if !defined(RPC_ONLY) || defined(RPC_ETH_GETLOGS)
  if (VERIFY_RPC("eth_getLogs")) // for txReceipt, we need the txhash
    return eth_verify_eth_getLog(vc, d_len(vc->result));
#endif
#if !defined(RPC_ONLY) || defined(RPC_ETH_SENDRAWTRANSACTION)
  if (VERIFY_RPC("eth_sendRawTransaction")) {
    bytes32_t hash;
    keccak(d_to_bytes(d_get_at(d_get(vc->request, K_PARAMS), 0)), hash);
    return bytes_cmp(*d_bytes(vc->result), bytes(hash, 32)) ? IN3_OK : vc_err(vc, "the transactionHash of the response does not match the raw transaction!");
  }
#endif
  return IN3_EIGNORE;
}

static in3_ret_t eth_send_transaction_and_wait(in3_rpc_handle_ctx_t* ctx) {
  d_token_t * tx_hash, *tx_receipt;
  str_range_t r       = d_to_json(ctx->params + 1);
  char*       tx_data = alloca(r.len + 1);
  memcpy(tx_data, r.data, r.len);
  tx_data[r.len]      = 0;
  in3_req_t* send_req = NULL;
  in3_req_t* last_r   = NULL;
  TRY(req_send_sub_request(ctx->req, "eth_sendTransaction", tx_data, NULL, &tx_hash, &send_req))
  // tx was sent, we have a tx_hash
  char tx_hash_hex[69];
  bytes_to_hex(d_bytes(tx_hash)->data, 32, tx_hash_hex + 3);
  tx_hash_hex[0] = tx_hash_hex[67] = '"';
  tx_hash_hex[1]                   = '0';
  tx_hash_hex[2]                   = 'x';
  tx_hash_hex[68]                  = 0;

  // get the tx_receipt
  TRY(req_send_sub_request(ctx->req, "eth_getTransactionReceipt", tx_hash_hex, NULL, &tx_receipt, &last_r))

  if (d_type(tx_receipt) == T_NULL || d_get_long(tx_receipt, K_BLOCK_NUMBER) == 0) {
    // no tx yet
    // we remove it and try again
    uint32_t wait = d_get_int(d_get(last_r->requests[0], K_IN3), K_WAIT);
    wait          = wait ? wait * 2 : 1000;
    req_remove_required(ctx->req, last_r, false);
    if (wait > 120000) // more than 2 minutes is too long, so we stop here
      return req_set_error(ctx->req, "Waited too long for the transaction to be minded", IN3_ELIMIT);
    char in3[20];
    sprintf(in3, "{\"wait\":%d}", wait);

    return req_send_sub_request(ctx->req, "eth_getTransactionReceipt", tx_hash_hex, in3, &tx_receipt, &last_r);
  }
  else {
    // we have a result and we keep it
    str_range_t r = d_to_json(tx_receipt);
    sb_add_range(in3_rpc_handle_start(ctx), r.data, 0, r.len);
    req_remove_required(ctx->req, last_r, false);
    req_remove_required(ctx->req, send_req, false);
    return in3_rpc_handle_finish(ctx);
  }
}

static in3_ret_t eth_newFilter(in3_filter_handler_t* filters, in3_rpc_handle_ctx_t* ctx) {
  if (!ctx->params || d_type(ctx->params) != T_ARRAY || !d_len(ctx->params) || d_type(ctx->params + 1) != T_OBJECT)
    return req_set_error(ctx->req, "invalid type of params, expected object", IN3_EINVAL);
  else if (!filter_opt_valid(ctx->params + 1))
    return req_set_error(ctx->req, "filter option parsing failed", IN3_EINVAL);
  if (!ctx->params->data) return req_set_error(ctx->req, "binary request are not supported!", IN3_ENOTSUP);

  char*     fopt = d_create_json(ctx->req->request_context, ctx->params + 1);
  in3_ret_t res  = filter_add(filters, ctx->req, FILTER_EVENT, fopt);
  if (res < 0) {
    _free(fopt);
    return req_set_error(ctx->req, "filter creation failed", res);
  }

  return in3_rpc_handle_with_int(ctx, (uint64_t) res);
}

static in3_ret_t eth_newBlockFilter(in3_filter_handler_t* filters, in3_rpc_handle_ctx_t* ctx) {
  in3_ret_t res = filter_add(filters, ctx->req, FILTER_BLOCK, NULL);
  if (res < 0) return req_set_error(ctx->req, "filter creation failed", res);
  return in3_rpc_handle_with_int(ctx, (uint64_t) res);
}

static in3_ret_t eth_getFilterChanges(in3_filter_handler_t* filters, in3_rpc_handle_ctx_t* ctx) {
  if (!ctx->params || d_len(ctx->params) == 0 || d_type(ctx->params + 1) != T_INTEGER)
    return req_set_error(ctx->req, "invalid type of params, expected filter-id as integer", IN3_EINVAL);

  uint64_t  id  = d_get_long_at(ctx->params, 0);
  sb_t      sb  = {0};
  in3_ret_t ret = filter_get_changes(filters, ctx->req, id, &sb);
  if (ret != IN3_OK) {
    if (sb.data) _free(sb.data);
    return req_set_error(ctx->req, "failed to get filter changes", ret);
  }
  in3_rpc_handle_with_string(ctx, sb.data);
  _free(sb.data);
  return IN3_OK;
}

/** called to see if we can handle the request internally */
static in3_ret_t eth_handle_intern(in3_filter_handler_t* filters, in3_rpc_handle_ctx_t* ctx) {
  UNUSED_VAR(filters);
  // we only support ETH in this module
  if (ctx->req->client->chain.type != CHAIN_ETH) return IN3_EIGNORE;

    // check method to handle internally
#if !defined(RPC_ONLY) || defined(RPC_ETH_SENDTRANSACTION)
  TRY_RPC("eth_sendTransaction", handle_eth_sendTransaction(ctx->req, ctx->request))
#endif
#if !defined(RPC_ONLY) || defined(RPC_ETH_SENDTRANSACTIONANDWAIT)
  TRY_RPC("eth_sendTransactionAndWait", eth_send_transaction_and_wait(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_ETH_NEWFILTER)
  TRY_RPC("eth_newFilter", eth_newFilter(filters, ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_ETH_NEWBLOCKFILTER)
  TRY_RPC("eth_newBlockFilter", eth_newBlockFilter(filters, ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_ETH_NEWPENDINGTRANSACTIONFILTER)
  TRY_RPC("eth_newPendingTransactionFilter", req_set_error(ctx->req, "pending filter not supported", IN3_ENOTSUP))
#endif
#if !defined(RPC_ONLY) || defined(RPC_ETH_GETFILTERCHANGES)
  TRY_RPC("eth_getFilterChanges", eth_getFilterChanges(filters, ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_ETH_GETFILTERLOGS)
  TRY_RPC("eth_getFilterLogs", eth_getFilterChanges(filters, ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_ETH_UNINSTALLFILTER)
  TRY_RPC("eth_uninstallFilter", (!ctx->params || d_len(ctx->params) == 0 || d_type(ctx->params + 1) != T_INTEGER)
                                     ? req_set_error(ctx->req, "invalid type of params, expected filter-id as integer", IN3_EINVAL)
                                     : in3_rpc_handle_with_string(ctx, filter_remove(filters, d_get_long_at(ctx->params, 0)) ? "true" : "false"))
#endif

  if (strcmp(ctx->method, "eth_chainId") == 0 && ctx->req->client->chain.chain_id != CHAIN_ID_LOCAL)
    return in3_rpc_handle_with_int(ctx, ctx->req->client->chain.chain_id);

  return IN3_EIGNORE;
}

static in3_ret_t free_filters(in3_filter_handler_t* f) {
  for (size_t i = 0; i < f->count; i++) {
    if (f->array[i]) f->array[i]->release(f->array[i]);
  }
  if (f->array) _free(f->array);
  _free(f);
  return IN3_OK;
}

in3_ret_t handle_basic(void* pdata, in3_plugin_act_t action, void* pctx) {
  UNUSED_VAR(pdata);
  switch (action) {
    case PLGN_ACT_RPC_VERIFY: return in3_verify_eth_basic(pctx);
    case PLGN_ACT_RPC_HANDLE: return eth_handle_intern(pdata, pctx);
    case PLGN_ACT_TERM: return free_filters(pdata);
    default: return IN3_EINVAL;
  }
}

in3_filter_handler_t* eth_basic_get_filters(in3_t* c) {
  for (in3_plugin_t* p = c->plugins; p; p = p->next) {
    if (p->action_fn == handle_basic) return p->data;
  }
  return NULL;
}

in3_ret_t in3_register_eth_basic(in3_t* c) {
  in3_filter_handler_t* handler = _calloc(1, sizeof(in3_filter_handler_t));
  in3_register_eth_nano(c);
  return in3_plugin_register(c, PLGN_ACT_TERM | PLGN_ACT_RPC_VERIFY | PLGN_ACT_RPC_HANDLE, handle_basic, handler, false);
}
