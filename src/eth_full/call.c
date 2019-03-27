#include "big.h"
#include "evm.h"
#include "gas.h"
#include "mem.h"
#include <client/verifier.h>
#include <stdlib.h>
#include <string.h>
#include <util/mem.h>
#include <util/utils.h>

// free a evm-instance
void evm_free(evm_t* evm) {
  if (evm->last_returned.data) _free(evm->last_returned.data);
  if (evm->return_data.data) _free(evm->return_data.data);
  if (evm->stack.b.data) _free(evm->stack.b.data);
  if (evm->memory.b.data) _free(evm->memory.b.data);
  if (evm->invalid_jumpdest) _free(evm->invalid_jumpdest);

#ifdef EVM_GAS
  logs_t* l;
  while (evm->logs) {
    l = evm->logs;
    _free(l->data.data);
    _free(l->topics.data);
    evm->logs = l->next;
    _free(l);
  }

  account_t* ac = NULL;
  storage_t* s;

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

#ifdef EVM_GAS

/**
 * reads a account from the enviroment.
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
    ac = _malloc(sizeof(account_t));
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

static void copy_state(evm_t* dst, evm_t* src) {

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
 * sets a variable value to 32byte word.
 */
void uint256_set(uint8_t* src, wlen_t src_len, uint8_t dst[32]) {
  if (src_len < 32) memset(dst, 0, 32 - src_len);
  memcpy(dst + 32 - src_len, src, src_len);
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

#ifdef EVM_GAS
  if (!ac_to) {
    // to account does exist, so we create it and manage gas for new account
    subgas(G_NEWACCOUNT);
    ac_to = evm_get_account(current, to_account, true);
  }
  subgas(base_gas);
#endif

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

/**
 * sets the default for the evm.
 */
int evm_prepare_evm(evm_t*      evm,
                    address_t   address,
                    address_t   account,
                    address_t   origin,
                    address_t   caller,
                    evm_get_env env,
                    void*       env_ptr) {
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

  evm->env     = env;
  evm->env_ptr = env_ptr;

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
  evm->account = account;
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
    uint8_t* tmp;
    int      l = env(evm, EVM_ENV_CODE_SIZE, account, 20, &tmp, 0, 32);
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
  // create a new evm
  evm_t evm;
  int   res = evm_prepare_evm(&evm, address, code_address, origin, caller, parent->env, parent->env_ptr), success = 0;

  evm.properties     = parent->properties;
  evm.call_data.data = data;
  evm.call_data.len  = l_data;

  // if this is a static call, we set the static flag which can be checked before any state-chage occur.
  if (mode == EVM_CALL_MODE_STATIC) evm.properties |= EVM_PROP_STATIC;

#ifdef EVM_GAS
  // inherit root-evm
  evm.parent = parent;

  uint64_t   max_gas_provided = parent->gas - (parent->gas >> 6);
  account_t* new_account      = NULL;

  if (!address) {
    new_account = evm_get_account(&evm, code_address, 1);
    // this is a create-call
    evm.code               = bytes(data, l_data);
    evm.call_data.len      = 0;
    evm.address            = code_address;
    new_account->nonce[31] = 1;

    // increment the nonce of the sender
    account_t* sender_account = evm_get_account(&evm, caller, 1);
    bytes32_t  new_nonce;
    uint8_t    one = 1;
    uint256_set(new_nonce, big_add(sender_account->nonce, 32, &one, 1, new_nonce, 32), sender_account->nonce);

    // handle gas
    gas = max_gas_provided;
  } else
    gas = min(gas, max_gas_provided);

  // give the call the amount of gas
  evm.gas = gas;

  // and try to transfer the value
  if (res == 0 && !big_is_zero(value, l_value)) {
    // if we have a value and this should be static we throw
    if (mode == EVM_CALL_MODE_STATIC)
      res = EVM_ERROR_UNSUPPORTED_CALL_OPCODE;
    else {
      // only for CALL or CALLCODE we add the CALLSTIPEND
      if (!mode && address) evm.gas += G_CALLSTIPEND;
      res = transfer_value(&evm, parent->address, evm.address, value, l_value, (!mode && address) ? G_CALLVALUE : 0);
    }
  }
  if (res == 0) {
    // if we don't even have enough gas
    if (parent->gas < gas)
      res = EVM_ERROR_OUT_OF_GAS;
    else
      parent->gas -= gas;
  }

#else
  UNUSED_VAR(value);
  UNUSED_VAR(gas);
  UNUSED_VAR(l_value);
#endif

  // execute the internal call
  if (res == 0) success = evm_run(&evm);

  // put the success in the stack ( in case of a create we add the new address)
  if (!address && success == 0)
    res = evm_stack_push(parent, evm.account, 20);
  else
    res = evm_stack_push_int(parent, success == 0 ? 1 : 0);

  // if we have returndata we write them into memory
  if (success == 0 && evm.return_data.data) {
    // if we have a target to write the result to we do.
    if (out_len) res = evm_mem_write(parent, out_offset, evm.return_data, out_len);

#ifdef EVM_GAS
    // if we created a new account, we can now copy the return_data as code
    if (new_account)
      new_account->code = evm.return_data;
#endif

    // move the return_data to parent.
    if (res == 0) {
      if (parent->last_returned.data) _free(parent->last_returned.data);
      parent->last_returned = evm.return_data;
      evm.return_data.data  = NULL;
      evm.return_data.len   = 0;
    }
  }

#ifdef EVM_GAS
  // if it was successfull we copy the new state to the parent
  if (success == 0 && evm.state != EVM_STATE_REVERTED)
    copy_state(parent, &evm);

  // if we have gas left and it was successfull we returen it to the parent process.
  if (success == 0) parent->gas += evm.gas;
#endif

  // clean up
  evm_free(&evm);

  // we always return 0 since a failure simply means we write a 0 on the stack.
  return res;
}

/**
 * run a evm-call
 */
int evm_call(in3_vctx_t* vc,
             address_t   address,
             uint8_t* value, wlen_t l_value,
             uint8_t* data, uint32_t l_data,
             address_t caller,
             uint64_t  gas,
             bytes_t** result) {

  evm_t evm;
  int   res = evm_prepare_evm(&evm, address, address, caller, caller, in3_get_env, vc);

  evm.properties |= vc->ctx->client->evm_flags;

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
  if (res == 0) res = evm_run(&evm);
  if (res == 0 && evm.return_data.data)
    *result = b_dup(&evm.return_data);
  evm_free(&evm);

  return res;
}
