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
 * Merkle Proof Verification.
 * 
 * */

#include "../../../core/util/bytes.h"

#ifndef MERKLE_H
#define MERKLE_H

#ifndef MERKLE_DEPTH_MAX
#define MERKLE_DEPTH_MAX 64
#endif

/**
 *  verifies a merkle proof.
 * 
 *
 * expectedValue == NULL     : value must not exist
 * expectedValue.data ==NULL : please copy the data I want to evaluate it afterwards.
 * expectedValue.data !=NULL : the value must match the data.
 * 
 * \param rootHash the expected root hash of the trie.
 * \param path the path the the value
 * \param proof a array of bytes of rlp encoded nodes. This array must be terminatzed with a NULL-Pointer.
 * \param expectedValue a byte-object with the raw value. If the data-pointer is NULL, it will be set to the last leaf and can be checked afterwards. 
 */
int trie_verify_proof(bytes_t* rootHash, bytes_t* path, bytes_t** proof, bytes_t* expectedValue);

/**
 * helper function split a path into 4-bit nibbles.
 * 
 * The result must be freed after use!
 * 
 * \param path the path of bytes.
 * \param use_prefix if true (or 1) the first byte of the path is interpreded as the leaf or extension marker.
 * 
 * \return the resulting bytes represent a 4bit-number each and are terminated with a 0xFF. 
 * 
 */
uint8_t* trie_path_to_nibbles(bytes_t path, int use_prefix);

/**
 * helper function to find the number of nibbles matching both paths.
 */
int trie_matching_nibbles(uint8_t* a, uint8_t* b);

/**
 * used to free the NULL-terminated proof-array.
 */
void trie_free_proof(bytes_t** proof);
#endif
