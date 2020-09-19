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

#include "../../core/util/bytes.h"
#include "../../core/util/data.h"
#ifndef _ETH_API__ABI2_H_
#define _ETH_API__ABI2_H_

typedef enum {
  ABI_TUPLE       = 1,
  ABI_STRING      = 2,
  ABI_NUMBER      = 3,
  ABI_BYTES       = 4,
  ABI_ADDRESS     = 5,
  ABI_FIXED_BYTES = 6,
  ABI_BOOL        = 8,
  ABI_ARRAY       = 9

} abi_coder_type_t;

typedef struct signature {
  abi_coder_type_t type;
  union {
    struct {
      struct signature** components;
      int                len;
    } tuple;

    struct {
      struct signature* component;
      int               len;
    } array;

    struct {
      bool sign;
      int  size;
    } number;

    struct {
      int len;
    } fixed;

  } data;
} abi_coder_t;

typedef struct {
  abi_coder_t* input;
  abi_coder_t* output;
  uint8_t      fn_hash[4];
} abi_sig_t;

void        abi_sig_free(abi_sig_t* c);
abi_sig_t*  abi_sig_create(char* signature, char** error);
bool        abi_is_dynamic(abi_coder_t* coder);
bytes_t     abi_encode(abi_sig_t* s, d_token_t* src, char** error);
json_ctx_t* abi_decode(abi_sig_t* s, bytes_t data, char** error);
#endif // _ETH_API__ABI_H_
