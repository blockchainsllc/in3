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

#include "../../../core/client/verifier.h"
#include "../../../core/util/mem.h"
#include "big.h"
#include "evm.h"
#include "evm_mem.h"
#include "gas.h"
#include <string.h>
#ifdef EVM_GAS
#include "accounts.h"
#endif
// free a evm-instance
void evm_free(evm_t* evm) {
  if (evm->last_returned.data) _free(evm->last_returned.data);
  if (evm->return_data.data) _free(evm->return_data.data);
  if (evm->stack.b.data) _free(evm->stack.b.data);
  if (evm->memory.b.data) _free(evm->memory.b.data);
  if (evm->invalid_jumpdest) _free(evm->invalid_jumpdest);

#ifdef EVM_GAS
  logs_t* l = NULL;
  while (evm->logs) {
    l = evm->logs;
    _free(l->data.data);
    _free(l->topics.data);
    evm->logs = l->next;
    _free(l);
  }

  account_t* ac = NULL;
  storage_t* s  = NULL;

  while (evm->accounts) {
    ac = evm->accounts;
    s  = NULL;
    while (ac->storage) {
      s           = ac->storage;
      ac->storage = s->next;
      _free(s);
    }
    evm->accounts = ac->next;
    _free(ac);
  }
#endif
}

/**
 * sets the default for the evm.
 */
int evm_prepare_evm(evm_t*      evm,
                    address_t   address,
                    address_t   account,
                    address_t   origin,
                    address_t   caller,
                    evm_get_env env,
                    void*       env_ptr,
                    wlen_t      mode) {
  evm->stack.b.data = _malloc(64);
  evm->stack.b.len  = 0;
  evm->stack.bsize  = 64;

  evm->memory.b.data = _calloc(32, 1);
  evm->memory.b.len  = 0;
  evm->memory.bsize  = 32;
  memset(evm->memory.b.data, 0, 32);

  evm->stack_size       = 0;
  evm->invalid_jumpdest = NULL;

  evm->pos   = 0;
  evm->state = EVM_STATE_INIT;

  evm->last_returned.data = NULL;
  evm->last_returned.len  = 0;

  evm->properties = EVM_PROP_CONSTANTINOPL;

  evm->env      = env;
  evm->env_ptr  = env_ptr;
  evm->chain_id = 1;

  evm->gas_price.data = NULL;
  evm->gas_price.len  = 0;

  evm->call_data.data = NULL;
  evm->call_data.len  = 0;

  evm->call_value.data = NULL;
  evm->call_value.len  = 0;

  evm->return_data.data = NULL;
  evm->return_data.len  = 0;

  evm->caller  = caller;
  evm->origin  = origin;
  evm->account = (mode == EVM_CALL_MODE_CALLCODE) ? address : account;
  evm->address = address;

#ifdef EVM_GAS
  evm->accounts = NULL;
  evm->gas      = 0;
  evm->logs     = NULL;
  evm->parent   = NULL;
  evm->refund   = 0;
  evm->init_gas = 0;
#endif

  // if the address is NULL this is a CREATE-CALL, so don't try to fetch the code here.
  if (address) {
    // get the code
    uint8_t* tmp = NULL;
    int      l   = env(evm, EVM_ENV_CODE_SIZE, account, 20, &tmp, 0, 32);
    // error?
    if (l < 0) return l;
    evm->code.len = bytes_to_int(tmp, l);

    // copy the code or return error
    l = env(evm, EVM_ENV_CODE_COPY, account, 20, &evm->code.data, 0, 0);
    return l < 0 ? l : 0;
  } else
    return 0;
}

/**
 * handle internal calls.
 */
int evm_sub_call(evm_t*    parent,
                 address_t address,
                 address_t code_address,
                 uint8_t* value, wlen_t l_value,
                 uint8_t* data, uint32_t l_data,
                 address_t caller,
                 address_t origin,
                 uint64_t  gas,
                 wlen_t    mode,
                 uint32_t out_offset, uint32_t out_len

) {

  UNUSED_VAR(gas);

  // create a new evm
  evm_t evm;
  int   res = evm_prepare_evm(&evm, address, code_address, origin, caller, parent->env, parent->env_ptr, mode), success = 0;

  evm.properties      = parent->properties;
  evm.chain_id        = parent->chain_id;
  evm.call_data.data  = data;
  evm.call_data.len   = l_data;
  evm.call_value.data = value;
  evm.call_value.len  = l_value;

  // if this is a static call, we set the static flag which can be checked before any state-chage occur.
  if (mode == EVM_CALL_MODE_STATIC) evm.properties |= EVM_PROP_STATIC;

  account_t* new_account = NULL;
  UNUSED_VAR(new_account);
  UPDATE_SUBCALL_GAS(evm, parent, address, code_address, caller, gas, mode, value, l_value);

  // execute the internal call
  if (res == 0) success = evm_run(&evm, code_address);

  // put the success in the stack ( in case of a create we add the new address)
  if (!address && success == 0)
    res = evm_stack_push(parent, evm.account, 20);
  else
    res = evm_stack_push_int(parent, (success == 0 || success == EVM_ERROR_SUCCESS_CONSUME_GAS) ? 1 : 0);

  // if we have returndata we write them into memory
  if ((success == 0 || success == EVM_ERROR_SUCCESS_CONSUME_GAS) && evm.return_data.data) {
    // if we have a target to write the result to we do.
    if (out_len) res = evm_mem_write(parent, out_offset, evm.return_data, out_len);

    UPDATE_ACCOUNT_CODE(&evm, new_account);

    // move the return_data to parent.
    if (res == 0) {
      if (parent->last_returned.data) _free(parent->last_returned.data);
      parent->last_returned = evm.return_data;
      evm.return_data.data  = NULL;
      evm.return_data.len   = 0;
    }
  }
  FINALIZE_SUBCALL_GAS(&evm, success, parent);
  // clean up
  evm_free(&evm);
  // we always return 0 since a failure simply means we write a 0 on the stack.
  return res;
}

/**
 * run a evm-call
 */
int evm_call(void*     vc,
             address_t address,
             uint8_t* value, wlen_t l_value,
             uint8_t* data, uint32_t l_data,
             address_t caller,
             uint64_t  gas,
             uint64_t  chain_id,
             bytes_t** result) {

  evm_t evm;
  int   res    = evm_prepare_evm(&evm, address, address, caller, caller, in3_get_env, vc, 0);
  evm.chain_id = chain_id;

  // check if the caller is empty
  uint8_t* ccaller = caller;
  int      l       = 20;
  optimize_len(ccaller, l);

#ifdef EVM_GAS
  // we only transfer initial value if the we have caller
  if (res == 0 && l > 1) res = transfer_value(&evm, caller, address, value, l_value, 0);
#else
  UNUSED_VAR(gas);
  UNUSED_VAR(value);
  UNUSED_VAR(l_value);
#endif

//  evm.properties     = EVM_PROP_DEBUG;
#ifdef EVM_GAS
  evm.gas = gas;
#endif
  evm.call_data.data = data;
  evm.call_data.len  = l_data;
  if (res == 0) res = evm_run(&evm, address);
  if (res == 0 && evm.return_data.data)
    *result = b_dup(&evm.return_data);
  evm_free(&evm);

  return res;
}
