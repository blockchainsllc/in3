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
account_t* evm_get_account(evm_t* evm, uint8_t adr[20], uint_fast8_t create) {
  if (!adr) return NULL;
  account_t* ac = evm->root->accounts;

  // check if we already have the account.
  while (ac) {
    if (memcmp(ac->address, adr, 20) == 0) return ac;
    ac = ac->next;
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
    ac->storage         = NULL;
    ac->next            = evm->root->accounts;
    evm->root->accounts = ac;

    // set balance & nonce
    uint256_set(balance, l_balance, ac->balance);
    uint256_set(nonce, l_nonce, ac->nonce);
  }
  return ac;
}

storage_t* evm_get_storage(evm_t* evm, uint8_t adr[20], uint8_t* s_key, uint_fast8_t s_key_len, uint_fast8_t create) {
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

  // get storage value from env
  int l = evm->env(evm, EVM_ENV_STORAGE, s_key, s_key_len, &data, 0, 0);

  // if it does not exist and we have a value, we set it
  if (create || l >= 0) {
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

/**
 * sets a variable value to 32byte word.
 */
void uint256_set(uint8_t* src, uint_fast8_t src_len, uint8_t dst[32]) {
  if (src_len < 32) memset(dst, 0, 32 - src_len);
  memcpy(dst + 32 - src_len, src, src_len);
}

/**
 * transfer a value to a account.
 */
int transfer_value(evm_t* evm, uint8_t from_account[20], uint8_t to_account[20], uint8_t* value, uint_fast8_t value_len, uint32_t base_gas) {
  if (big_is_zero(value, value_len)) return 0;
  account_t* ac_from = evm_get_account(evm, from_account, true);
  account_t* ac_to   = evm_get_account(evm, to_account, false);
  uint8_t    tmp[32];

#ifdef EVM_GAS
  if (!ac_to) {
    // to account does exisz, so we create it and manage gas for new account
    subgas(G_NEWACCOUNT);
    ac_to = evm_get_account(evm, to_account, true);
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
  uint256_set(tmp, big_add(ac_from->balance, 32, value, value_len, tmp, 32), ac_to->balance);

  return 0;
}
#endif

/**
 * sets the default for the evm.
 */
int evm_prepare_evm(evm_t*      evm,
                    uint8_t     address[20],
                    uint8_t     account[20],
                    uint8_t     origin[20],
                    uint8_t     caller[20],
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
  evm->root     = evm;
#endif

  // get the code
  uint8_t* tmp;
  int      l = env(evm, EVM_ENV_CODE_SIZE, account, 20, &tmp, 0, 32);
  // error?
  if (l < 0) return l;
  evm->code.len = bytes_to_int(tmp, l);

  // copy the code or return error
  return env(evm, EVM_ENV_CODE_COPY, account, 20, &evm->code.data, 0, 0);
}

/**
 * handle internal calls.
 */
int evm_sub_call(evm_t*   parent,
                 uint8_t  address[20],
                 uint8_t  code_address[20],
                 uint8_t* value, uint_fast8_t l_value,
                 uint8_t* data, uint32_t l_data,
                 uint8_t      caller[20],
                 uint8_t      origin[20],
                 uint64_t     gas,
                 uint_fast8_t mode,
                 uint32_t out_offset, uint32_t out_len

) {
  // create a new evm
  evm_t evm;
  int   res          = evm_prepare_evm(&evm, address, code_address, origin, caller, parent->env, parent->env_ptr);
  evm.call_data.data = data;
  evm.call_data.len  = l_data;
  evm.properties     = parent->properties;

  // if this is a static call, we set the static flag which can be checked before any state-chage occur.
  if (mode == EVM_CALL_MODE_STATIC) evm.properties |= EVM_PROP_STATIC;

#ifdef EVM_GAS

  // give the call the amount of gas
  evm.gas = gas;

  // and try to transfer the value
  if (res == 0) res = transfer_value(&evm, parent->account, address, value, l_value, G_CALLVALUE);
  if (res == 0) {
    // if we don't even have enough gas
    if (parent->gas < gas)
      res = EVM_ERROR_OUT_OF_GAS;
    else
      parent->gas -= gas;

    // inherit root-evm
    evm.root = parent->root;

    // reduce the gas based on the length of the data (which is not zero)
    for (uint32_t i = 0; i < evm.call_data.len; i++)
      evm.gas -= evm.call_data.data[i] ? G_TXDATA_NONZERO : G_TXDATA_ZERO;
  }

#else
  UNUSED_VAR(value);
  UNUSED_VAR(gas);
  UNUSED_VAR(l_value);
#endif

  // if we have a value and this should be static we throw
  if (mode == EVM_CALL_MODE_STATIC && l_value > 1) res = EVM_ERROR_UNSUPPORTED_CALL_OPCODE;

  // execute the internal call
  if (res == 0) res = evm_run(&evm);

  // put the success in the stack
  evm_stack_push_int(parent, res == 0 ? 1 : 0);

  // if we have returndata we write them into memory
  if (res == 0 && evm.return_data.data) {
    if (out_len) res = evm_mem_write(parent, out_offset, evm.return_data, out_len);
    if (res == 0) {
      if (parent->last_returned.data) _free(parent->last_returned.data);
      parent->last_returned = evm.return_data;
      evm.return_data.data  = NULL;
      evm.return_data.len   = 0;
    }
  }

#ifdef EVM_GAS
  // if we have gas left and it was successfull we returen it to the parent process.
  if (res == 0) parent->gas += evm.gas;
#endif

  // clean up
  evm_free(&evm);

  // we always return 0 since a failure simply means we write a 0 on the stack.
  return 0;
}

/**
 * run a evm-call
 */
int evm_call(in3_vctx_t* vc,
             uint8_t     address[20],
             uint8_t* value, uint_fast8_t l_value,
             uint8_t* data, uint32_t l_data,
             uint8_t   caller[20],
             uint64_t  gas,
             bytes_t** result) {

  evm_t evm;
  int   res = evm_prepare_evm(&evm, address, address, caller, caller, in3_get_env, vc);

  // check if the caller is empty
  uint8_t* ccaller = caller;
  int      l       = 20;
  optimize_len(ccaller, l);

#ifdef EVM_GAS
  evm.root = &evm;
  // we only transfer initial value if the we have caller
  if (res == 0 && l > 1) res = transfer_value(&evm, caller, address, value, l_value, 0);
#else
  if (value == NULL || l_value < 0) (void) gas;
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
