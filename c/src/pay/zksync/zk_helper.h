/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
 * 
 * Copyright (C) 2018-2019 slock.it GmbH, Blockchains LLC
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

#ifndef ZKSYNC_HELPER_H
#define ZKSYNC_HELPER_H

#include "zksync.h"

in3_ret_t  zksync_get_fee(zksync_config_t* conf, in3_ctx_t* ctx, d_token_t* fee_in, bytes_t to, d_token_t* token, char* type, zk_fee_p_t* fee); /**< resolves the fees */
void       set_quoted_address(char* c, uint8_t* address);                                                                                       /**< writes the address as hex into the string.*/
d_token_t* params_get(d_token_t* params, d_key_t k, uint32_t index);                                                                            /**< returns the token either by index or key depending on the token-type */
in3_ret_t  send_provider_request(in3_ctx_t* parent, zksync_config_t* conf, char* method, char* params, d_token_t** result);                     /**< send a request to the zksync-server*/
in3_ret_t  zksync_get_account(zksync_config_t* conf, in3_ctx_t* ctx, uint8_t** account);                                                        /**< resolves the account */
in3_ret_t  zksync_update_account(zksync_config_t* conf, in3_ctx_t* ctx);                                                                        /**< updates the account data from the server to the config*/
in3_ret_t  zksync_get_account_id(zksync_config_t* conf, in3_ctx_t* ctx, uint32_t* account_id);                                                  /**< resolves the account_id*/
in3_ret_t  zksync_get_sync_key(zksync_config_t* conf, in3_ctx_t* ctx, uint8_t* sync_key);                                                       /**< resolves the sync key*/
in3_ret_t  zksync_get_contracts(zksync_config_t* conf, in3_ctx_t* ctx, uint8_t** main);                                                         /**< resolves the main contract */
in3_ret_t  zksync_get_nonce(zksync_config_t* conf, in3_ctx_t* ctx, d_token_t* nonce_in, uint32_t* nonce);                                       /**< resolves the nonce */
in3_ret_t  resolve_tokens(zksync_config_t* conf, in3_ctx_t* ctx, d_token_t* token_src, zksync_token_t** token_dst);                             /**< resolve token list */
in3_ret_t  zksync_get_pubkey_hash(zksync_config_t* conf, in3_ctx_t* ctx, uint8_t* pubkey_hash);                                                 /**< get pubkeyhash */
void       zksync_calculate_account(address_t creator, bytes32_t codehash, bytes32_t saltarg, address_t pub_key_hash, address_t dst);
#define CHECK_PARAM_TOKEN(ctx, params, index)                                                                                                    \
  switch (d_type(d_get_at(params, index))) {                                                                                                     \
    case T_STRING: break;                                                                                                                        \
    case T_BYTES:                                                                                                                                \
      if (d_len(d_get_at(params, index)) != 20) return ctx_set_error(ctx, "argument at index " #index " must be a 20 byte address", IN3_EINVAL); \
      break;                                                                                                                                     \
    default: return ctx_set_error(ctx, "argument at index " #index " must be a token name or an address", IN3_EINVAL);                           \
  }

#endif