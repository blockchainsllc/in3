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
#if defined(ETH_FULL) && !defined(ZKSYNC_256)
#define ZKSYNC_256
#endif

/** represents a token supported in zksync. */
typedef struct {
  uint16_t  id;        /**< the id used in messaged */
  char      symbol[8]; /**< short symbol */
  uint8_t   decimals;  /**< decimals for display */
  address_t address;   /**< erc20 address (or 0x00 for eth) */
} zksync_token_t;

/** message type. */
typedef enum zk_msg_type {
  ZK_TRANSFER = 5, /**< transfer tx */
  ZK_WITHDRAW = 3  /**< withdraw tx */
} zk_msg_type_t;

/** signature-type which can be configured in the config */
typedef enum zk_sign_type {
  ZK_SIGN_PK       = 1, /**< sign with PK (default)*/
  ZK_SIGN_CONTRACT = 2, /**< use eip1271 contract signatures */
  ZK_SIGN_CREATE2  = 3  /**< use creat2 code */
} zk_sign_type_t;

/** create2-arguments */
typedef struct zk_create2 {
  address_t creator;  /**< address of the creator*/
  bytes32_t salt_arg; /**< saltarg*/
  bytes32_t codehash; /**< hash of the deploy-txdata*/
} zk_create2_t;

/** internal configuration-object */
typedef struct {
  char*           provider_url;  /**< url of the zksync-server */
  uint8_t*        account;       /**< address of the account */
  uint8_t*        main_contract; /**< address of the main zksync contract*/
  uint8_t*        gov_contract;  /**< address of the government contract */
  uint64_t        account_id;    /**< the id of the account as used in the messages */
  uint64_t        nonce;         /**< the current nonce */
  address_t       pub_key_hash;  /**< the pub_key_hash */
  uint16_t        token_len;     /**< number of tokens in the tokenlist */
  bytes32_t       sync_key;      /**< the raw key to sign with*/
  zksync_token_t* tokens;        /**< the token-list */
  zk_sign_type_t  sign_type;     /**< the signature-type to use*/
  uint32_t        version;       /**< zksync version */
  zk_create2_t*   create2;       /**< create2 args */
  bytes_t         musig_pub_keys; /**< the public keys of all participants of a schnorr musig signature */
} zksync_config_t;

/** a transaction */
typedef struct {
  zksync_config_t* conf;       /**< the configuration of the zksync-account */
  uint32_t         account_id; /**< the id of the account */
  address_t        from;       /**< the from-address */
  address_t        to;         /**< the address of the receipient */
  zksync_token_t*  token;      /**< the token to use */
  uint32_t         nonce;      /**< current nonce */
  zk_msg_type_t    type;       /**< message type */
#ifdef ZKSYNC_256
  bytes32_t amount; /**< amount to send */
  bytes32_t fee;    /**< ransaction fees */
#else
  uint64_t amount; /**< amount to send */
  uint64_t fee;    /**< ransaction fees */
#endif
} zksync_tx_data_t;

/** registers the zksync-plugin in the client */
in3_ret_t in3_register_zksync(in3_t* c);

/** sets a PubKeyHash for the current Account */
in3_ret_t zksync_set_key(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, d_token_t* params);

/** sends a transfer transaction in Layer 2*/
in3_ret_t zksync_transfer(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, d_token_t* params, zk_msg_type_t type);

/** sends a deposit transaction in Layer 1*/
in3_ret_t zksync_deposit(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, d_token_t* params);

/** sends a emergency withdraw  transaction in Layer 1*/
in3_ret_t zksync_emergency_withdraw(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, d_token_t* params);

/** creates message data and signs a transfer-message */
in3_ret_t zksync_sign_transfer(sb_t* sb, zksync_tx_data_t* data, in3_ctx_t* ctx, uint8_t* sync_key);

/** creates message data and signs a change_pub_key-message */
in3_ret_t zksync_sign_change_pub_key(sb_t* sb, in3_ctx_t* ctx, uint8_t* sync_pub_key, uint8_t* sync_key, uint32_t nonce, zksync_config_t* conf,
#ifdef ZKSYNC_256
                                     bytes32_t fee
#else
                                     uint64_t fee
#endif
                                     ,
                                     zksync_token_t* token);

#endif

in3_ret_t zksync_musig_create_pre_commit(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, d_token_t* params);