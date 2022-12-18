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
 * Ethereum Nano verification.
 * */

#ifndef in3_signer_h__
#define in3_signer_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "client.h"
#include "plugin.h"

/**
 *  calcuzlates the adddress from a private key.
 *
 * returns the number of of bytes if successfull or the negative error code if not.
 */
int eth_get_address(uint8_t* pk, uint8_t* address, in3_curve_type_t type);

/**
 * registeres a private key as signer
 */
bool signer_add_key(in3_t* c, bytes32_t pk, in3_curve_type_t type);

/**
 * simply signer with one private key.
 *
 * since the pk pointting to the 32 byte private key is not cloned, please make sure, you manage memory allocation correctly!
 */
in3_ret_t eth_set_pk_signer(in3_t* in3, bytes32_t pk, in3_curve_type_t type, uint8_t** address);

/**
 * registers pk signer as plugin so you can use config or in3_addKeys as rpc
 */
in3_ret_t eth_register_pk_signer(in3_t* in3);

/** sets the signer and a pk to the client*/
in3_ret_t eth_set_request_signer(in3_t* in3, bytes32_t pk);

/**
 * simply signer with one private key as hex.
 */
void eth_set_pk_signer_hex(in3_t* in3, char* key);

/**
 * if the key are hex-data the key is taken as raw private key.
 * if the key is a seedphrase (bip39) the path will be used to derrive the key. (path could also contain comma seperated list of paths to derrive multiple keys)
 */
char* eth_set_pk_signer_from_string(in3_t* in3, char* key, char* path, char* passphrase);

/** Signs message after hashing it with hasher function given in 'hasher_t', with the given private key*/
in3_ret_t ec_sign_pk_hash(uint8_t* message, size_t len, uint8_t* pk, d_digest_type_t hasher, uint8_t* dst);

/** hashes the msg by adding the Ethereum Signed Message-Prefix */
void eth_create_prefixed_msg_hash(bytes32_t dst, bytes_t msg);

/** signs with a pk bases on the type. This function allocates memory on the heap (result.data) and must be freed after use! */
bytes_t sign_with_pk(const bytes32_t pk, const bytes_t data, const d_digest_type_t type);

/** adds a path to a hd signer */
in3_ret_t hd_signer_add_path(in3_t* in3, bytes32_t seed_id, char* path, uint8_t** address);
/** sets the signer and a pk to the client*/
in3_ret_t register_hd_signer(in3_t* in3, bytes_t seed, in3_curve_type_t type, bytes32_t seed_id);

#ifdef __cplusplus
}
#endif

#endif
