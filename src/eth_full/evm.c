
#include "../eth_basic/eth_basic.h"
#include "../eth_nano/eth_nano.h"
#include "../eth_nano/merkle.h"
#include "../eth_nano/rlp.h"
#include "../eth_nano/serialize.h"
#include "big.h"
#include "eth_full.h"
#include <client/context.h>
#include <crypto/bignum.h>
#include <crypto/ecdsa.h>
#include <crypto/secp256k1.h>
#include <string.h>
#include <util/data.h>
#include <util/mem.h>
#include <util/utils.h>

#include "evm.h"

int evm_stack_push(evm_t* evm, uint8_t* data, uint8_t len) {
  if (bb_check_size(&evm->stack, len + 1)) return EVM_ERROR_EMPTY_STACK;
  uint8_t* buffer = evm->stack.b.data + evm->stack.b.len;
  memcpy(data, buffer, len);
  evm->stack.b.len += len + 1;
  buffer[evm->stack.b.len - 1] = len;
  evm->stack_size++;
  return 0;
}

int evm_stack_pop(evm_t* evm, uint8_t* dst, uint8_t len) {
  if (evm->stack_size == 0) return EVM_ERROR_EMPTY_STACK; // stack empty
  uint8_t l = evm->stack.b.data[evm->stack.b.len - 1];
  evm->stack.b.len -= l + 1;
  evm->stack_size--;
  if (l > len) return EVM_ERROR_BUFFER_TOO_SMALL;
  memmove(dst, evm->stack.b.data + evm->stack.b.len, l);
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
  dst = evm->stack.b.data + evm->stack.b.len + l - 1;
  return l;
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
