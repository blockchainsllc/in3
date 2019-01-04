
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

static int op_add(evm_t* evm) {
  bignum256 a, b;
  if (evm_stack_pop_bn(evm, &a)) return EVM_ERROR_EMPTY_STACK;
  if (evm_stack_pop_bn(evm, &b)) return EVM_ERROR_EMPTY_STACK;
  bn_add(&a, &b);
  return evm_stack_push_bn(evm, &a);
}
static int op_mul(evm_t* evm) {
  bignum256 a, b;
  if (evm_stack_pop_bn(evm, &a)) return EVM_ERROR_EMPTY_STACK;
  if (evm_stack_pop_bn(evm, &b)) return EVM_ERROR_EMPTY_STACK;
  //  bn_multiply(&a, &b,2);
  return evm_stack_push_bn(evm, &a);
}

static int op_is_zero(evm_t* evm) {
  uint8_t res = 1, *a;
  int     l   = evm_stack_pop_ref(evm, &a), i;
  if (l < 0) return l;
  for (i = 0; i < l; i++) {
    if (a[i]) {
      res = 0;
      break;
    }
  }
  // push result to stack
  return evm_stack_push(evm, &res, 1);
}

static int op_not(evm_t* evm) {
  uint8_t res[32], *a;
  int     l = evm_stack_pop_ref(evm, &a), i;
  if (l < 0) return l;

  for (i = 0; i < l; i++)
    res[i + 32 - l] = a[i] ^ 0xFF;
  if (l < 32) memset(res, 0, 32 - l);

  // push result to stack
  return evm_stack_push(evm, res, 32);
}

#define OP_AND 0
#define OP_OR 1
#define OP_XOR 2

static int op_bit(evm_t* evm, uint8_t op) {
  uint8_t result[32], *res = result, *a, l, l1 = evm_stack_pop_ref(evm, &a), i, j;
  if (l1 < 0) return l1;
  memcpy(res + 32 - l1, a, l1);
  if (l1 < 32) memset(res, 0, 32 - l1);
  if ((l = evm_stack_pop_ref(evm, &a)) < 0) return EVM_ERROR_EMPTY_STACK;
  l1 = l1 > l ? l1 : l;
  res += 32 - l1;

  switch (op) {
    case OP_AND:
      for (i = 0, j = l1 - l; i < l; i++) res[j++] &= a[i];
      if (l < l1) memset(res, 0, l1 - l);
      break;
    case OP_OR:
      for (i = 0, j = l1 - l; i < l; i++) res[j++] |= a[i];
      break;
    case OP_XOR:
      for (i = 0, j = l1 - l; i < l; i++) res[j++] ^= a[i];
      break;
    default:
      return -1;
  }

  // push result to stack
  return evm_stack_push(evm, res, l1);
}

static int op_byte(evm_t* evm) {

  uint8_t pos, *b, res = 0xFF;
  int     l = evm_stack_pop_byte(evm, &pos);
  if (l == EVM_ERROR_EMPTY_STACK) return l;
  if (l < 0 || (pos & 0xE0)) res = 0;
  if ((l = evm_stack_pop_ref(evm, &b)) < 0) return EVM_ERROR_EMPTY_STACK;
  if (res) res = pos >= (32 - l) ? b[pos + l - 32] : 0;
  return evm_stack_push(evm, &res, 1);
}

static int op_cmp(evm_t* evm, int8_t eq, uint8_t sig) {
  uint8_t *a, *b, res = 0, sig_a, sig_b;
  int      len_a, len_b;
  // try fetch the 2 values from the stack
  if ((len_a = evm_stack_pop_ref(evm, &a)) < 0 || (len_b = evm_stack_pop_ref(evm, &b)) < 0) return EVM_ERROR_EMPTY_STACK;

  // if it is signed,. we store the sign and convert it to unsigned.
  if (sig) {
    sig_a = big_signed(a, len_a, a);
    sig_b = big_signed(b, len_b, b);
  }

  // compare the value
  switch (eq) {
    case -1:
      res = big_cmp(a, len_a, b, len_b) < 0;
      break;
    case 0:
      res = big_cmp(a, len_a, b, len_b) == 0;
      break;
    case 1:
      res = big_cmp(a, len_a, b, len_b) > 0;
      break;
  }

  // if it is signed we check check the sign and change the result if the sign changes.
  if (sig && eq) {
    if (sig_a && sig_b)
      res ^= 1;
    else if (sig_a || sig_b)
      res = eq < 0 ? sig_a : sig_b;
  }

  // push result to stack
  return evm_stack_push(evm, &res, 1);
}

int op_shift(evm_t* evm, uint8_t left) {
  uint8_t pos, *b, res[32];
  int     l;
  if ((evm->properties & EVM_EIP_CONSTANTINOPL) == 0) return EVM_ERROR_INVALID_OPCODE;
  l = evm_stack_pop_byte(evm, &pos);
  if (l == EVM_ERROR_EMPTY_STACK) return l;
  if (l < 0) { // the number is out of range
    if ((l = evm_stack_pop_ref(evm, &b)) < 0) return EVM_ERROR_EMPTY_STACK;
    if (left == 2 && l == 32 && (*b & 128)) { //signed number return max NUMBER as fault
      memset(res, 0xFF, 32);
      return evm_stack_push(evm, res, 32);
    } else {
      res[0] = 0;
      return evm_stack_push(evm, res, 1);
    }
  }

  if ((l = evm_stack_pop_ref(evm, &b)) < 0) return EVM_ERROR_EMPTY_STACK;
  memmove(res + 32 - l, b, l);
  if (l < 32) memset(res, 0, 32 - l);
  if (left == 1)
    big_shift_left(res, 32, pos);
  else if (left == 0)
    big_shift_right(res, 32, pos);
  else if (left == 2) { // signed shift right
    big_shift_right(res, 32, pos);
    if (l == 32 && (*b & 128)) { // the original number was signed
      for (l = 0; l<pos>> 3; l++) res[l] = 0xFF;
      l = 8 - (pos % 8);
      res[pos >> 3] |= (0XFF >> l) << l;
      return evm_stack_push(evm, res, 32);
    }
  }
  // optimize length
  for (pos = 32; pos > 0; pos--) {
    if (res[32 - pos]) break;
  }
  return evm_stack_push(evm, res + 32 - pos, pos);
}

int evm_execute(evm_t* evm, int pos) {

  switch (evm->code[pos++]) {
    case 0x00: // STOP
      evm->state = EVM_STATE_STOPPED;
      return 0;

    case 0x01: //  ADD
      return op_add(evm);
    case 0x02: //  MUL
      return op_mul(evm);
    case 0x10: // LT
      return op_cmp(evm, -1, 0);
    case 0x11: // GT
      return op_cmp(evm, 1, 0);
    case 0x12: // SLT
      return op_cmp(evm, -1, 1);
    case 0x13: // SGT
      return op_cmp(evm, 1, 1);
    case 0x14: // EQ
      return op_cmp(evm, 0, 0);
    case 0x15: // IS_ZERO
      return op_is_zero(evm);
    case 0x16: // AND
      return op_bit(evm, OP_AND);
    case 0x17: // OR
      return op_bit(evm, OP_OR);
    case 0x18: // XOR
      return op_bit(evm, OP_XOR);
    case 0x19: // NOT
      return op_not(evm);
    case 0x1a: // BYTE
      return op_byte(evm);
    case 0x1b: // SHL
      return op_shift(evm, 1);
    case 0x1c: // SHR
      return op_shift(evm, 0);
    case 0x1d: // SAR
      return op_shift(evm, 2);
    default:
      return EVM_ERROR_INVALID_OPCODE;
  }
}