/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
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

// @PUBLIC_HEADER
/** @file
 * ZKSync API.
 * 
 * This header-file registers zksync api functions.
 * */

#ifndef ZKSYNC_H
#define ZKSYNC_H

#include "../../core/client/plugin.h"
#ifdef ETH_FULL
// #define ZKSYNC_256
#endif
typedef struct {
  uint16_t  id;
  char      symbol[8];
  uint8_t   decimals;
  address_t address;
} zksync_token_t;

typedef struct {
  char*           provider_url;
  uint8_t*        account;
  uint8_t*        main_contract;
  uint8_t*        gov_contract;
  uint64_t        account_id;
  uint16_t        token_len;
  bytes32_t       sync_key;
  zksync_token_t* tokens;

} zksync_config_t;
typedef struct {
  uint32_t        account_id;
  address_t       from;
  address_t       to;
  zksync_token_t* token;
#ifdef ZKSYNC_256
  bytes32_t amount;
  bytes32_t fee;
#else
  uint64_t amount;
  uint64_t fee;
#endif
  uint32_t nonce;
} zksync_tx_data_t;

in3_ret_t in3_register_zksync(in3_t* c);
in3_ret_t zksync_sign_transfer(sb_t* sb, zksync_tx_data_t* data, in3_ctx_t* ctx);
#endif