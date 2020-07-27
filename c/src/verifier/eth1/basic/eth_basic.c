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
  char* method = d_get_stringk(vc->request, K_METHOD);

  // make sure we want to verify
  if (in3_ctx_get_proof(vc->ctx, vc->index) == PROOF_NONE) return IN3_OK;

  // do we have a result? if not it is a valid error-response
  if (!vc->result) {
    return IN3_OK;
  }
  else if (d_type(vc->result) == T_NULL) {
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
  }
  else if (strcmp(method, "eth_getBlockByNumber") == 0)
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
    keccak(d_to_bytes(d_get_at(d_get(vc->request, K_PARAMS), 0)), hash);
    return bytes_cmp(*d_bytes(vc->result), bytes(hash, 32)) ? IN3_OK : vc_err(vc, "the transactionHash of the response does not match the raw transaction!");
  }
  else
    return IN3_EIGNORE;
}

/** called to see if we can handle the request internally */
static in3_ret_t eth_handle_intern(in3_rpc_handle_ctx_t* rctx) {

  in3_ctx_t* ctx    = rctx->ctx;
  char*      method = d_get_stringk(rctx->request, K_METHOD);
  d_token_t* params = d_get(rctx->request, K_PARAMS);

  // we only support ETH in this module
  if (in3_get_chain(ctx->client)->type != CHAIN_ETH) return IN3_EIGNORE;

  // check method to handle internally
  if (strcmp(method, "eth_sendTransaction") == 0)
    return handle_eth_sendTransaction(ctx, rctx->request);

  else if (strcmp(method, "eth_newFilter") == 0) {
    if (!params || d_type(params) != T_ARRAY || !d_len(params) || d_type(params + 1) != T_OBJECT)
      return ctx_set_error(ctx, "invalid type of params, expected object", IN3_EINVAL);
    else if (!filter_opt_valid(params + 1))
      return ctx_set_error(ctx, "filter option parsing failed", IN3_EINVAL);
    if (!params->data) return ctx_set_error(ctx, "binary request are not supported!", IN3_ENOTSUP);

    char*     fopt = d_create_json(params + 1);
    in3_ret_t res  = filter_add(ctx, FILTER_EVENT, fopt);
    if (res < 0) {
      _free(fopt);
      return ctx_set_error(ctx, "filter creation failed", res);
    }

    return in3_rpc_handle_with_int(rctx, (uint64_t) res);
  }
  else if (strcmp(method, "eth_chainId") == 0)
    return in3_rpc_handle_with_int(rctx, ctx->client->chain_id);
  else if (strcmp(method, "eth_newBlockFilter") == 0) {
    in3_ret_t res = filter_add(ctx, FILTER_BLOCK, NULL);
    if (res < 0) return ctx_set_error(ctx, "filter creation failed", res);
    return in3_rpc_handle_with_int(rctx, (uint64_t) res);
  }
  else if (strcmp(method, "eth_newPendingTransactionFilter") == 0)
    return ctx_set_error(ctx, "pending filter not supported", IN3_ENOTSUP);

  else if (strcmp(method, "eth_uninstallFilter") == 0)
    return (!params || d_len(params) == 0 || d_type(params + 1) != T_INTEGER)
               ? ctx_set_error(ctx, "invalid type of params, expected filter-id as integer", IN3_EINVAL)
               : in3_rpc_handle_with_string(rctx, filter_remove(ctx->client, d_get_long_at(params, 0)) ? "true" : "false");

  else if (strcmp(method, "eth_getFilterChanges") == 0 || strcmp(method, "eth_getFilterLogs") == 0) {
    if (!params || d_len(params) == 0 || d_type(params + 1) != T_INTEGER)
      return ctx_set_error(ctx, "invalid type of params, expected filter-id as integer", IN3_EINVAL);

    uint64_t  id  = d_get_long_at(params, 0);
    sb_t      sb  = {0};
    in3_ret_t ret = filter_get_changes(ctx, id, &sb);
    if (ret != IN3_OK) {
      if (sb.data) _free(sb.data);
      return ctx_set_error(ctx, "failed to get filter changes", ret);
    }
    in3_rpc_handle_with_string(rctx, sb.data);
    _free(sb.data);
    return IN3_OK;
  }
  return IN3_EIGNORE;
}

in3_ret_t handle_basic(void* pdata, in3_plugin_act_t action, void* pctx) {
  UNUSED_VAR(pdata);
  switch (action) {
    case PLGN_ACT_RPC_VERIFY: return in3_verify_eth_basic(pctx);
    case PLGN_ACT_RPC_HANDLE: return eth_handle_intern(pctx);
    default: return IN3_EINVAL;
  }
}

in3_ret_t in3_register_eth_basic(in3_t* c) {
  in3_register_eth_nano(c);
  return in3_plugin_register(c, PLGN_ACT_RPC_VERIFY | PLGN_ACT_RPC_HANDLE, handle_basic, NULL, false);
}
