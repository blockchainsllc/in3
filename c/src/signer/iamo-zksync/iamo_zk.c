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

#include "iamo_zk.h"
#include "../../core/client/context_internal.h"
#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/secp256k1.h"
#include "../../verifier/eth1/nano/serialize.h"
#include <string.h>
#include <time.h>

static in3_ret_t iamo_free(iamo_zk_config_t* conf) {
  _free(conf->cosign_rpc);
  _free(conf);
  return IN3_OK;
}

static in3_ret_t iamo_config_set(iamo_zk_config_t* conf, in3_configure_ctx_t* ctx) {
  if (ctx->token->key == key("iamo_zk")) {
    bytes_t* tmp;
    char*    cosign_rpc = d_get_stringk(ctx->token, key("cosign_rpc"));
    if (cosign_rpc) conf->cosign_rpc = _strdupn(cosign_rpc, -1);
    if ((tmp = d_get_bytesk(ctx->token, key("master_copy"))) && tmp && tmp->len == 20)
      memcpy(conf->master_copy, tmp->data, 20);
    if ((tmp = d_get_bytesk(ctx->token, key("creator"))) && tmp && tmp->len == 20)
      memcpy(conf->creator, tmp->data, 20);
    return IN3_OK;
  }
  return IN3_EIGNORE;
}

static in3_ret_t iamo_rpc(iamo_zk_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  TRY_RPC("iamo_zk_add_wallet", iamo_zk_add_wallet(conf, ctx))
  TRY_RPC("iamo_zk_is_valid", iamo_zk_is_valid(conf, ctx))
  TRY_RPC("iamo_zk_create_wallet", iamo_zk_create_wallet(conf, ctx))
  TRY_RPC("iamo_zk_get_config", iamo_zk_get_config(conf, ctx))
  return IN3_EIGNORE;
}

static in3_ret_t iamo_handle(void* data, in3_plugin_act_t action, void* action_ctx) {
  switch (action) {
    case PLGN_ACT_TERM: return iamo_free(data);
    case PLGN_ACT_CONFIG_SET: return iamo_config_set(data, action_ctx);
    case PLGN_ACT_RPC_HANDLE: return iamo_rpc(data, action_ctx);
    default: return IN3_ENOTSUP;
  }
  return IN3_ENOTSUP;
}

in3_ret_t register_iamo_zk(in3_t* in3) {
  return in3_plugin_register(in3, PLGN_ACT_TERM | PLGN_ACT_CONFIG_SET | PLGN_ACT_RPC_HANDLE, iamo_handle, _calloc(1, sizeof(iamo_zk_config_t)), true);
}
