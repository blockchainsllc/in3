#include "big.h"
#include "evm.h"
#include "gas.h"
#include "mem.h"
#include <client/verifier.h>
#include <stdlib.h>
#include <string.h>
#include <util/mem.h>
#include <util/utils.h>

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
    //    if (ac->code.data) _free(ac->code.data);
    s = NULL;
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
account_t* evm_get_account(evm_t* evm, uint8_t* adr, uint8_t create) {
  if (!adr) return NULL;
  account_t* ac = evm->root->accounts;
  while (ac) {
    if (memcmp(ac->address, adr, 20) == 0) return ac;
    ac = ac->next;
  }

  uint8_t *data, *nonce, *cs;
  int      l      = evm->env(evm, EVM_ENV_BALANCE, adr, 20, &data, 0, 0);
  int      lcs    = evm->env(evm, EVM_ENV_CODE_SIZE, adr, 20, &cs, 0, 0);
  int      lnonce = evm->env(evm, EVM_ENV_NONCE, adr, 20, &nonce, 0, 0);
  if (l >= 0) optimize_len(data, l);
  if (lnonce >= 0) optimize_len(nonce, lnonce);
  if (lcs >= 0) optimize_len(cs, lcs);

  if (create || l > 1 || lnonce > 1 || lcs > 1 || (l == 1 && *data) || (lnonce == 1 && *nonce) || (lcs == 1 && *cs)) {
    ac = _malloc(sizeof(account_t));
    memcpy(ac->address, adr, 20);

    ac->code.data = NULL;
    ac->code.len  = bytes_to_long(cs, lcs);

    if (ac->code.len)
      evm->env(evm, EVM_ENV_CODE_COPY, adr, 20, &ac->code.data, 0, 0);
    ac->storage   = NULL;
    ac->next      = evm->accounts;
    evm->accounts = ac;

    if (l >= 0) {
      if (l < 32) memset(ac->balance, 0, 32 - l);
      memcpy(ac->balance + 32 - l, data, l);
    } else
      memset(ac->balance, 0, 32);

    if (lnonce >= 0) {
      if (lnonce < 32) memset(ac->nonce, 0, 32 - lnonce);
      memcpy(ac->nonce + 32 - lnonce, data, lnonce);
    } else
      memset(ac->nonce, 0, 32);
  }
  return ac;
}

storage_t* evm_get_storage(evm_t* evm, uint8_t* adr, uint8_t* key, int keylen, uint8_t create) {
  if (!adr) return NULL;
  account_t* ac = evm_get_account(evm, adr, create);
  if (!ac) return NULL;
  storage_t* s = ac->storage;
  uint8_t    k[32];
  uint256_set(key, keylen, k);
  while (s) {
    if (memcmp(s->key, k, 32) == 0) return s;
    s = s->next;
  }
  uint8_t* data;
  int      l = evm->env(evm, EVM_ENV_STORAGE, key, keylen, &data, 0, 0);
  if (create || l >= 0) {
    s = _malloc(sizeof(storage_t));
    memcpy(s->key, k, 32);
    s->next     = ac->storage;
    ac->storage = s;

    if (l >= 0) {
      if (l < 32) memset(s->value, 0, 32 - l);
      memcpy(s->value + 32 - l, data, l);
    } else
      memset(s->value, 0, 32);
  }
  return s;
}

void uint256_set(uint8_t* src, int src_len, uint8_t* dst) {
  if (src_len < 32) memset(dst, 0, 32 - src_len);
  memcpy(dst + 32 - src_len, src, src_len);
}
int transfer_value(evm_t* evm, uint8_t* from_account, uint8_t* to_account, uint8_t* value, int value_len, uint32_t base_gas) {
  if (big_is_zero(value, value_len)) return 0;
  account_t* ac_from = evm_get_account(evm, from_account, true);
  account_t* ac_to   = evm_get_account(evm, to_account, false);
#ifdef EVM_GAS
  if (!ac_to) {
    subgas(G_NEWACCOUNT);
    ac_to = evm_get_account(evm, to_account, true);
  }
  subgas(base_gas);
#endif
  uint8_t tmp[32];
  if (ac_from) {
    if (big_cmp(ac_from->balance, 32, value, value_len) < 0) return EVM_ERROR_BALANCE_TOO_LOW;
    uint256_set(tmp, big_sub(ac_from->balance, 32, value, value_len, tmp), ac_from->balance);
  }
  uint256_set(tmp, big_add(ac_from->balance, 32, value, value_len, tmp, 32), ac_to->balance);

  return 0;
}
#endif

int evm_prepare_evm(evm_t*      evm,
                    uint8_t*    address,
                    uint8_t*    account,
                    uint8_t*    origin,
                    uint8_t*    caller,
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
  if (l < 0) return l;
  evm->code.len = (uint32_t) bytes_to_long(tmp, l);

  if ((l = env(evm, EVM_ENV_CODE_COPY, account, 20, &tmp, 0, 0)) < 0) return l;
  evm->code.data = tmp;
  return 0;
}

int evm_sub_call(evm_t*   parent,
                 uint8_t* address,
                 uint8_t* code_address,
                 uint8_t* value, int l_value,
                 uint8_t* data, int l_data,
                 uint8_t* caller,
                 uint8_t* origin,
                 uint64_t gas,
                 uint8_t  mode,
                 int out_offset, int out_len

) {
  evm_t evm;
  int   res;
  res                = evm_prepare_evm(&evm, address, code_address, origin, caller, parent->env, parent->env_ptr);
  evm.call_data.data = data;
  evm.call_data.len  = l_data;
  evm.properties     = parent->properties;

#ifdef EVM_GAS
  evm.gas = gas;
  if (parent->gas < gas)
    res = EVM_ERROR_OUT_OF_GAS;
  else
    parent->gas -= gas;
  evm.root = parent->root;

  if (res == 0) res = transfer_value(&evm, parent->account, address, value, l_value, G_CALLVALUE);

  for (uint32_t i = 0; i < evm.call_data.len; i++)
    evm.gas -= evm.call_data.data[i] ? G_TXDATA_NONZERO : G_TXDATA_ZERO;

#else
  UNUSED_VAR(value);
  UNUSED_VAR(gas);
  UNUSED_VAR(l_value);
#endif
  if (mode == EVM_CALL_MODE_STATIC && l_value > 1) res = EVM_ERROR_UNSUPPORTED_CALL_OPCODE;

  if (res == 0) {
    res = evm_run(&evm);
    evm_stack_push_int(parent, res == 0 ? 1 : 0);
    //    res = 0;
  }
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
  if (res == 0) parent->gas += evm.gas;
#endif

  evm_free(&evm);

  return res;
}

int evm_call(in3_vctx_t* vc,
             uint8_t*    address,
             uint8_t* value, int l_value,
             uint8_t* data, int l_data,
             uint8_t*  caller,
             uint64_t  gas,
             bytes_t** result) {
  evm_t evm;

  int      res     = evm_prepare_evm(&evm, address, address, caller, caller, in3_get_env, vc);
  uint8_t* ccaller = caller;
  int      l       = 20;
  optimize_len(ccaller, l);

#ifdef EVM_GAS
  evm.root = &evm;
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
