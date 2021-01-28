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

#include "plugin.h"

#if defined(ETH_FULL) && !defined(ZKSYNC_256)
#define ZKSYNC_256
#endif

#ifdef ZKSYNC_256
typedef bytes32_t zk_fee_t;
typedef uint8_t   zk_fee_p_t;
#else
typedef uint64_t zk_fee_t;
typedef uint64_t zk_fee_p_t;
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

/** a musig session to create a combined signature */
typedef struct zk_musig_session {
  uint64_t                 id;               /**< identifier of a current session */
  bytes32_t                seed;             /**< a random seed used */
  bytes_t                  pub_keys;         /**< the seesion id */
  unsigned int             pos;              /**< the position within the pub_keys */
  unsigned int             len;              /**< number of participants */
  bytes_t                  precommitments;   /**< all precommits */
  bytes_t                  commitments;      /**< all commits */
  bytes_t                  signature_shares; /**< all signatures shares */
  void*                    signer;           /**< handle for the signer */
  struct zk_musig_session* next;             /**< next session */
} zk_musig_session_t;

/** internal configuration-object */
typedef struct {
  char*               provider_url;     /**< url of the zksync-server */
  uint8_t*            account;          /**< address of the account */
  uint8_t*            main_contract;    /**< address of the main zksync contract*/
  uint8_t*            gov_contract;     /**< address of the government contract */
  uint64_t            account_id;       /**< the id of the account as used in the messages */
  uint64_t            nonce;            /**< the current nonce */
  address_t           pub_key_hash_set; /**< the pub_key_hash as set in the account*/
  address_t           pub_key_hash_pk;  /**< the pub_key_hash based on the sync_key*/
  bytes32_t           pub_key;          /**< the pub_key */
  uint16_t            token_len;        /**< number of tokens in the tokenlist */
  bytes32_t           sync_key;         /**< the raw key to sign with*/
  zksync_token_t*     tokens;           /**< the token-list */
  zk_sign_type_t      sign_type;        /**< the signature-type to use*/
  uint32_t            version;          /**< zksync version */
  zk_create2_t*       create2;          /**< create2 args */
  bytes_t             musig_pub_keys;   /**< the public keys of all participants of a schnorr musig signature */
  zk_musig_session_t* musig_sessions;   /**< linked list of open musig sessions */
  char**              musig_urls;       /**< urls to get signatureshares, the order must be in the same order as the pub_keys */
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
  zk_fee_t         amount;     /**< amount to send */
  zk_fee_t         fee;        /**< ransaction fees */
} zksync_tx_data_t;

/** registers the zksync-plugin in the client */
NONULL in3_ret_t in3_register_zksync(in3_t* c);

/** sets a PubKeyHash for the current Account */
NONULL in3_ret_t zksync_set_key(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, d_token_t* params);

/** sends a transfer transaction in Layer 2*/
NONULL in3_ret_t zksync_transfer(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, d_token_t* params, zk_msg_type_t type);

/** sends a deposit transaction in Layer 1*/
NONULL in3_ret_t zksync_deposit(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, d_token_t* params);

/** sends a emergency withdraw  transaction in Layer 1*/
NONULL in3_ret_t zksync_emergency_withdraw(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, d_token_t* params);

/** creates message data and signs a transfer-message */
NONULL in3_ret_t zksync_sign_transfer(sb_t* sb, zksync_tx_data_t* data, in3_ctx_t* ctx, zksync_config_t* conf);

/** creates message data and signs a change_pub_key-message */
NONULL in3_ret_t zksync_sign_change_pub_key(sb_t* sb, in3_ctx_t* ctx, uint8_t* sync_pub_key, uint32_t nonce, zksync_config_t* conf, zk_fee_t fee, zksync_token_t* token);

#endif

in3_ret_t           zksync_musig_sign(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, d_token_t* params);
zk_musig_session_t* zk_musig_session_free(zk_musig_session_t* s);
in3_ret_t           zksync_sign(zksync_config_t* conf, bytes_t msg, in3_ctx_t* ctx, uint8_t* sig);