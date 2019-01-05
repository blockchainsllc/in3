
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

static int ensure_memory(evm_t* evm, uint32_t max_pos) {
  return max_pos > evm->memory.bsize ? bb_check_size(&evm->memory, max_pos - evm->memory.b.len) : 0;
}

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

static int op_sha3(evm_t* evm) {

  int32_t offset = evm_stack_pop_int(evm), len = evm_stack_pop_int(evm);
  if (offset < 0 || len < 0) return EVM_ERROR_EMPTY_STACK;
  if ((uint32_t)(offset + len) > evm->memory.bsize) return EVM_ERROR_ILLEGAL_MEMORY_ACCESS;
  uint8_t         res[32];
  struct SHA3_CTX ctx;
  sha3_256_Init(&ctx);
  sha3_Update(&ctx, evm->memory.b.data + offset, len);
  keccak_Final(&ctx, res);
  return evm_stack_push(evm, res, 32);
}

static int op_account(evm_t* evm, uint8_t key) {
  uint8_t *address, data[32];
  int      l = evm_stack_pop_ref(evm, &address);
  if (l < 0) return EVM_ERROR_EMPTY_STACK;
  l = evm->env(evm, key, address, l, data, 0, 0);
  return l < 0 ? l : evm_stack_push(evm, data, l);
}
static int op_dataload(evm_t* evm) {
  int pos = evm_stack_pop_int(evm);
  if (pos < 0) return pos;
  if (evm->call_data.len < (uint32_t) pos) return evm_stack_push_int(evm, 0);
  if (evm->call_data.len > (uint32_t) pos + 32)
    return evm_stack_push(evm, evm->call_data.data + pos, 32);
  else {
    uint8_t buffer[32];
    memset(buffer, 0, 32);
    if (evm->call_data.len - pos) {
      memcpy(buffer, evm->call_data.data + pos, evm->call_data.len - pos);
      return evm_stack_push(evm, buffer, 32);
    } else
      return evm_stack_push_int(evm, 0);
  }
}

static int op_datacopy(evm_t* evm, bytes_t* src) {
  int mem_pos = evm_stack_pop_int(evm), data_pos = evm_stack_pop_int(evm), data_len = evm_stack_pop_int(evm);
  if (mem_pos < 0 || data_len < 0 || data_pos < 0) return EVM_ERROR_EMPTY_STACK;
  if (src->len < (uint32_t)(data_pos + data_len)) return 0;
  if (ensure_memory(evm, mem_pos + data_len) < 0) return EVM_ERROR_ILLEGAL_MEMORY_ACCESS;
  memcpy(evm->memory.b.data + mem_pos, src->data + data_pos, data_len);
  return 0;
}

static int op_extcodecopy(evm_t* evm) {
  uint8_t* address;
  int      l = evm_stack_pop_ref(evm, &address), mem_pos = evm_stack_pop_int(evm), code_pos = evm_stack_pop_int(evm), data_len = evm_stack_pop_int(evm);
  if (l < 0 || mem_pos < 0 || data_len < 0 || code_pos < 0) return EVM_ERROR_EMPTY_STACK;
  if (ensure_memory(evm, mem_pos + data_len) < 0) return EVM_ERROR_ILLEGAL_MEMORY_ACCESS;

  // address, memOffset, codeOffset, length
  return evm->env(evm, EVM_ENV_CODE_COPY, address, 20, evm->memory.b.data + mem_pos, code_pos, data_len);
}

static int op_header(evm_t* evm, uint8_t index) {
  bytes_t b;
  if (rlp_decode_in_list(evm->block_header, index, &b) == 1)
    return evm_stack_push(evm, b.data, b.len);
  else
    return evm_stack_push_int(evm, 0);
}

static int op_mload(evm_t* evm) {
  int mem_pos = evm_stack_pop_int(evm);
  if (mem_pos < 0) return EVM_ERROR_EMPTY_STACK;
  if (evm->memory.bsize < (uint32_t) mem_pos + 32) {
    uint8_t data[32];
    memset(data, 0, 32);
    if (evm->memory.bsize > (uint32_t) mem_pos)
      memcpy(data + 32 - evm->memory.bsize + mem_pos, evm->memory.b.data + mem_pos, evm->memory.bsize - mem_pos);
    return evm_stack_push(evm, data, 32);
  }
  return evm_stack_push(evm, evm->memory.b.data + mem_pos, 32);
}

static int op_mstore(evm_t* evm, uint8_t len) {
  int mem_pos = evm_stack_pop_int(evm), l;
  if (mem_pos < 0) return EVM_ERROR_EMPTY_STACK;
  if (ensure_memory(evm, mem_pos + len) < 0) return EVM_ERROR_ILLEGAL_MEMORY_ACCESS;
  uint8_t* data;
  if ((l = evm_stack_pop_ref(evm, &data)) < 0) return EVM_ERROR_EMPTY_STACK;
  if (len == 1)
    evm->memory.b.data[mem_pos] = l ? data[l - 1] : 0;
  else {
    if (l < 32) memset(evm->memory.b.data + mem_pos, 0, 32 - l);
    memcpy(evm->memory.b.data + mem_pos + 32 - l, data, l);
  }
  return 0;
}

static int op_sload(evm_t* evm) {
  uint8_t* data;
  int      l;
  if ((l = evm_stack_pop_ref(evm, &data)) < 0) return l;
  if ((l = evm->env(evm, EVM_ENV_STORAGE, data, l, data, 0, 0)) < 0) return l;
  return evm_stack_push(evm, data, l);
}

static int op_jump(evm_t* evm, uint8_t cond) {
  int pos = evm_stack_pop_int(evm);
  if (pos < 0) return pos;
  if ((uint32_t) pos > evm->code.len || evm->code.data[pos] != 0x5B) return EVM_ERROR_INVALID_JUMPDEST;
  if (cond) {
    uint8_t c;
    int     ret = evm_stack_pop_byte(evm, &c);
    if (ret == EVM_ERROR_EMPTY_STACK) return EVM_ERROR_EMPTY_STACK;
    if (!c && ret >= 0) return 0; // the condition was false
  }
  evm->pos = pos + 1;
  return 0;
}

static int op_push(evm_t* evm, uint8_t len) {
  if (evm->code.len < (uint32_t) evm->pos + len) return EVM_ERROR_INVALID_PUSH;
  if (evm_stack_push(evm, evm->code.data + evm->pos, len) < 0)
    return EVM_ERROR_BUFFER_TOO_SMALL;
  evm->pos += len;
  return 0;
}

static int op_dup(evm_t* evm, uint8_t pos) {
  uint8_t* data;
  int      l = evm_stack_get_ref(evm, pos, &data);
  if (l < 0) return l;
  return evm_stack_push(evm, data, l);
}

static int op_swap(evm_t* evm, uint8_t pos) {
  uint8_t data[33], *a, *b;
  int     l1 = evm_stack_get_ref(evm, 1, &a);
  if (l1 < 0) return l1;
  int l2 = evm_stack_get_ref(evm, pos, &b);
  if (l2 < 0) return l2;
  if (l1 == l2) {
    memcpy(data, a, l1);
    memcpy(a, b, l1);
    memcpy(b, data, l1);
  } else if (l2 > l1) {
    memcpy(data, b, l2 + 1); // keep old b + len
    memcpy(b, a, l1 + 1);
    if (pos > 2) memmove(b + l1 + 1, b + l2 + 1, a - b - l2 - 1);
    memcpy(a + l1 - l2, data, l2 + 1);
  } else {
    memcpy(data, a, l1 + 1); // keep old b + len
    memcpy(a, b, l2 + 1);
    if (pos > 2) memmove(b + l1 + 1, b + l2 + 1, a - b - l2 - 1);
    memcpy(b, data, l1 + 1);
  }
  return 0;
}

int evm_execute(evm_t* evm) {
  uint8_t op = evm->code.data[evm->pos++];
  if (op >= 0x60 && op <= 0x7F) // PUSH
    return op_push(evm, op - 0x5F);
  if (op >= 0x80 && op <= 0x8F) // DUP
    return op_dup(evm, op - 0x7F);
  if (op >= 0x90 && op <= 0x9F) // SWAP
    return op_swap(evm, op - 0x8F);
  if (op >= 0xA0 && op <= 0xA4) // LOG --> for now, we don't support logs
    return EVM_ERROR_UNSUPPORTED_CALL_OPCODE;

  switch (op) {
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
    case 0x20: // SHA3
      return op_sha3(evm);
    case 0x30: // ADDRESS
      return evm_stack_push(evm, evm->address, 20);
    case 0x31: // BALANCE
      return op_account(evm, EVM_ENV_BALANCE);
    case 0x32: // ORIGIN
      return evm_stack_push(evm, evm->origin, 20);
    case 0x33: // CALLER
      return evm_stack_push(evm, evm->caller, 20);
    case 0x34: // CALLVALUE
      return evm_stack_push(evm, evm->call_value.data, evm->call_value.len);
    case 0x35: // CALLDATALOAD
      return op_dataload(evm);
    case 0x36: // CALLDATA_SIZE
      return evm_stack_push_int(evm, evm->call_value.len);
    case 0x37: // CALLDATACOPY
      return op_datacopy(evm, &evm->call_data);
    case 0x38: // CODESIZE
      return evm_stack_push_int(evm, evm->code.len);
    case 0x39: // CODESIZE
      return op_datacopy(evm, &evm->code);
    case 0x3a: // GASPRICE
      return evm_stack_push(evm, evm->gas_price.data, evm->gas_price.len);
    case 0x3b: // EXTCODESIZE
      return op_account(evm, EVM_ENV_CODE_SIZE);
    case 0x3c: // EXTCODECOPY
      return op_extcodecopy(evm);
    case 0x3d: // RETURNDATASIZE
      return evm_stack_push_int(evm, evm->last_returned.len);
    case 0x3e: // RETURNDATACOPY
      return op_datacopy(evm, &evm->last_returned);
    case 0x40: // BLOCKHASH
      return op_account(evm, EVM_ENV_BLOCKHASH);
    case 0x41: // COINBASE
      return op_header(evm, BLOCKHEADER_MINER);
    case 0x42: // TIMESTAMP
      return op_header(evm, BLOCKHEADER_TIMESTAMP);
    case 0x43: // NUMBER
      return op_header(evm, BLOCKHEADER_NUMBER);
    case 0x44: // DIFFICULTY
      return op_header(evm, BLOCKHEADER_DIFFICULTY);
    case 0x45: // GASLIMIT
      return op_header(evm, BLOCKHEADER_GAS_LIMIT);

    case 0x50: // POP
      return evm_stack_pop(evm, NULL, 0);
    case 0x51: // MLOAD
      return op_mload(evm);
    case 0x52: // MSTORE
      return op_mstore(evm, 32);
    case 0x53: // MSTORE8
      return op_mstore(evm, 1);
    case 0x54: // SLOAD
      return op_sload(evm);
    case 0x55: // SSTORE   -->   for eth_call we do not support storing storage yet!
      return EVM_ERROR_UNSUPPORTED_CALL_OPCODE;
    case 0x56: // JUMP
      return op_jump(evm, 0);
    case 0x57: // JUMPI
      return op_jump(evm, 1);
    case 0x58: // PC
      return evm_stack_push_int(evm, evm->pos - 1);
    case 0x59: // MSIZE
      return evm_stack_push_int(evm, evm->memory.bsize);
    case 0x5a: // GAS     --> here we always return enough gas to keep going, since eth call should not use it anyway
      return evm_stack_push_int(evm, 0xFFFFFFF);
    case 0x5b: // JUMPDEST
      return 0;
    case 0xF0: // CREATE   -> we don't support it for call
      return EVM_ERROR_UNSUPPORTED_CALL_OPCODE;
    case 0xF1: // CALL
      return 0;

    default:
      return EVM_ERROR_INVALID_OPCODE;
  }
}