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

#include "accounts.h"
#include "../../../core/util/mem.h"
#include "../../../core/util/utils.h"
#include "big.h"
#include "gas.h"
#ifdef EVM_GAS
/**
 * sets a variable value to 32byte word.
 */

account_t* evm_get_account(evm_t* evm, address_t adr, wlen_t create) {
  if (!adr) return NULL;
  account_t* ac = evm->accounts;

  // check if we already have the account.
  while (ac) {
    if (memcmp(ac->address, adr, 20) == 0) return ac;
    ac = ac->next;
  }

  // if this is a internal call take it from the parent
  if (evm->parent) {
    ac = evm_get_account(evm->parent, adr, create);

    if (ac) {
      // clone and add account
      account_t* a = _malloc(sizeof(account_t));
      memcpy(a, ac, sizeof(account_t));
      a->storage    = NULL;
      a->next       = evm->accounts;
      evm->accounts = a;
      return a;
    }
  }

  // get balance, nonce and code
  uint8_t *balance, *nonce, *code_size;
  int      l_balance   = evm->env(evm, EVM_ENV_BALANCE, adr, 20, &balance, 0, 0);
  int      l_code_size = evm->env(evm, EVM_ENV_CODE_SIZE, adr, 20, &code_size, 0, 0);
  int      l_nonce     = evm->env(evm, EVM_ENV_NONCE, adr, 20, &nonce, 0, 0);

  if (l_balance >= 0) optimize_len(balance, l_balance);
  if (l_nonce >= 0) optimize_len(nonce, l_nonce);
  if (l_code_size >= 0) optimize_len(code_size, l_code_size);

  // is this a non-empty account? (or do we have to create one)
  if (create || l_balance > 1 || l_nonce > 1 || l_code_size > 1 || (l_balance == 1 && *balance) || (l_nonce == 1 && *nonce) || (l_code_size == 1 && *code_size)) {
    ac = _calloc(1, sizeof(account_t));
    memcpy(ac->address, adr, 20);

    // get the code (if code_size>0)
    ac->code.data = NULL;
    ac->code.len  = bytes_to_int(code_size, l_code_size);
    if (ac->code.len)
      evm->env(evm, EVM_ENV_CODE_COPY, adr, 20, &ac->code.data, 0, 0);

    // add to accounts
    ac->storage   = NULL;
    ac->next      = evm->accounts;
    evm->accounts = ac;

    // set balance & nonce
    uint256_set(balance, l_balance, ac->balance);
    uint256_set(nonce, l_nonce, ac->nonce);
  }
  return ac;
}

account_t* evm_create_account(evm_t* evm, uint8_t* data, uint32_t l_data, address_t code_address, address_t caller) {

  account_t* new_account = NULL;
  new_account            = evm_get_account(evm, code_address, 1);
  // this is a create-call
  evm->code              = bytes(data, l_data);
  evm->call_data.len     = 0;
  evm->address           = code_address;
  new_account->nonce[31] = 1;

  // increment the nonce of the sender
  account_t* sender_account = evm_get_account(evm, caller, 1);
  bytes32_t  new_nonce;
  uint8_t    one = 1;
  uint256_set(new_nonce, big_add(sender_account->nonce, 32, &one, 1, new_nonce, 32), sender_account->nonce);
  return new_account;
}

storage_t* evm_get_storage(evm_t* evm, address_t adr, uint8_t* s_key, wlen_t s_key_len, wlen_t create) {
  account_t* ac = evm_get_account(evm, adr, create);
  if (!ac) return NULL;

  storage_t* s = ac->storage;

  // create full word key
  uint8_t key_data[32], *data;
  uint256_set(s_key, s_key_len, key_data);

  // find existing entry
  while (s) {
    if (memcmp(s->key, key_data, 32) == 0) return s;
    s = s->next;
  }

  // not found?, but if we have parents, we try to copy the entry from there first
  if (evm->parent) {
    storage_t* parent_s = evm_get_storage(evm->parent, adr, s_key, s_key_len, create);
    if (parent_s) {
      // clone and add account
      s = _malloc(sizeof(storage_t));
      memcpy(s, parent_s, sizeof(storage_t));
      s->next     = ac->storage;
      ac->storage = s;
      return s;
    }
  }

  // get storage value from env
  int l = evm->env(evm, EVM_ENV_STORAGE, s_key, s_key_len, &data, 0, 0);

  // if it does not exist and we have a value, we set it
  if (create || l > 1 || (l == 1 && *data)) {
    // create with key
    s = _malloc(sizeof(storage_t));
    memcpy(s->key, key_data, 32);

    // add to account
    s->next     = ac->storage;
    ac->storage = s;

    // set the value
    uint256_set(data, l, s->value);
  }
  return s;
}

void copy_state(evm_t* dst, evm_t* src) {

  // first move all logs
  if (src->logs) {
    logs_t* last = src->logs;
    while (last->next) last = last->next;

    last->next = dst->logs;
    dst->logs  = src->logs;
    src->logs  = NULL;
  }

  account_t *sa = src->accounts, *prv = NULL;
  while (sa) {
    // find the account in the dst-state
    account_t *a = dst->accounts, *da = NULL;
    while (a) {
      if (memcmp(a->address, sa->address, 20) == 0) {
        da = a;
        break;
      }
      a = a->next;
    }
    if (!da) {
      // the account does not exist yet, so we simply move it

      // remove from src
      if (prv == NULL)
        src->accounts = sa->next;
      else
        prv->next = sa->next;

      // add to dst
      sa->next      = dst->accounts;
      dst->accounts = sa;

      // next item...
      sa = prv == NULL ? src->accounts : prv->next;
      continue;
    } else {
      // clone data
      memcpy(da->balance, sa->balance, 32);
      memcpy(da->nonce, sa->nonce, 32);
      da->code = sa->code;

      // clone storage
      storage_t *ss = sa->storage, *ps = NULL, *ds, *s;
      while (ss) {
        // find the storage in the parent
        ds = NULL;
        s  = da->storage;
        while (s) {
          if (memcmp(s->key, ss->key, 32) == 0) {
            ds = s;
            break;
          }
          s = s->next;
        }

        if (ds)
          memcpy(ds->value, ss->value, 32);
        else {
          // move the storage to the parent
          // remove from src
          if (ps == NULL)
            sa->storage = ss->next;
          else
            ps->next = ss->next;

          // add to dst
          ss->next    = da->storage;
          da->storage = ss;

          // next item...
          ss = ps == NULL ? sa->storage : ps->next;
          continue;
        }

        ps = ss;
        ss = ss->next;
      }
    }
    prv = sa;
    sa  = sa->next;
  }
}

/**
 * transfer a value to a account.
 */
int transfer_value(evm_t* current, address_t from_account, address_t to_account, uint8_t* value, wlen_t value_len, uint32_t base_gas) {
  if (big_is_zero(value, value_len)) return 0;

  // while the gas is handled by the parent, the new state is handled in the current evm, so we can roll it back.
  evm_t* evm = current->parent ? current->parent : current;

  account_t* ac_from = evm_get_account(current, from_account, true);
  account_t* ac_to   = evm_get_account(current, to_account, false);
  uint8_t    tmp[32], val[32];

  // we clone it because the value may point to the value we want to change.
  memcpy(val, value, value_len);
  value = val;

  if (!ac_to) {
    // to account does exist, so we create it and manage gas for new account
    subgas(G_NEWACCOUNT);
    ac_to = evm_get_account(current, to_account, true);
  }
  subgas(base_gas);

  if (ac_from) {
    // check if the balance of the sender is high enough
    if (big_cmp(ac_from->balance, 32, value, value_len) < 0) return EVM_ERROR_BALANCE_TOO_LOW;

    // sub balance from sender
    uint256_set(tmp, big_sub(ac_from->balance, 32, value, value_len, tmp), ac_from->balance);
  }

  // add balance to receiver. (This will be executed) even if the sender is null (which means initial setup for test)
  uint256_set(tmp, big_add(ac_to->balance, 32, value, value_len, tmp, 32), ac_to->balance);

  return 0;
}
#endif
