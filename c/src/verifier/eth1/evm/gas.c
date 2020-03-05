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

#include "gas.h"

#ifdef EVM_GAS
void init_gas(evm_t* evm) {
  // prepare evm gas
  evm->refund = 0;
  if (!evm->init_gas) evm->init_gas = evm->gas;
}

void update_account_code(evm_t* evm, account_t* new_account) {
  // prepare evm gas
  if (new_account)
    new_account->code = evm->return_data;
}

void evm_init(evm_t* evm) {
  evm->accounts = NULL;
  evm->gas      = 0;
  evm->logs     = NULL;
  evm->parent   = NULL;
  evm->refund   = 0;
  evm->init_gas = 0;
}

void finalize_and_refund_gas(evm_t* evm) {
  uint64_t gas_used = evm->init_gas - evm->gas;
  if ((evm->properties & EVM_PROP_NO_FINALIZE) == 0) {
    // finalize and refund
    if (evm->refund && evm->parent) {
      evm->parent->gas -= gas_used;
      evm->gas += gas_used + evm->refund;
    } else {
      evm->gas += min(evm->refund, gas_used >> 1);
    }
  }
}

void finalize_subcall_gas(evm_t* evm, int success, evm_t* parent) {
  // if it was successfull we copy the new state to the parent
  if ((success == 0 || success == EVM_ERROR_SUCCESS_CONSUME_GAS) && evm->state != EVM_STATE_REVERTED)
    copy_state(parent, evm);
  // if we have gas left and it was successfull we returen it to the parent process.
  if (success == 0 || success == EVM_ERROR_SUCCESS_CONSUME_GAS) parent->gas += evm->gas;
}

#endif