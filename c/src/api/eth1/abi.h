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

#if !defined(_ETH_API__ABI_H_)
#define _ETH_API__ABI_H_

typedef enum {
  A_UINT    = 1,
  A_INT     = 2,
  A_BYTES   = 3,
  A_BOOL    = 4,
  A_ADDRESS = 5,
  A_TUPLE   = 6,
  A_STRING  = 7
} atype_t;

typedef struct el {
  atype_t type;
  bytes_t data;
  uint8_t type_len;
  int     array_len;
} var_t;

typedef struct {
  var_t*           in_data;
  var_t*           out_data;
  bytes_builder_t* call_data;
  var_t*           current;
  char*            error;
  int              data_offset;
} call_request_t;

call_request_t* parseSignature(char* sig);
json_ctx_t*     req_parse_result(call_request_t* req, bytes_t data);
void            req_free(call_request_t* req);
int             set_data(call_request_t* req, d_token_t* data, var_t* tuple);
var_t*          t_next(var_t* t);
int             word_size(int b);

#endif // _ETH_API__ABI_H_
