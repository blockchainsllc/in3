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
#ifndef in3_types_h__
#define in3_types_h__

#define LEDGER_NANOS_VID 0x2C97
// #define LEDGER_NANOS_PID 0x1015
#define LEDGER_NANOS_PID 0x00

typedef enum CURVE_CODE_ {
  IDM_ED        = 0,
  IDM_SECP256K1 = 1,
  IDM_SECP256R1 = 2,
  IDM_NO_CURVE  = 255,
} CURVE_CODE;

typedef struct TXN_ {
  bytes_t nonce;
  bytes_t gasprice;
  bytes_t startgas;
  bytes_t to;
  bytes_t value;
  bytes_t data;
  bytes_t v; //chain id
  bytes_t r;
  bytes_t s;
} TXN;

#endif