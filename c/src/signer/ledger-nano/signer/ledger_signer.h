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
 * this file defines the incubed configuration struct and it registration.
 * 
 * 
 * */

#ifndef in3_ledger_signer_h__
#define in3_ledger_signer_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../core/client/client.h"

/**
 * attaches ledger nano hardware wallet signer  with incubed .
 * 
 * bip32 path to be given to point the specific public/private key in HD tree for Ethereum!
 */
in3_ret_t eth_ledger_set_signer(in3_t* in3, uint8_t* bip_path);

/**
 * returns public key at the bip_path .
 * 
 * returns IN3_ENODEVICE error if ledger nano device is not connected 
 */
in3_ret_t eth_ledger_get_public_key(uint8_t* bip_path, uint8_t* public_key);

#ifdef __cplusplus
}
#endif

#endif