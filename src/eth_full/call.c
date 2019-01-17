#include "big.h"
#include "evm.h"
#include "gas.h"
#include <client/verifier.h>
#include <stdlib.h>
#include <string.h>
#include <util/utils.h>

void evm_free(evm_t* evm) {
  if (evm->return_data.data) _free(evm->return_data.data);
  if (evm->stack.b.data) _free(evm->stack.b.data);
  if (evm->memory.b.data) _free(evm->memory.b.data);
  printf("#  FREE EVM 1 \n");

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
  printf("#  FREE EVM 2 \n");

  while (evm->accounts) {
    ac = evm->accounts;
    if (ac->code.data) _free(ac->code.data);
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
  printf("#  FREE EVM 3 \n");
}

#ifdef EVM_GAS
account_t* evm_get_account(evm_t* evm, uint8_t* adr, uint8_t create) {
  if (!adr) return NULL;
  account_t* ac = evm->root->accounts;
  while (ac) {
    if (memcmp(ac->address, adr, 20) == 0) return ac;
    ac = ac->next;
  }
  if (create) {
    ac = _malloc(sizeof(account_t));
    memcpy(ac->address, adr, 20);
    ac->code.data = NULL;
    ac->code.len  = 0;
    ac->storage   = NULL;
    ac->next      = evm->accounts;
    evm->accounts = ac;

    uint8_t* data;
    int      l = evm->env(evm, EVM_ENV_BALANCE, adr, 20, &data, 0, 0);
    if (l >= 0) {
      if (l < 32) memset(ac->balance, 0, 32 - l);
      memcpy(ac->balance + 32 - l, data, l);
    } else
      memset(ac->balance, 0, 32);
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
  if (create) {
    s = _malloc(sizeof(storage_t));
    memcpy(s->key, k, 32);
    s->next     = ac->storage;
    ac->storage = s;

    uint8_t* data;
    int      l = evm->env(evm, EVM_ENV_STORAGE, key, keylen, &data, 0, 0);
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
int transfer_value(evm_t* evm, uint8_t* from_account, uint8_t* to_account, uint8_t* value, int value_len) {
  account_t* ac_from = evm_get_account(evm, from_account, true);
  account_t* ac_to   = evm_get_account(evm, to_account, true);
  uint8_t    tmp[32];
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

  evm->memory.b.data = _malloc(32);
  evm->memory.b.len  = 0;
  evm->memory.bsize  = 32;

  evm->stack_size = 0;

  evm->pos   = 0;
  evm->state = EVM_STATE_INIT;

  evm->last_returned.data = NULL;
  evm->last_returned.len  = 0;

  evm->properties = EVM_EIP_CONSTANTINOPL;

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
  evm->root     = &evm;
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
  res = evm_prepare_evm(&evm, address, code_address, origin, caller, parent->env, parent->env_ptr);

#ifdef EVM_GAS
  evm.gas = gas;
  if (parent->gas < gas)
    res = EVM_ERROR_OUT_OF_GAS;
  else
    parent->gas -= gas;
  evm.root = parent->root;
  if (res == 0) res = transfer_value(&evm, parent->account, address, value, l_value);
#endif
  evm.call_data.data = data;
  evm.call_data.len  = l_data;
  if (mode == EVM_CALL_MODE_STATIC && l_value > 1) res = EVM_ERROR_UNSUPPORTED_CALL_OPCODE;

  if (res == 0) res = evm_run(&evm);
  if (res == 0 && evm.return_data.data && out_offset && out_len) {
    res = evm_ensure_memory(parent, out_offset + out_len);
    if (res == 0) memcpy(parent->memory.b.data + out_offset, evm.return_data.data, out_len);
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
  printf("#start EVM_CALL\n");

  int res = evm_prepare_evm(&evm, address, address, caller, caller, in3_get_env, vc);

#ifdef EVM_GAS
  printf("#EVM_GAS\n");
  evm.root = &evm;
  if (res == 0) res = transfer_value(&evm, caller, address, value, l_value);
#endif

  evm.gas            = gas;
  evm.call_data.data = data;
  evm.call_data.len  = l_data;
  printf("#START_RUN %i\n", res);
  if (res == 0) res = evm_run(&evm);
  printf("#END_RUN %i\n", res);
  if (res == 0 && evm.return_data.data)
    *result = b_dup(&evm.return_data);
  evm_free(&evm);
  printf("#EVM_FREE %i\n", res);

  return res;
}
