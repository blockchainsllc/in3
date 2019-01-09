#include "evm.h"
#include <stdlib.h>
#include <string.h>
#include <util/utils.h>

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

  // get the code
  uint8_t* tmp;
  int      l = env(evm, EVM_ENV_CODE_SIZE, account, 20, &tmp, 0, 32);
  if (l < 0) return l;
  evm->code.len = (uint32_t) bytes_to_long(tmp, l);

  if ((l = env(evm, EVM_ENV_CODE_COPY, account, 20, &tmp, 0, 0)) < 0) return l;
  evm->code.data = tmp;
}

int evm_sub_call(evm_t*   parent,
                 uint8_t* gas_limit, int l_gas,
                 uint8_t* address,
                 uint8_t* code_address,
                 uint8_t* value, int l_value,
                 uint8_t* data, int l_data,
                 uint8_t* caller,
                 uint8_t* origin,
                 uint8_t  mode,
                 int out_offset, int out_len

) {
  evm_t evm;
  int   res;
  res = evm_prepare_evm(&evm, address, code_address, origin, caller, parent->env, parent->env_ptr);
  if (res == 0) res = evm_run(&evm);
  if (res == 0 && evm.return_data.data && out_offset && out_len) {
    res = evm_ensure_memory(parent, out_offset + out_len);
    if (res == 0) memcpy(parent->memory.b.data + out_offset, evm.return_data.data, out_len);
  }

  if (evm.return_data.data) _free(evm.return_data.data);
  if (evm.stack.b.data) _free(evm.stack.b.data);
  if (evm.memory.b.data) _free(evm.memory.b.data);

  return res;
}