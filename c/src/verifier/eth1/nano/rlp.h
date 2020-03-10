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
 * RLP-En/Decoding as described in the [Ethereum RLP-Spec](https://github.com/ethereum/wiki/wiki/RLP).
 * 
 * This decoding works without allocating new memory.
 * */

#ifndef rlp_h__
#define rlp_h__

#include "../../../core/util/bytes.h"

/**
 * this function decodes the given bytes and returns the element with the given index by updating the reference of dst.
 * 
 * the bytes will only hold references and do **not** need to be freed!
 *
 * ```c
 * bytes_t* tx_raw = serialize_tx(tx);
 * 
 * bytes_t item;
 * 
 * // decodes the tx_raw by letting the item point to range of the first element, which should be the body of a list.
 * if (rlp_decode(tx_raw, 0, &item) !=2) return -1 ;
 * 
 * // now decode the 4th element (which is the value) and let item point to that range.
 * if (rlp_decode(&item, 4, &item) !=1) return -1 ;
 *  
 * ```
 * \param b the ptr to the incoming bytes to decode.
 * \param index the element to decode (if the index<0 the function returns the number of elements found.)
 * \param dst the bytes to store the range found. (only the data and len-property will be modified to point to the correct range.)
 * 
 * \return
 * - 0 : means item out of range
 * - 1 : item found
 * - 2 : list found ( you can then decode the same bytes again) 
 */
int rlp_decode(bytes_t* b, int index, bytes_t* dst);

/**
 * this function expects a list item (like the blockheader as first item and will then find the item within this list).
 * 
 * It is a shortcut for 
 * 
 * ```c
 *   // decode the list
 *   if (rlp_decode(b,0,dst)!=2) return 0;
 *   // and the decode the item
 *   return rlp_decode(dst,index,dst);
 * ```
 * \param b the ptr to the incoming bytes to decode.
 * \param index the element to decode (if the index<0 the function returns the number of elements found.)
 * \param dst the bytes to store the range found. (only the data and len-property will be modified to point to the correct range.)
 * 
 * \return
 * - 0 : means item out of range
 * - 1 : item found
 * - 2 : list found ( you can then decode the same bytes again) 
 *
 */
int rlp_decode_in_list(bytes_t* b, int index, bytes_t* dst);

/**
 * returns the number of elements found in the data.
 * 
 * \param b the ptr to the incoming bytes to decode.
 */
int rlp_decode_len(bytes_t* b);

/**
 * encode a item as single string and add it to the bytes_builder.
 * 
 * \param bb a builder a dst
 * \param val the bytes to be added.
 * 
 */
void rlp_encode_item(bytes_builder_t* bb, bytes_t* val);

/**
 * encode a the value as list of already encoded items.
 * 
 * \param bb a builder a dst
 * \param val the list to be added.
 * 
 */

void rlp_encode_list(bytes_builder_t* bb, bytes_t* val);

/**
 * converts the data in the builder to a list.
 * 
 * This function is optimized to not increase the memory more than needed and is fastet than 
 * creating a second builder to encode the data. 
 * 
 * \param bb the builder containing the data.
 * 
 * \return the same builder.
 */
bytes_builder_t* rlp_encode_to_list(bytes_builder_t* bb);

/**
 * converts the data in the builder to a rlp-encoded item.
 * 
 * This function is optimized to not increase the memory more than needed and is fastet than 
 * creating a second builder to encode the data. 
 * 
 * \param bb the builder containing the data.
 * 
 * \return the same builder.
 */
bytes_builder_t* rlp_encode_to_item(bytes_builder_t* bb);
/**
 * helper to encode the prefix for a value
 */
void rlp_add_length(bytes_builder_t* bb, uint32_t len, uint8_t offset);
#endif // rlp__