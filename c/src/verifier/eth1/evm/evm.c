/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 * 
 * 
 * COMMERCIAL LICENSE USAGE
 * 
 * Licensees holding a valid commercial license may use this file in accordance 
 * with the commercial license agreement provided with the Software or, alternatively, 
 * in accordance with the terms contained in a written agreement between you and 
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
 * information please contact slock.it at in3@slock.it.
 * 	
 * Alternatively, this file may be used under the AGPL license as follows:
 *    
 * AGPL LICENSE USAGE
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

#include "evm.h"
#include "../../../core/client/context.h"
#include "../../../core/util/data.h"
#include "../../../core/util/log.h"
#include "../../../core/util/mem.h"
#include "../../../third-party/crypto/bignum.h"
#include "../nano/merkle.h"
#include "../nano/serialize.h"
#include "gas.h"
#include "opcodes.h"
#include "precompiled.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
int exit_zero(void) { return 0; }
int evm_stack_push(evm_t* evm, uint8_t* data, uint8_t len) {
  if (evm->stack_size == EVM_STACK_LIMIT || len > 32) return EVM_ERROR_STACK_LIMIT;
  // we need to make sure the data ref is not part of the stack and would be ionvalidated now
  uint32_t tmp[32];
  memcpy(tmp, data, len);
  if (bb_check_size(&evm->stack, len + 1)) return EVM_ERROR_EMPTY_STACK;
  memcpy(evm->stack.b.data + evm->stack.b.len, tmp, len);
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
  uint8_t  i, l = 0;
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
#define __code(n)                     \
  {                                   \
    in3_log_trace(COLOR_GREEN_S2, n); \
    return;                           \
  }
void evm_print_op(evm_t* evm, uint64_t last_gas, uint32_t pos) {
  uint8_t op = evm->code.data[pos];
#ifdef EVM_GAS
  in3_log_trace("\n::: ");
  evm_t* pp = evm->parent;
  while (pp) {
    in3_log_trace(" .. ");
    pp = pp->parent;
  }

  if (last_gas > evm->gas) {
    in3_log_trace("%" PRIu64 " %03i " COLOR_YELLOW_PRIu64 " %02x : ", evm->gas, pos, last_gas - evm->gas, op);
  } else {
    in3_log_trace("%" PRIu64 " %03i " COLOR_YELLOW_PRIu64plus " %02x : ", evm->gas, pos, evm->gas - last_gas, op);
  }
#else
  UNUSED_VAR(last_gas);
  in3_log_trace("\n%03i       %02x : ", pos, op);
#endif
  if (op >= 0x60 && op <= 0x7F) {
    in3_log_trace(COLOR_GREEN_STR_INT "    %s", "PUSH", op - 0x5F, (op - 0x05F) < 10 ? " " : "");
    //    for (int j = 0; j < op - 0x5F; j++) printf("%02x", evm->code.data[evm->pos + j + 1]);
    return;
  }
  if (op >= 0x80 && op <= 0x8F) {
    in3_log_trace(COLOR_GREEN_STR_INT "     %s", "DUP", op - 0x7F, (op - 0x7F) < 10 ? " " : "");
    return;
  }
  if (op >= 0x90 && op <= 0x9F) {
    in3_log_trace(COLOR_GREEN_STR_INT "    %s", "SWAP", op - 0x8F, (op - 0x8F) < 10 ? " " : "");
    return;
  }
  if (op >= 0xA0 && op <= 0xA4) {
    in3_log_trace(COLOR_GREEN_STR_INT "      ", "LOG", op - 0xA0);
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
    case 0x15: __code("ISZERO");
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
    case 0x35: __code("CALLDATALOAD ");
    case 0x36: __code("CALLDATA_SIZE");
    case 0x37: __code("CALLDATACOPY ");
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
    case 0x46: __code("CHAINID");
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
  in3_log_trace(" [ ");
  for (int i = 0; i < evm->stack_size; i++) {
    uint8_t* dst = NULL;
    int      l   = evm_stack_get_ref(evm, i + 1, &dst);
    optimize_len(dst, l);
    for (int j = 0; j < l; j++) {
      if (j == 0 && dst[j] < 16) {
        in3_log_trace("%x", dst[j]);
      } else {
        in3_log_trace("%02x", dst[j]);
      }
    }
    in3_log_trace(" | ");
  }
}

int evm_execute(evm_t* evm) {

  uint8_t op = evm->code.data[evm->pos++];
  if (op >= 0x60 && op <= 0x7F) // PUSH
    op_exec(op_push(evm, op - 0x5F), G_VERY_LOW);
  if (op >= 0x80 && op <= 0x8F) // DUP
    op_exec(op_dup(evm, op - 0x7F), G_VERY_LOW);
  if (op >= 0x90 && op <= 0x9F) // SWAP
    op_exec(op_swap(evm, op - 0x8E), G_VERY_LOW);
  if (op >= 0xA0 && op <= 0xA4) // LOG
    op_exec(OP_LOG(evm, op - 0xA0), G_LOG);

  switch (op) {
    case 0x00: // STOP
      evm->state = EVM_STATE_STOPPED;
      return 0;

    case 0x01: //  ADD
      op_exec(op_math(evm, MATH_ADD, 0), G_VERY_LOW);
    case 0x02: //  MUL
      op_exec(op_math(evm, MATH_MUL, 0), G_LOW);
    case 0x03: //  SUB
      op_exec(op_math(evm, MATH_SUB, 0), G_VERY_LOW);
    case 0x04: //  DIV
      op_exec(op_math(evm, MATH_DIV, 0), G_LOW);
    case 0x05: //  SDIV
      op_exec(op_math(evm, MATH_SDIV, 0), G_LOW);
    case 0x06: //  MOD
      op_exec(op_math(evm, MATH_MOD, 0), G_LOW);
    case 0x07: //  SMOD
      op_exec(op_math(evm, MATH_SMOD, 0), G_LOW);
    case 0x08: //  ADDMOD
      op_exec(op_math(evm, MATH_ADD, 1), G_MID);
    case 0x09: //  MULMOD
      op_exec(op_math(evm, MATH_MUL, 1), G_MID);
    case 0x0A: //  EXP
      op_exec(op_math(evm, MATH_EXP, 0), G_EXP);
    case 0x0B: //  SIGNEXTEND
      op_exec(op_signextend(evm), G_LOW);

    case 0x10: // LT
      op_exec(op_cmp(evm, -1, 0), G_VERY_LOW);
    case 0x11: // GT
      op_exec(op_cmp(evm, 1, 0), G_VERY_LOW);
    case 0x12: // SLT
      op_exec(op_cmp(evm, -1, 1), G_VERY_LOW);
    case 0x13: // SGT
      op_exec(op_cmp(evm, 1, 1), G_VERY_LOW);
    case 0x14: // EQ
      op_exec(op_cmp(evm, 0, 0), G_VERY_LOW);
    case 0x15: // IS_ZERO
      op_exec(op_is_zero(evm), G_VERY_LOW);
    case 0x16: // AND
      op_exec(op_bit(evm, OP_AND), G_VERY_LOW);
    case 0x17: // OR
      op_exec(op_bit(evm, OP_OR), G_VERY_LOW);
    case 0x18: // XOR
      op_exec(op_bit(evm, OP_XOR), G_VERY_LOW);
    case 0x19: // NOT
      op_exec(op_not(evm), G_VERY_LOW);
    case 0x1a: // BYTE
      op_exec(op_byte(evm), G_VERY_LOW);
    case 0x1b: // SHL
      op_exec(op_shift(evm, 1), G_VERY_LOW);
    case 0x1c: // SHR
      op_exec(op_shift(evm, 0), G_VERY_LOW);
    case 0x1d: // SAR
      op_exec(op_shift(evm, 2), G_VERY_LOW);
    case 0x20: // SHA3
      op_exec(op_sha3(evm), G_SHA3);
    case 0x30: // ADDRESS
      op_exec(evm_stack_push(evm, evm->address, 20), G_BASE);
    case 0x31: // BALANCE
      op_exec(op_account(evm, EVM_ENV_BALANCE), G_BALANCE);
    case 0x32: // ORIGIN
      op_exec(evm_stack_push(evm, evm->origin, 20), G_BASE);
    case 0x33: // CALLER
      op_exec(evm_stack_push(evm, evm->caller, 20), G_BASE);
    case 0x34: // CALLVALUE
      op_exec(evm_stack_push(evm, evm->call_value.data, evm->call_value.len), G_BASE);
    case 0x35: // CALLDATALOAD
      op_exec(op_dataload(evm), G_VERY_LOW);
    case 0x36: // CALLDATA_SIZE
      op_exec(evm_stack_push_int(evm, evm->call_data.len), G_BASE);
    case 0x37: // CALLDATACOPY
      op_exec(op_datacopy(evm, &evm->call_data, 0), G_VERY_LOW);
    case 0x38: // CODESIZE
      op_exec(evm_stack_push_int(evm, evm->code.len), G_BASE);
    case 0x39: // CODECOPY
      op_exec(op_datacopy(evm, &evm->code, 0), G_VERY_LOW);
    case 0x3a: // GASPRICE
      op_exec(evm_stack_push(evm, evm->gas_price.data, evm->gas_price.len), G_BASE);
    case 0x3b: // EXTCODESIZE
      op_exec(op_account(evm, EVM_ENV_CODE_SIZE), G_EXTCODE);
    case 0x3c: // EXTCODECOPY
      op_exec(op_extcodecopy(evm), G_EXTCODE);
    case 0x3d: // RETURNDATASIZE
      op_exec(evm_stack_push_int(evm, evm->last_returned.len), G_BASE);
    case 0x3e: // RETURNDATACOPY
      op_exec(op_datacopy(evm, &evm->last_returned, 1), G_VERY_LOW);
    case 0x3f: // EXTCODEHASH
      op_exec(op_account(evm, EVM_ENV_CODE_HASH), G_BALANCE);
    case 0x40: // BLOCKHASH
      op_exec(op_account(evm, EVM_ENV_BLOCKHASH), G_BLOCKHASH);
    case 0x41: // COINBASE
      op_exec(op_header(evm, BLOCKHEADER_MINER), G_BASE);
    case 0x42: // TIMESTAMP
      op_exec(op_header(evm, BLOCKHEADER_TIMESTAMP), G_BASE);
    case 0x43: // NUMBER
      op_exec(op_header(evm, BLOCKHEADER_NUMBER), G_BASE);
    case 0x44: // DIFFICULTY
      op_exec(op_header(evm, BLOCKHEADER_DIFFICULTY), G_BASE);
    case 0x45: // GASLIMIT
      op_exec(op_header(evm, BLOCKHEADER_GAS_LIMIT), G_BASE);
    case 0x46: // CHAINID
      op_exec((evm->properties & EVM_PROP_ISTANBUL) ? evm_stack_push_long(evm, evm->chain_id) : EVM_ERROR_INVALID_OPCODE, G_BASE);

    case 0x50: // POP
      op_exec(evm_stack_pop(evm, NULL, 0), G_BASE);
    case 0x51: // MLOAD
      op_exec(op_mload(evm), G_VERY_LOW);
    case 0x52: // MSTORE
      op_exec(op_mstore(evm, 32), G_VERY_LOW);
    case 0x53: // MSTORE8
      op_exec(op_mstore(evm, 1), G_VERY_LOW);
    case 0x54: // SLOAD
      op_exec(op_sload(evm), evm->properties & EVM_PROP_FRONTIER ? FRONTIER_G_SLOAD : G_SLOAD);
    case 0x55: // SSTORE
      return OP_SSTORE(evm);
    case 0x56: // JUMP
      op_exec(op_jump(evm, 0), G_MID);
    case 0x57: // JUMPI
      op_exec(op_jump(evm, 1), G_HIGH);
    case 0x58: // PC
      op_exec(evm_stack_push_int(evm, evm->pos - 1), G_BASE);
    case 0x59: // MSIZE
      op_exec(evm_stack_push_int(evm, evm->memory.b.len), G_BASE);
    case 0x5a: // GAS     --> here we always return enough gas to keep going, since eth call should not use it anyway
#ifdef EVM_GAS
      op_exec(evm_stack_push_long(evm, evm->gas), G_BASE);
#else
      return evm_stack_push_int(evm, 0xFFFFFFF);
#endif
    case 0x5b: // JUMPDEST
      op_exec(0, G_JUMPDEST);
    case 0xF0: // CREATE
      op_exec(OP_CREATE(evm, 0), G_CREATE);
    case 0xF1: // CALL
      op_exec(op_call(evm, CALL_CALL), G_CALL);
    case 0xF2: // CALLCODE
      op_exec(op_call(evm, CALL_CODE), G_CALL);
    case 0xF3: // RETURN
      return op_return(evm, 0);
    case 0xF4: // DELEGATE_CALL
      op_exec(op_call(evm, CALL_DELEGATE), G_CALL);
    case 0xF5: // CREATE2
      op_exec(OP_CREATE(evm, 1), G_CREATE);
    case 0xFA: // STATIC_CALL
      op_exec(op_call(evm, CALL_STATIC), G_CALL);
    case 0xFD: // REVERT
      return op_return(evm, 1);
    case 0xFE: // INVALID OPCODE
      return EVM_ERROR_INVALID_OPCODE;
    case 0xFF: // SELFDESTRUCT
      op_exec(OP_SELFDESTRUCT(evm), (evm->properties & EVM_PROP_FRONTIER) ? 0 : G_SELFDESTRUCT);

    default:
      return EVM_ERROR_INVALID_OPCODE;
  }
}

int evm_run(evm_t* evm, address_t code_address) {

  INIT_GAS(evm);

  // for precompiled we simply execute it there
  if (evm_is_precompiled(evm, code_address))
    return evm_run_precompiled(evm, code_address);
  // timeout is simply used in case we don't use gas to make sure we don't run a infite loop.
  uint32_t timeout = 0xFFFFFFFF;
  int      res     = 0;
#ifdef DEBUG
  uint32_t last     = 0;
  uint64_t last_gas = 0;
#endif
  // inital state
  evm->state = EVM_STATE_RUNNING;

  // loop opcodes
  while (res >= 0 && evm->state == EVM_STATE_RUNNING && evm->pos < evm->code.len) {
    EVM_DEBUG_BLOCK({
      last     = evm->pos;
      last_gas = KEEP_TRACK_GAS(evm);
    });

    // execute the opcode
    res = evm_execute(evm);
    // display the result of the opcode (only if the debug flag is set)
#ifdef EVM_GAS
    // debug gas output
    EVM_DEBUG_BLOCK({ evm_print_stack(evm, last_gas, last); });
#endif
    if ((timeout--) == 0) return EVM_ERROR_TIMEOUT;
  }
  // done...

#ifdef EVM_GAS
  // debug gas output
  EVM_DEBUG_BLOCK({
    in3_log_trace("\n Result-code (%i)   init_gas: %" PRIu64 "   gas_left: %" PRIu64 "  refund: %" PRIu64 "  gas_used: %" PRIu64 "  ", res, evm->init_gas, evm->gas, evm->refund, evm->init_gas - evm->gas);
  });
#endif
  if (res == 0) FINALIZE_AND_REFUND_GAS(evm);

  // return result
  return res;
}
