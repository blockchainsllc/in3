/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 *
 * Copyright (C) 2019 Blockchains, LLC
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
 * 
 * If you cannot meet the requirements of AGPL, 
 * you should contact us to inquire about a commercial license.
 *******************************************************************************/

#include "evm.h"
#include <string.h>

/** reads a account from the enviroment. */
account_t* evm_get_account(evm_t* evm, address_t adr, wlen_t create);

/** get account storage */
storage_t* evm_get_storage(evm_t* evm, address_t adr, uint8_t* s_key, wlen_t s_key_len, wlen_t create);

/** copy state. */
void copy_state(evm_t* dst, evm_t* src);

int transfer_value(evm_t* current, address_t from_account, address_t to_account, uint8_t* value, wlen_t value_len, uint32_t base_gas);

account_t* evm_create_account(evm_t* evm, uint8_t* data, uint32_t l_data, address_t code_address, address_t caller);
