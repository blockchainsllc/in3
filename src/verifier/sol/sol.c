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

#include "sol.h"
#include "../../core/client/keys.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/crypto.h"
#include "../../core/util/data.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "sol_sign.h"
#include <string.h>

in3_ret_t sol_send_req(in3_rpc_handle_ctx_t* ctx, char* method, char* params, d_token_t** result) {
  chain_id_t chain_id = in3_chain_id(ctx->req);
  char*      in3      = NULL;
  if (chain_id != CHAIN_ID_LOCAL)
    in3 = "{\"rpc\":\"https://api.testnet.solana.com\"}";
  return req_send_sub_request(ctx->req, method, params, in3, result, NULL);
}

in3_ret_t sol_handle_req(in3_rpc_handle_ctx_t* ctx, char* method, d_token_t* params) {
  d_token_t* result;
  char*      p  = params ? sprintx("%j", params) : _strdupn("[]", -1);
  int        pl = strlen(p);
  p[pl - 1]     = 0;
  TRY_FINAL(sol_send_req(ctx, method, p + 1, &result), _free(p))
  sb_add_json(in3_rpc_handle_start(ctx), "", result);
  return in3_rpc_handle_finish(ctx);
}

static in3_ret_t in3_handle_sol_rpc(in3_rpc_handle_ctx_t* ctx) {
  if (strncmp(ctx->method, "sol_", 4)) return IN3_EIGNORE;

  TRY_RPC("sol_send_tx", sol_send_tx(ctx))

  // fallback is we forward the request
  return sol_handle_req(ctx, ctx->method + 4, ctx->params);
}

static in3_ret_t in3_handle_sol(void* pdata, in3_plugin_act_t action, void* pctx) {
  UNUSED_VAR(pdata);
  switch (action) {
    case PLGN_ACT_RPC_HANDLE: return in3_handle_sol_rpc(pctx);
    default: return IN3_ENOTSUP;
  }
}

in3_ret_t in3_register_sol(in3_t* c) {
  return in3_plugin_register(c, PLGN_ACT_RPC_HANDLE, in3_handle_sol, NULL, false);
}
