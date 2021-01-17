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

#include "../../../core/util/bytes.h"
#include "evm.h"
#ifndef evm_mem_h__
#define evm_mem_h__

#define MEM_LIMIT 0xFFFFFFF // this cost about 8M gas
//#define MEM_INT_LIMIT 3   // bytes
int mem_check(evm_t* evm, uint32_t max_pos, uint8_t read_only);

int evm_mem_read_ref(evm_t* evm, uint32_t off, uint32_t len, bytes_t* src);
int evm_mem_read(evm_t* evm, bytes_t mem_off, uint8_t* dst, uint32_t len);
int evm_mem_readi(evm_t* evm, uint32_t off, uint8_t* dst, uint32_t len);
int evm_mem_write(evm_t* evm, uint32_t mem_off, bytes_t src, uint32_t len);

#endif