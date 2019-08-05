#ifndef opcodes_h__
#define opcodes_h__

#include "evm.h"


/*
int evm_ensure_memory(evm_t* evm, uint32_t max_pos) {

#ifdef EVM_GAS
  uint32_t old_l = evm->memory.bsize;
  if (max_pos > evm->memory.b.len) {

    int old_wc = (evm->memory.b.len + 31) / 32;
    int new_wc = (max_pos + 31) / 32;
    if (new_wc > old_wc) {
      int old_cost = old_wc * G_MEMORY + (old_wc * old_wc) / 512;
      int new_cost = new_wc * G_MEMORY + (new_wc * new_wc) / 512;
      if (new_cost > old_cost)
        subgas(new_cost - old_cost);
    }

    new_wc            = bb_check_size(&evm->memory, max_pos - evm->memory.b.len);
    evm->memory.b.len = max_pos;
    if (old_l < evm->memory.bsize)
      memset(evm->memory.b.data + old_l, 0, evm->memory.bsize - old_l);
    return new_wc;
#else
  if (max_pos > evm->memory.b.len) {
    int r             = bb_check_size(&evm->memory, max_pos - evm->memory.b.len);
    evm->memory.b.len = max_pos;
    if (old_l < evm->memory.bsize)
      memset(evm->memory.b.data + old_l, 0, evm->memory.bsize - old_l);
    return r;
#endif
  } else
    return 0;
}
*/



int op_math(evm_t *evm, uint8_t op, uint8_t mod);

int op_signextend(evm_t *evm);

int op_is_zero(evm_t *evm);

int op_not(evm_t *evm);


int op_bit(evm_t *evm, uint8_t op);

int op_byte(evm_t *evm);

int op_cmp(evm_t *evm, int8_t eq, uint8_t sig);

int op_shift(evm_t *evm, uint8_t left);

int op_sha3(evm_t *evm);

int op_account(evm_t *evm, uint8_t key);

int op_dataload(evm_t *evm);

int op_datacopy(evm_t *evm, bytes_t *src, uint_fast8_t check_size);

int op_extcodecopy(evm_t *evm);

int op_header(evm_t *evm, uint8_t index);

int op_mload(evm_t *evm);

int op_mstore(evm_t *evm, uint8_t len);

int op_sload(evm_t *evm);

int op_sstore(evm_t *evm);

int op_jump(evm_t *evm, uint8_t cond);

int op_push(evm_t *evm, wlen_t len);

int op_dup(evm_t *evm, uint8_t pos);

int op_swap(evm_t *evm, uint8_t pos);

int op_log(evm_t *evm, uint8_t len);

int op_return(evm_t *evm, uint8_t revert);

int op_selfdestruct(evm_t *evm);


int op_create(evm_t *evm, uint_fast8_t use_salt);

int op_call(evm_t *evm, uint8_t mode);

#endif