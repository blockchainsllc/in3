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
 * Ethereum Nanon verification.
 * */

#ifndef in3_eth_nano_h__
#define in3_eth_nano_h__

#include "../../../core/client/verifier.h"

/** entry-function to execute the verification context. */
in3_ret_t in3_verify_eth_nano(in3_vctx_t* v);

/** verifies a blockheader. */
in3_ret_t eth_verify_blockheader(in3_vctx_t* vc, bytes_t* header, bytes_t* expected_blockhash);

/** 
 * verifies a single signature blockheader.
 * 
 * This function will return a positive integer with a bitmask holding the bit set according to the address that signed it. 
 * This is based on the signatiures in the request-config.
 * 
 */
int eth_verify_signature(in3_vctx_t* vc, bytes_t* msg_hash, d_token_t* sig);

/**
 *  returns the address of the signature if the msg_hash is correct
 */
bytes_t* ecrecover_signature(bytes_t* msg_hash, d_token_t* sig);

/**
 * verifies a transaction receipt.
 */
in3_ret_t eth_verify_eth_getTransactionReceipt(in3_vctx_t* vc, bytes_t* tx_hash);

/**
 * verifies the nodelist.
 */
in3_ret_t eth_verify_in3_nodelist(in3_vctx_t* vc, uint32_t node_limit, bytes_t* seed, d_token_t* required_addresses);

/**
 * verifies the nodelist.
 */
in3_ret_t eth_verify_in3_whitelist(in3_vctx_t* vc);

/**
 * this function should only be called once and will register the eth-nano verifier.
 */
void in3_register_eth_nano();

/**
 * helper function to rlp-encode the transaction_index.
 * 
 * The result must be freed after use!
 */
bytes_t* create_tx_path(uint32_t index);

#endif // in3_eth_nano_h__