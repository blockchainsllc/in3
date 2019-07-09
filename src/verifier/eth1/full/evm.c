#include "evm.h"
#include "../basic/eth_basic.h"
#include "../nano/eth_nano.h"
#include "../nano/merkle.h"
#include "../nano/rlp.h"
#include "../nano/serialize.h"
#include "big.h"
#include "eth_full.h"
#include "gas.h"
#include <client/context.h>
#include <crypto/bignum.h>
#include <crypto/ecdsa.h>
#include <crypto/secp256k1.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/data.h>
#include <util/mem.h>
#include <util/utils.h>

int evm_stack_push(evm_t* evm, uint8_t* data, uint8_t len) {
  if (evm->stack_size == EVM_STACK_LIMIT || len > 32) return EVM_ERROR_STACK_LIMIT;
  if (&evm->stack.b.len + len > &evm->stack.bsize) {
    // we need to make sure the data ref is not part of the stack and would be ionvalidated now
    uint32_t tmp[32];
    memcpy(tmp, data, len);
    if (bb_check_size(&evm->stack, len + 1)) return EVM_ERROR_EMPTY_STACK;
    memcpy(evm->stack.b.data + evm->stack.b.len, tmp, len);
  } else
    memcpy(evm->stack.b.data + evm->stack.b.len, data, len);

  evm->stack.b.len += len + 1;
  evm->stack.b.data[evm->stack.b.len - 1] = len;
  evm->stack_size++;
  return 0;
}

int evm_stack_push_ref(evm_t* evm, uint8_t** dst, uint8_t len) {
  if (evm->stack_size == EVM_STACK_LIMIT) return EVM_ERROR_STACK_LIMIT;
  if (bb_check_size(&evm->stack, len + 1)) return EVM_ERROR_EMPTY_STACK;
  *dst = evm->stack.b.data + evm->stack.b.len;
  evm->stack.b.len += len + 1;
  evm->stack.b.data[evm->stack.b.len - 1] = len;
  evm->stack_size++;
  return 0;
}
int evm_stack_push_int(evm_t* evm, uint32_t val) {
  uint8_t bytes[4];
  bytes[3] = val & 0xFF;
  bytes[2] = (val >> 8) & 0xFF;
  bytes[1] = (val >> 16) & 0xFF;
  bytes[0] = (val >> 24) & 0xFF;
  if (bytes[0]) return evm_stack_push(evm, bytes, 4);
  if (bytes[1]) return evm_stack_push(evm, bytes + 1, 3);
  if (bytes[2]) return evm_stack_push(evm, bytes + 2, 2);
  return evm_stack_push(evm, bytes + 3, 1);
}

int evm_stack_push_long(evm_t* evm, uint64_t val) {
  uint8_t bytes[8];
  bytes[7] = val & 0xFF;
  bytes[6] = (val >> 8) & 0xFF;
  bytes[5] = (val >> 16) & 0xFF;
  bytes[4] = (val >> 24) & 0xFF;
  bytes[3] = (val >> 32) & 0xFF;
  bytes[2] = (val >> 40) & 0xFF;
  bytes[1] = (val >> 48) & 0xFF;
  bytes[0] = (val >> 56) & 0xFF;
  for (uint8_t i = 0; i < 7; i++) {
    if (bytes[i] == 0) return evm_stack_push(evm, bytes + i, 8 - i);
  }
  return evm_stack_push(evm, bytes + 7, 1);
}

int evm_stack_pop(evm_t* evm, uint8_t* dst, uint8_t len) {
  if (evm->stack_size == 0) return EVM_ERROR_EMPTY_STACK; // stack empty
  uint8_t l = evm->stack.b.data[evm->stack.b.len - 1];
  evm->stack.b.len -= l + 1;
  evm->stack_size--;
  if (!dst) return l;
  if (l == len)
    memcpy(dst, evm->stack.b.data + evm->stack.b.len, l);
  else if (l < len) {
    memset(dst, 0, len - l);
    memcpy(dst + len - l, evm->stack.b.data + evm->stack.b.len, l);
  } else
    memcpy(dst, evm->stack.b.data + evm->stack.b.len + l - len, len);
  return l;
}

int evm_stack_pop_ref(evm_t* evm, uint8_t** dst) {
  if (evm->stack_size == 0) return EVM_ERROR_EMPTY_STACK; // stack empty
  uint8_t l = evm->stack.b.data[evm->stack.b.len - 1];
  evm->stack.b.len -= l + 1;
  evm->stack_size--;
  *dst = evm->stack.b.data + evm->stack.b.len;
  return l;
}

int evm_stack_get_ref(evm_t* evm, uint8_t pos, uint8_t** dst) {
  if (evm->stack_size - pos < 0 || pos < 1) return EVM_ERROR_EMPTY_STACK; // stack empty
  uint32_t p = evm->stack.b.len;
  uint8_t  i, l;
  for (i = 0; i < pos; i++) {
    l = evm->stack.b.data[p - 1];
    p -= l + 1;
  }
  *dst = evm->stack.b.data + p;
  return l;
}

int evm_stack_peek_ref(evm_t* evm, uint8_t** dst) {
  if (evm->stack_size == 0) return EVM_ERROR_EMPTY_STACK; // stack empty
  uint8_t l = evm->stack.b.data[evm->stack.b.len - 1];
  evm->stack.b.len -= l + 1;
  evm->stack_size--;
  *dst = evm->stack.b.data + evm->stack.b.len;
  return l;
}
int evm_stack_pop_byte(evm_t* evm, uint8_t* dst) {
  if (evm->stack_size == 0) return EVM_ERROR_EMPTY_STACK; // stack empty
  uint8_t l = evm->stack.b.data[evm->stack.b.len - 1];
  evm->stack.b.len -= l + 1;
  evm->stack_size--;
  if (l > 1) {
    for (uint32_t i = evm->stack.b.len; i < evm->stack.b.len + l - 1; i++) {
      if (evm->stack.b.data[i]) return -3;
    }
  } else if (l == 0)
    return -3;
  *dst = evm->stack.b.data[evm->stack.b.len + l - 1];
  return l;
}
int evm_stack_peek_len(evm_t* evm) {
  if (evm->stack_size == 0) return EVM_ERROR_EMPTY_STACK;
  uint8_t l = evm->stack.b.data[evm->stack.b.len - 1], *p = evm->stack.b.data + evm->stack.b.len - l - 1;
  optimize_len(p, l);
  return l;
}

int32_t evm_stack_pop_int(evm_t* evm) {
  if (evm->stack_size == 0) return EVM_ERROR_EMPTY_STACK; // stack empty
  uint8_t l = evm->stack.b.data[evm->stack.b.len - 1], *p = evm->stack.b.data + evm->stack.b.len - l - 1;
  evm->stack.b.len -= l + 1;
  evm->stack_size--;
  optimize_len(p, l);
  return (l > 4 || (l == 4 && *p & 0xF0)) ? 0xFFFFFFF : bytes_to_int(p, l);
}

int evm_stack_pop_bn(evm_t* evm, bignum256* dst) {
  if (evm->stack_size == 0) return EVM_ERROR_EMPTY_STACK; // stack empty
  uint8_t l = evm->stack.b.data[evm->stack.b.len - 1];
  evm->stack.b.len -= l + 1;
  evm->stack_size--;
  if (l == 32)
    bn_read_be(evm->stack.b.data + evm->stack.b.len, dst);
  else {
    uint8_t t[32];
    memmove(t + 32 - l, evm->stack.b.data + evm->stack.b.len, l);
    memset(t, 0, 32 - l);
    bn_read_be(t, dst);
  }
  return l;
}

int evm_stack_push_bn(evm_t* evm, bignum256* val) {
  if (bb_check_size(&evm->stack, 33)) return EVM_ERROR_EMPTY_STACK;
  uint8_t* buffer = evm->stack.b.data + evm->stack.b.len;
  bn_write_be(val, buffer);
  evm->stack.b.len += 33;
  buffer[evm->stack.b.len - 1] = 32;
  evm->stack_size++;
  return 0;
}
/*
I:79338654 267     3 63 : PUSH4      [ 364087e | 1 | 945304eb96065b2a98b57a48a06ae28d285a71b5 | ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff | ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff | ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff | 
P:79338654 267     3 63 : PUSH4      [ 364087e | 1 | 945304eb96065b2a98b57a48a06ae28d285a71b5 | ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff | ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff | ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff |

*/
#define __code(n)                      \
  {                                    \
    printf("\x1B[32m%-10s\x1B[0m", n); \
    return;                            \
  }
void evm_print_op(evm_t* evm, uint64_t last_gas, uint32_t pos) {
  uint8_t op = evm->code.data[pos];
#ifdef EVM_GAS
  printf("\n::: ");
  evm_t* pp = evm->parent;
  while (pp) {
    printf(" .. ");
    pp = pp->parent;
  }

  if (last_gas > evm->gas)
    printf("%08" PRIx64 " %03i \x1B[33m%5" PRIu64 "\x1B[0m %02x : ", evm->gas, pos, last_gas - evm->gas, op);
  else
    printf("%08" PRIx64 " %03i \x1B[33m+%5" PRIu64 "\x1B[0m %02x : ", evm->gas, pos, evm->gas - last_gas, op);
#else
  UNUSED_VAR(last_gas);
  printf("\n%03i       %02x : ", pos, op);
#endif
  if (op >= 0x60 && op <= 0x7F) {
    printf("\x1B[32mPUSH%i\x1B[0m    %s", op - 0x5F, (op - 0x05F) < 10 ? " " : "");
    //    for (int j = 0; j < op - 0x5F; j++) printf("%02x", evm->code.data[evm->pos + j + 1]);
    return;
  }
  if (op >= 0x80 && op <= 0x8F) {
    printf("\x1B[32mDUP%i\x1B[0m     %s", op - 0x7F, (op - 0x7F) < 10 ? " " : "");
    return;
  }
  if (op >= 0x90 && op <= 0x9F) {
    printf("\x1B[32mSWAP%i\x1B[0m    %s", op - 0x8F, (op - 0x8F) < 10 ? " " : "");
    return;
  }
  if (op >= 0xA0 && op <= 0xA4) {
    printf("\x1B[32mLOG%i\x1B[0m      ", op - 0xA0);
    return;
  }

  switch (op) {
    case 0x00: __code("STOP");
    case 0x01: __code("ADD");
    case 0x02: __code("MUL");
    case 0x03: __code("SUB");
    case 0x04: __code("DIV");
    case 0x05: __code("SDIV");
    case 0x06: __code("MOD");
    case 0x07: __code("SMOD");
    case 0x08: __code("ADDMOD");
    case 0x09: __code("MULMOD");
    case 0x0A: __code("EXP");
    case 0x0B: __code("SIGNEXTEND");
    case 0x10: __code("LT");
    case 0x11: __code("GT");
    case 0x12: __code("SLT");
    case 0x13: __code("SGT");
    case 0x14: __code("EQ");
    case 0x15: __code("IS_ZERO");
    case 0x16: __code("AND");
    case 0x17: __code("OR");
    case 0x18: __code("XOR");
    case 0x19: __code("NOT");
    case 0x1a: __code("BYTE");
    case 0x1b: __code("SHL");
    case 0x1c: __code("SHR");
    case 0x1d: __code("SAR");
    case 0x20: __code("SHA3");
    case 0x30: __code("ADDRESS");
    case 0x31: __code("BALANCE");
    case 0x32: __code("ORIGIN");
    case 0x33: __code("CALLER");
    case 0x34: __code("CALLVALUE");
    case 0x35: __code("CALLDATALOAD");
    case 0x36: __code("CALLDATA_SIZE");
    case 0x37: __code("CALLDATACOPY");
    case 0x38: __code("CODESIZE");
    case 0x39: __code("CODECOPY");
    case 0x3a: __code("GASPRICE");
    case 0x3b: __code("EXTCODESIZE");
    case 0x3c: __code("EXTCODECOPY");
    case 0x3d: __code("RETURNDATASIZE");
    case 0x3e: __code("RETURNDATACOPY");
    case 0x3f: __code("EXTCODEHASH");
    case 0x40: __code("BLOCKHASH");
    case 0x41: __code("COINBASE");
    case 0x42: __code("TIMESTAMP");
    case 0x43: __code("NUMBER");
    case 0x44: __code("DIFFICULTY");
    case 0x45: __code("GASLIMIT");
    case 0x50: __code("POP");
    case 0x51: __code("MLOAD");
    case 0x52: __code("MSTORE");
    case 0x53: __code("MSTORE8");
    case 0x54: __code("SLOAD");
    case 0x55: __code("SSTORE");
    case 0x56: __code("JUMP");
    case 0x57: __code("JUMPI");
    case 0x58: __code("PC");
    case 0x59: __code("MSIZE");
    case 0x5a: __code("GAS");
    case 0x5b: __code("JUMPDEST");
    case 0xF0: __code("CREATE");
    case 0xF1: __code("CALL");
    case 0xF2: __code("CALLCODE");
    case 0xF3: __code("RETURN");
    case 0xF4: __code("DELEGATE_CALL");
    case 0xFA: __code("STATIC_CALL");
    case 0xFD: __code("REVERT");
    case 0xFE: __code("INVALID OPCODE");
    case 0xFF: __code("SELFDESTRUCT");
  }
}
void evm_print_stack(evm_t* evm, uint64_t last_gas, uint32_t pos) {

  evm_print_op(evm, last_gas, pos);
  printf(" [ ");
  for (int i = 0; i < evm->stack_size; i++) {
    uint8_t* dst = NULL;
    int      l = evm_stack_get_ref(evm, i + 1, &dst);
    optimize_len(dst, l);
    for (int j = 0; j < l; j++) {
      if (j == 0 && dst[j] < 16)
        printf("%x", dst[j]);
      else
        printf("%02x", dst[j]);
    }
    printf(" | ");
  }
}
