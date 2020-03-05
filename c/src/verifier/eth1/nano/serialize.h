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

/** @file 
 * serialization of ETH-Objects.
 * 
 * This incoming tokens will represent their values as properties based on [JSON-RPC](https://github.com/ethereum/wiki/wiki/JSON-RPC).
 * 
 * */

#include "../../../core/util/data.h"

/**
 * creates rlp-encoded raw bytes for a receipt.
 * 
 * The bytes must be freed with b_free after use!
 * 
 * \param receipt the json-onject as descibed in [eth_getTransactionReceipt](https://github.com/ethereum/wiki/wiki/JSON-RPC#eth_gettransactionreceipt)
 */
bytes_t* serialize_tx_receipt(d_token_t* receipt);
/**
 * creates rlp-encoded raw bytes for a transaction.
 * 
 * The bytes must be freed with b_free after use!
 * 
 * \param receipt the json-onject as descibed in [eth_getTransactionByHash](https://github.com/ethereum/wiki/wiki/JSON-RPC#eth_gettransactionbyhash)
 * 
 */

bytes_t* serialize_tx(d_token_t* tx);

/**
 * creates rlp-encoded raw bytes for a transaction from direct values.
 * 
 * The bytes must be freed with b_free after use!
 * 
 * 
 */
bytes_t* serialize_tx_raw(bytes_t nonce, bytes_t gas_price, bytes_t gas_limit, bytes_t to, bytes_t value, bytes_t data, uint64_t v, bytes_t r, bytes_t s);

/**
 * creates rlp-encoded raw bytes for a account.
 * 
 * The bytes must be freed with b_free after use!
 * 
 * 
 */

bytes_t* serialize_account(d_token_t* a);

/**
 * creates rlp-encoded raw bytes for a blockheader.
 * 
 * The bytes must be freed with b_free after use!
 * 
 * \param receipt the json-onject as descibed in [eth_getBlockByHash](https://github.com/ethereum/wiki/wiki/JSON-RPC#eth_getblockbyhash)
 */
bytes_t* serialize_block_header(d_token_t* block);

/**
 * adds the value represented by the token rlp-encoded to the byte_builder.
 * \param ml the minimum number of bytes. if the length is 0, a one byte-string with 0x00 will be written as 0x.
 * \param rlp the builder to add to.
 * \param t the token representing the value. must be a integer, bytes or NULL.
 * 
 * \return 0 if added -1 if the value could not be handled.
 */
int rlp_add(bytes_builder_t* rlp, d_token_t* t, int ml);

// clang-format off

#define BLOCKHEADER_PARENT_HASH       0
#define BLOCKHEADER_SHA3_UNCLES       1
#define BLOCKHEADER_MINER             2
#define BLOCKHEADER_STATE_ROOT        3
#define BLOCKHEADER_TRANSACTIONS_ROOT 4
#define BLOCKHEADER_RECEIPT_ROOT      5
#define BLOCKHEADER_LOGS_BLOOM        6
#define BLOCKHEADER_DIFFICULTY        7
#define BLOCKHEADER_NUMBER            8
#define BLOCKHEADER_GAS_LIMIT         9
#define BLOCKHEADER_GAS_USED         10
#define BLOCKHEADER_TIMESTAMP        11
#define BLOCKHEADER_EXTRA_DATA       12
#define BLOCKHEADER_SEALED_FIELD1    13
#define BLOCKHEADER_SEALED_FIELD2    14
#define BLOCKHEADER_SEALED_FIELD3    15

