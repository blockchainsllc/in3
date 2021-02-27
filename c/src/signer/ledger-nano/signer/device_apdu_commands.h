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

#ifndef in3_device_apdu_h__
#define in3_device_apdu_h__

#include "../../../core/client/client.h"
#include "../../../third-party/hidapi/hidapi/hidapi.h"

#define HID_CMD_MIN_LEN  64
#define HID_CMD_MID_LEN  128
#define HID_CMD_HIGH_LEN 255

extern uint8_t CLA;
extern uint8_t INS_GET_PUBLIC_KEY;
extern uint8_t INS_GET_PUB_ADDR;
extern uint8_t INS_SIGN_MSG;
extern uint8_t INS_SIGN_TX;
extern uint8_t INS_SIGN;
extern uint8_t P1_MORE;
extern uint8_t P1_FINAL;
extern uint8_t P2_FINAL;
extern uint8_t TAG;

int len_to_bytes(uint16_t x, uint8_t* buf);

uint16_t bytes_to_len(uint8_t* buf);

void wrap_apdu(uint8_t* i_apdu, int len, uint16_t seq, bytes_t* wrapped_hid_cmd);

void read_hid_response(hid_device* handle, bytes_t* response);

int write_hid(hid_device* handle, uint8_t* data, int len);

hid_device* open_device();

void close_device(hid_device* handle);

#endif