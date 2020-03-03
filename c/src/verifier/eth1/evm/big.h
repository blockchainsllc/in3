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

#ifndef in3_bignum_h__
#define in3_bignum_h__

#include "../../../core/util/bytes.h"
#include <stdint.h>

uint8_t big_is_zero(uint8_t* data, wlen_t l);
void    big_shift_left(uint8_t* a, wlen_t len, int bits);
void    big_shift_right(uint8_t* a, wlen_t len, int bits);
int     big_cmp(const uint8_t* a, const wlen_t len_a, const uint8_t* b, const wlen_t len_b);
int     big_signed(uint8_t* val, wlen_t len, uint8_t* dst);
int32_t big_int(uint8_t* val, wlen_t len);
int     big_add(uint8_t* a, wlen_t len_a, uint8_t* b, wlen_t len_b, uint8_t* out, wlen_t max);
int     big_sub(uint8_t* a, wlen_t len_a, uint8_t* b, wlen_t len_b, uint8_t* out);
int     big_mul(uint8_t* a, wlen_t la, uint8_t* b, wlen_t lb, uint8_t* res, wlen_t max);
int     big_div(uint8_t* a, wlen_t la, uint8_t* b, wlen_t lb, wlen_t sig, uint8_t* res);
int     big_mod(uint8_t* a, wlen_t la, uint8_t* b, wlen_t lb, wlen_t sig, uint8_t* res);
int     big_exp(uint8_t* a, wlen_t la, uint8_t* b, wlen_t lb, uint8_t* res);
int     big_log256(uint8_t* a, wlen_t len);
#endif