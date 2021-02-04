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

// @PUBLIC_HEADER
/** @file
 * BTC API.
 *
 * This header-file defines easy to use function, which are preparing the JSON-RPC-Request, which is then executed and verified by the incubed-client.
 * */

#ifndef IN3_BTC_API_H
#define IN3_BTC_API_H
#ifdef __cplusplus
extern "C" {
#endif

#include "../../core/client/client.h"
#include "../../core/util/bytes.h"
#include "../utils/api_utils.h"

/**< The current error or null if all is ok */
#define btc_last_error() api_last_error()

/** the tx in */
typedef struct btc_transaction_in {
  uint32_t  vout;        /**< the tx index of the output */
  bytes32_t txid;        /**< the tx id of the output */
  uint32_t  sequence;    /**< the sequence */
  bytes_t   script;      /**< the script */
  bytes_t   txinwitness; /**< witnessdata (if used) */
} btc_transaction_in_t;

/** the tx out */
typedef struct btc_transaction_out {
  uint64_t value;         /**< the value of the tx */
  uint32_t n;             /**< the index */
  bytes_t  script_pubkey; /**< the script pubkey (or signature)*/
} btc_transaction_out_t;

/** a transaction */
typedef struct btc_transaction {
  bool                   in_active_chain; /**< true if it is part of the active chain */
  bytes_t                data;            /**< the serialized transaction-data */
  bytes32_t              txid;            /**< the transaction id */
  bytes32_t              hash;            /**< the transaction hash */
  uint32_t               size;            /**< raw size of the transaction */
  uint32_t               vsize;           /**< virtual size of the transaction */
  uint32_t               weight;          /**< weight of the tx */
  uint32_t               version;         /**< used version  */
  uint32_t               locktime;        /**< locktime */
  btc_transaction_in_t*  vin;             /**< array of transaction inputs */
  btc_transaction_out_t* vout;            /**< array of transaction outputs */
  uint32_t               vin_len;         /**< number of tx inputs */
  uint32_t               vout_len;        /**< number of tx outputs */
  bytes32_t              blockhash;       /**< hash of block containing the tx */
  uint32_t               confirmations;   /**< number of confirmations or blocks mined on top of the containing block*/
  uint32_t               time;            /**< unix timestamp in seconds since 1970 */
  uint32_t               blocktime;       /**< unix timestamp in seconds since 1970 */
} btc_transaction_t;

/** the blockheader */
typedef struct btc_blockheader {
  bytes32_t hash;          /**< the hash of the blockheader */
  uint32_t  confirmations; /**< number of confirmations or blocks mined on top of the containing block*/
  uint32_t  height;        /**< block number */
  uint32_t  version;       /**< used version  */
  bytes32_t merkleroot;    /**< merkle root of the trie of all transactions in the block  */
  uint32_t  time;          /**< unix timestamp in seconds since 1970 */
  uint32_t  nonce;         /**< nonce-field of the block */
  uint8_t   bits[4];       /**<bits (target) for the block */
  bytes32_t chainwork;     /**<total amount of work since genesis */
  uint32_t  n_tx;          /**< number of transactions in the block */
  bytes32_t previous_hash; /**< hash of the parent blockheader */
  bytes32_t next_hash;     /**< hash of the next blockheader */
  uint8_t   data[80];      /**< raw serialized header-bytes */
} btc_blockheader_t;

/** a block with all transactions including their full data */
typedef struct btc_block_txdata {
  btc_blockheader_t  header; /**< the blockheader */
  uint32_t           tx_len; /**< number of transactions */
  btc_transaction_t* tx;     /**< array of transactiondata */
} btc_block_txdata_t;

/** a block with all transaction ids */
typedef struct btc_block_txids {
  btc_blockheader_t header; /**< the blockheader */
  uint32_t          tx_len; /**< number of transactions */
  bytes32_t*        tx;     /**< array of transaction ids */
} btc_block_txids_t;

/** 
 * gets the transaction as raw bytes or null if it does not exist.
 * You must free the result with b_free() after use!
 */
bytes_t* btc_get_transaction_bytes(in3_t*    in3, /**< the in3-instance*/
                                   bytes32_t txid /**< the txid */
);

/** 
 * gets the transaction as struct or null if it does not exist.
 * You must free the result with free() after use!
 */
btc_transaction_t* btc_get_transaction(in3_t*    in3, /**< the in3-instance*/
                                       bytes32_t txid /**< the txid */
);

/** 
 * gets the blockheader as struct or null if it does not exist.
 * You must free the result with free() after use!
 */
btc_blockheader_t* btc_get_blockheader(in3_t*    in3,      /**< the in3-instance*/
                                       bytes32_t blockhash /**< the block hash */
);

/** 
 * gets the blockheader as raw serialized data (80 bytes) or null if it does not exist.
 * You must free the result with b_free() after use!
 */
bytes_t* btc_get_blockheader_bytes(in3_t*    in3,      /**< the in3-instance*/
                                   bytes32_t blockhash /**< the block hash */
);

/** 
 * gets the block as struct including all transaction data or null if it does not exist.
 * You must free the result with free() after use!
 */
btc_block_txdata_t* btc_get_block_txdata(in3_t*    in3,      /**< the in3-instance*/
                                         bytes32_t blockhash /**< the block hash */
);

/** 
 * gets the block as struct including all transaction ids or null if it does not exist.
 * You must free the result with free() after use!
 */
btc_block_txids_t* btc_get_block_txids(in3_t*    in3,      /**< the in3-instance*/
                                       bytes32_t blockhash /**< the block hash */
);

/** 
 * gets the block as raw serialized block bytes including all transactions or null if it does not exist.
 * You must free the result with b_free() after use!
 */
bytes_t* btc_get_block_bytes(in3_t*    in3,      /**< the in3-instance*/
                             bytes32_t blockhash /**< the block hash */
);

/**
 * Deserialization helpers
 */
btc_transaction_t*  btc_d_to_tx(d_token_t* t);           /** Deserializes a `btc_transaction_t` type. You must free the result with free() after use! */
btc_blockheader_t*  btc_d_to_blockheader(d_token_t* t);  /** Deserializes a `btc_blockheader_t` type. You must free the result with free() after use! */
btc_block_txids_t*  btc_d_to_block_txids(d_token_t* t);  /** Deserializes a `btc_block_txids_t` type. You must free the result with free() after use! */
btc_block_txdata_t* btc_d_to_block_txdata(d_token_t* t); /** Deserializes a `btc_block_txdata_t` type. You must free the result with free() after use! */

#ifdef __cplusplus
}
#endif

#endif //IN3_BTC_API_H
