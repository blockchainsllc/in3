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

// @PUBLIC_HEADER
/** @file
 * Ethereum Nano verification.
 * */

#ifndef iamo_zk_h__
#define iamo_zk_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "../../core/client/client.h"
#include "../../core/client/plugin.h"

typedef struct iamo_zk_config {
  char*     cosign_rpc;
  address_t creator;
  address_t master_copy;
} iamo_zk_config_t;

/**
 * registers pk signer as plugin so you can use config or in3_addKeys as rpc
 */
in3_ret_t register_iamo_zk(in3_t* in3);

in3_ret_t iamo_zk_add_wallet(iamo_zk_config_t* conf, in3_rpc_handle_ctx_t* ctx);
in3_ret_t iamo_zk_create_wallet(iamo_zk_config_t* conf, in3_rpc_handle_ctx_t* ctx);
in3_ret_t iamo_zk_is_valid(iamo_zk_config_t* conf, in3_rpc_handle_ctx_t* ctx);
in3_ret_t iamo_zk_get_config(iamo_zk_config_t* conf, in3_rpc_handle_ctx_t* ctx);

#ifdef __cplusplus
}
#endif

#endif
