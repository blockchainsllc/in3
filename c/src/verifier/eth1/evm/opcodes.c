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

#include "opcodes.h"
#include "../../../core/client/context.h"
#include "../../../core/util/data.h"
#include "../../../core/util/log.h"
#include "../../../core/util/mem.h"
#include "../../../third-party/crypto/bignum.h"
#include "../../../third-party/crypto/ecdsa.h"
#include "../nano/merkle.h"
#include "../nano/rlp.h"
#include "../nano/serialize.h"
#include "big.h"
#include "evm_mem.h"
#include "gas.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

int op_math(evm_t* evm, uint8_t op, uint8_t mod) {
  uint8_t *a, *b, res[65], *r = res;
  int      la = evm_stack_pop_ref(evm, &a), lb = evm_stack_pop_ref(evm, &b), l;
  if (la < 0 || lb < 0) return EVM_ERROR_EMPTY_STACK;
  switch (op) {
    case MATH_ADD:
      l = big_add(a, la, b, lb, res, mod ? 64 : 32);
      break;
    case MATH_SUB:
      l = big_sub(a, la, b, lb, res);
      break;
    case MATH_MUL:
      l = big_mul(a, la, b, lb, res, mod ? 65 : 32);
      break;
    case MATH_DIV:
      l = big_div(a, la, b, lb, 0, res);
      break;
    case MATH_SDIV:
      l = big_div(a, la, b, lb, 1, res);
      break;
    case MATH_MOD:
      l = big_mod(a, la, b, lb, 0, res);
      break;
    case MATH_SMOD:
      l = big_mod(a, la, b, lb, 1, res);
      break;
    case MATH_EXP:
      l = big_exp(a, la, b, lb, res);
      subgas((evm->properties & EVM_PROP_FRONTIER ? FRONTIER_G_EXPBYTE : G_EXPBYTE) * big_log256(b, lb));
      break;
    default:
      return EVM_ERROR_INVALID_OPCODE;
  }

  if (l < 0) return EVM_ERROR_UNSUPPORTED_CALL_OPCODE;
  // optimize
  optimize_len(r, l);

  if (mod) {
    uint8_t *mod_data, tmp[65];
    int      modl = evm_stack_pop_ref(evm, &mod_data);
    if (modl < 0) return modl;
    memcpy(tmp, r, l);
    l = big_mod(tmp, l, mod_data, modl, 0, res);
    r = res;
    if (l < 0) return l;

    // optimize
    optimize_len(r, l);
  }

  return evm_stack_push(evm, r, l);
}

int op_signextend(evm_t* evm) {
  uint8_t* val = NULL;
  int32_t  k   = evm_stack_pop_int(evm), l;

  if (k < 0) return k;
  if (k > 31) return 0;
  if ((l = evm_stack_pop_ref(evm, &val)) < 0) return l;

  //  uint8_t bit_set = l>k ? val[l-k-1] & 128 :0
  bool    bitset = l > k && val[l - k - 1] & 128;
  uint8_t tmp[32], tmp2[32];
  memset(tmp2, 0, 32);
  memcpy(tmp2, val, l);
  val = tmp2;
  if (k < 31)
    memset(tmp, bitset ? 0xFF : 0, 31 - k);
  memcpy(tmp + 31 - k, val + l - k - 1, k + 1);
  val = tmp;
  l   = 32;
  optimize_len(val, l);
  return evm_stack_push(evm, val, l);
}

int op_is_zero(evm_t* evm) {
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

int op_not(evm_t* evm) {
  uint8_t res[32], *a;
  int     l = evm_stack_pop_ref(evm, &a), i;
  if (l < 0) return l;
  if (l < 32) memset(res, 0, 32 - l);
  memcpy(res + 32 - l, a, l);

  for (i = 0; i < 32; i++) res[i] ^= 0xFF;

  a = res;
  l = 32;
  optimize_len(a, l);

  // push result to stack
  return evm_stack_push(evm, a, l);
}

int op_bit(evm_t* evm, uint8_t op) {
  uint8_t result[32], *res = result, *a;
  int     l, l1            = evm_stack_pop_ref(evm, &a), i, j;
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

  optimize_len(res, l1);

  // push result to stack
  return evm_stack_push(evm, res, l1);
}

int op_byte(evm_t* evm) {
  uint8_t pos, *b, res = 0xFF;
  int     l = evm_stack_pop_byte(evm, &pos);
  if (l == EVM_ERROR_EMPTY_STACK) return l;
  if (l < 0 || (pos & 0xE0)) res = 0;
  if ((l = evm_stack_pop_ref(evm, &b)) < 0) return EVM_ERROR_EMPTY_STACK;
  if (res) res = pos >= (32 - l) ? b[pos + l - 32] : 0;
  return evm_stack_push(evm, &res, 1);
}

int op_cmp(evm_t* evm, int8_t eq, uint8_t sig) {
  uint8_t *a, *b, res = 0, sig_a = 0, sig_b = 0;
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
  if ((evm->properties & EVM_PROP_CONSTANTINOPL) == 0) return EVM_ERROR_INVALID_OPCODE;
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
      for (l = 0; l < (pos >> 3); l++) res[l] = 0xFF;
      l = 8 - (pos % 8);
      res[pos >> 3] |= (0XFF >> l) << l;
      return evm_stack_push(evm, res, 32);
    }
  }

  b   = res;
  pos = 32;
  optimize_len(b, pos);
  return evm_stack_push(evm, b, pos);
}

int op_sha3(evm_t* evm) {
  int offset = evm_stack_pop_int(evm);
  if (offset < 0) return offset;
  //  if (offset == MEM_LIMIT) return EVM_ERROR_OUT_OF_GAS;
  int len = evm_stack_pop_int(evm);
  if (len < 0) return len;
  if (len == MEM_LIMIT) return EVM_ERROR_OUT_OF_GAS;

  bytes_t src = {.data = NULL, .len = 0};
  if (len && evm_mem_read_ref(evm, offset, len, &src) < 0) return EVM_ERROR_OUT_OF_GAS;
  subgas(((len + 31) / 32) * G_SHA3WORD);

  uint8_t         res[32];
  struct SHA3_CTX ctx;
  sha3_256_Init(&ctx);
  if (src.data && src.len >= (uint32_t) len)
    sha3_Update(&ctx, src.data, len);
  else {
    uint8_t  tmp[32];
    uint32_t p = 0;
    memset(tmp, 0, 32);
    if (src.data) {
      sha3_Update(&ctx, src.data, src.len);
      p += src.len;
    }
    while (p < (uint32_t) len) {
      uint8_t part = 32;
      if (len - p < 32) part = len - p;
      sha3_Update(&ctx, tmp, part);
      p += part;
    }
  }
  keccak_Final(&ctx, res);
  return evm_stack_push(evm, res, 32);
}

int op_account(evm_t* evm, uint8_t key) {
  if ((evm->properties & EVM_PROP_CONSTANTINOPL) == 0 && key == EVM_ENV_CODE_HASH) return EVM_ERROR_UNSUPPORTED_CALL_OPCODE;
  uint8_t *address, *data;
  int      l = evm_stack_pop_ref(evm, &address);
  if (l < 0) return EVM_ERROR_EMPTY_STACK;
  OP_ACCOUNT_GAS(evm, key, address, data, l);
  l = evm->env(evm, key, address, l, &data, 0, 0);
  return l < 0 ? l : evm_stack_push(evm, data, l);
}
int op_dataload(evm_t* evm) {
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

int op_datacopy(evm_t* evm, bytes_t* src, uint_fast8_t check_size) {
  int mem_pos = evm_stack_pop_int(evm), data_pos = evm_stack_pop_int(evm), data_len = evm_stack_pop_int(evm), res = 0;
  if (mem_pos < 0 || data_len < 0 || data_pos < 0) return EVM_ERROR_EMPTY_STACK;
  subgas(((data_len + 31) / 32) * G_COPY);
  bytes_t src_data = {.data = ((uint32_t) data_pos < src->len) ? (src->data + data_pos) : NULL, .len = src->len - data_pos};
  src_data.len     = src_data.data ? min(src_data.len, ((uint32_t) data_len)) : 0;
  if (check_size && !src_data.data) return EVM_ERROR_ILLEGAL_MEMORY_ACCESS;

  if (src_data.len < (uint32_t) data_len)
    res = evm_mem_write(evm, mem_pos + src_data.len, bytes(NULL, 0), data_len - src_data.len);

  if (src_data.len && res == 0)
    res = evm_mem_write(evm, mem_pos, src_data, src_data.len);
  return res;
}

int op_extcodecopy(evm_t* evm) {
  address_t address;
  uint8_t*  data = NULL;
  int       l = evm_stack_pop(evm, address, 20), mem_pos = evm_stack_pop_int(evm), code_pos = evm_stack_pop_int(evm), data_len = evm_stack_pop_int(evm);
  if (l < 0 || mem_pos < 0 || data_len < 0 || code_pos < 0) return EVM_ERROR_EMPTY_STACK;
  subgas(((data_len + 31) / 32) * G_COPY);
  OP_EXTCODECOPY_GAS(evm);
  // address, memOffset, codeOffset, length
  int res = evm->env(evm, EVM_ENV_CODE_COPY, address, 20, &data, code_pos, data_len);
  if (res < 0)
    // we will write 0x0
    return evm_mem_write(evm, mem_pos, bytes(NULL, 0), data_len);
  else
    return evm_mem_write(evm, mem_pos, bytes(data, res), data_len);
}

int op_header(evm_t* evm, uint8_t index) {
  bytes_t b;
  int     l;
  if ((l = evm->env(evm, EVM_ENV_BLOCKHEADER, NULL, 0, &b.data, 0, 0)) < 0) return l;
  b.len = l;

  if (rlp_decode_in_list(&b, index, &b) == 1)
    return evm_stack_push(evm, b.data, b.len);
  else
    return evm_stack_push_int(evm, 0);
}

int op_mload(evm_t* evm) {
  uint8_t *off, *dst;
  int      off_len = evm_stack_pop_ref(evm, &off);
  if (off_len < 0) return off_len;

  uint8_t tmp[32] = {0};
  memcpy(tmp + 32 - off_len, off, off_len);
  if (evm_stack_push_ref(evm, &dst, 32)) return EVM_ERROR_ILLEGAL_MEMORY_ACCESS;
  return evm_mem_read(evm, bytes(tmp, 32), dst, 32);
}

int op_mstore(evm_t* evm, uint8_t len) {
  int offset = evm_stack_pop_int(evm);
  if (offset < 0) return offset;
  uint8_t* data     = NULL;
  int      data_len = evm_stack_pop_ref(evm, &data);

  if (data_len < 0) return data_len;

  if (len == 32) {
    uint8_t tmp[32];
    memset(tmp, 0, 32);
    memcpy(tmp + 32 - data_len, data, data_len);
    return evm_mem_write(evm, offset, bytes(tmp, 32), len);
  }
  return evm_mem_write(evm, offset, bytes(data, data_len), len);
}

int op_sload(evm_t* evm) {
  uint8_t *key, *value;
  int      l;
  if ((l = evm_stack_pop_ref(evm, &key)) < 0) return l;
  OP_SLOAD_GAS(evm);
  if ((l = evm->env(evm, EVM_ENV_STORAGE, key, l, &value, 0, 0)) < 0) return l;
  return evm_stack_push(evm, value, l);
}

int op_jump(evm_t* evm, uint8_t cond) {
  int pos = evm_stack_pop_int(evm);
  if (pos < 0) return pos;
  if (cond) {
    uint8_t c   = 0;
    int     ret = evm_stack_pop_byte(evm, &c);
    if (ret == EVM_ERROR_EMPTY_STACK) return EVM_ERROR_EMPTY_STACK;
    if (!c && ret >= 0) return 0; // the condition was false
  }
  if ((uint32_t) pos > evm->code.len || evm->code.data[pos] != 0x5B) return EVM_ERROR_INVALID_JUMPDEST;

  // check if this is a invalid jumpdest
  if (evm->invalid_jumpdest == NULL) {
    uint32_t size = 8, p = 0, *list = _malloc(8 * sizeof(uint32_t)), i, cl = evm->code.len;
    uint8_t  op, jumpl = 0;
    for (i = 0; i < cl; i++) {
      op = evm->code.data[i];
      if (jumpl) {
        if (op == 0x5B) {
          // add it
          if (p == size - 2) {
            list = _realloc(list, (size + 8) * sizeof(uint32_t), size * sizeof(uint32_t));
            size = size + 8;
          }
          list[p++] = i;
        }
        jumpl--;
      } else if (op >= 0x60 && op <= 0x7F) // PUSH
        jumpl = op - 0x5F;
    }
    list[p]               = 0xFFFFFFFF;
    evm->invalid_jumpdest = list;
  }

  // check if our dest contains the pos as invalid
  for (int i = 0;; i++) {
    if (evm->invalid_jumpdest[i] == 0xFFFFFFFF) break;
    if (evm->invalid_jumpdest[i] == (uint32_t) pos) return EVM_ERROR_INVALID_JUMPDEST;
  }

  evm->pos = pos;
  return 0;
}

int op_push(evm_t* evm, wlen_t len) {
  if (evm->code.len < (uint32_t) evm->pos + len) {
    bytes32_t tmp;
    memset(tmp, 0, 32);
    memcpy(tmp, evm->code.data + evm->pos, evm->code.len - evm->pos);
    evm->pos += len;
    return evm_stack_push(evm, tmp, len);
  }
  if (evm_stack_push(evm, evm->code.data + evm->pos, len) < 0)
    return EVM_ERROR_BUFFER_TOO_SMALL;
  evm->pos += len;
  return 0;
}

int op_dup(evm_t* evm, uint8_t pos) {
  uint8_t* data = NULL;
  int      l    = evm_stack_get_ref(evm, pos, &data);
  if (l < 0) return l;
  return evm_stack_push(evm, data, l);
}

int op_swap(evm_t* evm, uint8_t pos) {
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
    memcpy(a + l1 - l2, b, l2 + 1);
    if (pos > 2) memmove(b + l1 + 1, b + l2 + 1, a - b - l2 - 1);
    memcpy(b, data, l1 + 1);
  }
  return 0;
}

int op_return(evm_t* evm, uint8_t revert) {
  int offset, len;
  if ((offset = evm_stack_pop_int(evm)) < 0) return offset;
  if ((len = evm_stack_pop_int(evm)) < 0) return len;
  if (len > MEM_LIMIT) return EVM_ERROR_OUT_OF_GAS;

  if (evm->return_data.data) _free(evm->return_data.data);
  evm->return_data.data = _malloc(len);
  if (!evm->return_data.data) return EVM_ERROR_BUFFER_TOO_SMALL;
  if (evm_mem_readi(evm, offset, evm->return_data.data, len) < 0) return EVM_ERROR_OUT_OF_GAS;
  evm->return_data.len = len;
  evm->state           = revert ? EVM_STATE_REVERTED : EVM_STATE_STOPPED;
  return 0;
}

int op_call(evm_t* evm, uint8_t mode) {
  // 0         1          2      3         4         5          6
  // gasLimit, toAddress, value, inOffset, inLength, outOffset, outLength
  uint8_t *gas_limit, address[20], *value, zero = 0;
  int32_t  l_gas, l_value = 0, in_offset, in_len, out_offset, out_len;
  if ((l_gas = evm_stack_pop_ref(evm, &gas_limit)) < 0) return l_gas;
  if (evm_stack_pop(evm, address, 20) < 0) return EVM_ERROR_EMPTY_STACK;
  if ((mode == CALL_CALL || mode == CALL_CODE) && (l_value = evm_stack_pop_ref(evm, &value)) < 0) return l_value;
  if ((in_offset = evm_stack_pop_int(evm)) < 0) return in_offset;
  if ((in_len = evm_stack_pop_int(evm)) < 0) return in_len;
  if ((out_offset = evm_stack_pop_int(evm)) < 0) return out_offset;
  if ((out_len = evm_stack_pop_int(evm)) < 0) return out_len;
  uint64_t gas = bytes_to_long(gas_limit, l_gas);

  if ((out_len > 0 && mem_check(evm, out_offset + out_len, true) < 0) || (in_len && mem_check(evm, in_offset + in_len, true) < 0)) return EVM_ERROR_ILLEGAL_MEMORY_ACCESS;

  //TODO  do we need this check?
  //  if ((uint32_t) in_offset + in_len > evm->memory.bsize) return EVM_ERROR_ILLEGAL_MEMORY_ACCESS;

  switch (mode) {
    case CALL_CALL:
      return evm_sub_call(evm,
                          address, address,
                          value, l_value,
                          evm->memory.b.data + in_offset, in_len,
                          evm->address,
                          evm->origin, gas, EVM_CALL_MODE_CALL, out_offset, out_len);
    case CALL_CODE:
      return evm_sub_call(evm,
                          evm->address, address,
                          value, l_value,
                          evm->memory.b.data + in_offset, in_len,
                          evm->address,
                          evm->origin, gas, EVM_CALL_MODE_CALLCODE, out_offset, out_len);
    case CALL_DELEGATE:
      return evm_sub_call(evm,
                          evm->address, address,
                          evm->call_value.data, evm->call_value.len,
                          evm->memory.b.data + in_offset, in_len,
                          evm->caller,
                          evm->origin, gas, EVM_CALL_MODE_DELEGATE, out_offset, out_len);
    case CALL_STATIC:
      return evm_sub_call(evm,
                          address, address,
                          &zero, 1,
                          evm->memory.b.data + in_offset, in_len,
                          evm->address,
                          evm->origin, gas, EVM_CALL_MODE_STATIC, out_offset, out_len);
  }
  return EVM_ERROR_INVALID_OPCODE;
}

#ifdef EVM_GAS
int op_create(evm_t* evm, uint_fast8_t use_salt) {
  bytes_t   in_data, tmp;
  uint8_t*  value   = NULL;
  int32_t   l_value = 0, in_offset, in_len;
  bytes32_t hash;
  // read data from stack
  TRY_SET(l_value, evm_stack_pop_ref(evm, &value));
  TRY_SET(in_offset, evm_stack_pop_int(evm));
  TRY_SET(in_len, evm_stack_pop_int(evm));

  // check gas for extending memory
  TRY(mem_check(evm, in_offset + in_len, true));

  // read the data from memory
  TRY(evm_mem_read_ref(evm, in_offset, in_len, &in_data));

  if (use_salt == 0) {
    //  calculate the generated address
    uint8_t*         nonce = evm_get_account(evm, evm->address, true)->nonce;
    bytes_builder_t* bb    = bb_new();
    tmp                    = bytes(evm->address, 20);
    rlp_encode_item(bb, &tmp);
    if (big_is_zero(nonce, 32))
      tmp.len = 0;
    else {
      tmp.len  = 32;
      tmp.data = nonce;
      optimize_len(tmp.data, tmp.len);
    }
    rlp_encode_item(bb, &tmp);
    rlp_encode_to_list(bb);
    sha3_to(&bb->b, hash);
    bb_free(bb);
  } else {
    // CREATE2 is only allowed after CONSTANTINOPL
    if ((evm->properties & EVM_PROP_CONSTANTINOPL) == 0) return EVM_ERROR_INVALID_OPCODE;
    uint8_t buffer[85]; // 1 +20 +32+32
    tmp.data  = buffer;
    tmp.len   = 85;
    buffer[0] = 0xFF;
    memcpy(buffer + 1, evm->address, 20);
    TRY(evm_stack_pop(evm, buffer + 21, 32));
    sha3_to(&in_data, buffer + 21 + 32);
    sha3_to(&tmp, hash);
  }

  // now execute the call
  return evm_sub_call(evm, NULL, hash + 12, value, l_value, in_data.data, in_data.len, evm->address, evm->origin, 0, 0, 0, 0);
}
int op_selfdestruct(evm_t* evm) {
  uint8_t adr[20], l, *p;
  if (evm_stack_pop(evm, adr, 20) < 0) return EVM_ERROR_EMPTY_STACK;
  account_t* self_account = evm_get_account(evm, evm->address, 1);
  // TODO check if this account was selfsdesstructed before
  evm->refund += R_SELFDESTRUCT;

  l = 32;
  p = self_account->balance;
  optimize_len(p, l);
  if (l && (l > 1 || *p != 0)) {
    if (evm_get_account(evm, adr, 0) == NULL) {
      if ((evm->properties & EVM_PROP_NO_FINALIZE) == 0) subgas(G_NEWACCOUNT);
      evm_get_account(evm, adr, 1);
    }
    if (transfer_value(evm, evm->address, adr, self_account->balance, 32, 0) < 0) return EVM_ERROR_OUT_OF_GAS;
  }
  memset(self_account->balance, 0, 32);
  memset(self_account->nonce, 0, 32);
  self_account->code.len = 0;
  storage_t* s           = NULL;
  while (self_account->storage) {
    s                     = self_account->storage;
    self_account->storage = s->next;
    _free(s);
  }
  evm->state = EVM_STATE_STOPPED;
  return 0;
}
int op_log(evm_t* evm, uint8_t len) {
  int memoffset = evm_stack_pop_int(evm);
  if (memoffset < 0) return memoffset;
  int memlen = evm_stack_pop_int(evm);
  if (memlen < 0) return memlen;
  subgas(len * G_LOGTOPIC + memlen * G_LOGDATA);

  if (memlen) TRY(mem_check(evm, memoffset + memlen, true));

  logs_t* log = _malloc(sizeof(logs_t));

  log->next      = evm->logs;
  evm->logs      = log;
  log->data.data = _malloc(memlen);
  log->data.len  = memlen;

  evm_mem_readi(evm, memoffset, log->data.data, memlen);
  log->topics.data = _malloc(len * 32);
  log->topics.len  = len * 32;

  uint8_t* t = NULL;
  int      l;

  for (int i = 0; i < len; i++) {
    if ((l = evm_stack_pop_ref(evm, &t)) < 0) return l;
    if (l < 32) memset(log->topics.data + i * 32, 0, 32 - l);
    memcpy(log->topics.data + i * 32 + 32 - l, t, l);
  }
  return 0;
}
int op_sstore(evm_t* evm) {
  uint8_t *key, *value;
  int      l_key, l_val;
  if ((l_key = evm_stack_pop_ref(evm, &key)) < 0) return l_key;
  if ((l_val = evm_stack_pop_ref(evm, &value)) < 0) return l_val;

  storage_t* s       = evm_get_storage(evm, evm->account, key, l_key, 0);
  uint8_t    created = s == NULL, el = l_val;
  uint8_t    l_current = 0;
  if (created)
    s = evm_get_storage(evm, evm->account, key, l_key, 1);
  else {
    created = true;
    for (int i = 0; i < 32; i++) {
      if (s->value[i] != 0) {
        l_current = 32 - i;
        created   = false;
        break;
      }
    }
  }

  while (el > 0 && value[l_val - el] == 0) el--;

  if (evm->properties & EVM_PROP_CONSTANTINOPL) {
    uint8_t* original   = NULL;
    uint8_t  changed    = big_cmp(value, l_val, s->value, 32);
    int      l_original = evm->env(evm, EVM_ENV_STORAGE, key, l_key, &original, 0, 0); // wo we need this call, or simply use s?
    if (l_original < 0) l_original = 0;

    if (!changed) {
      subgas(GAS_CC_NET_SSTORE_NOOP_GAS);
    } else if (big_cmp(original, l_original, s->value, 32) == 0) {
      if (l_original == 0) {
        subgas(GAS_CC_NET_SSTORE_INIT_GAS);
      }
      if (el == 0) {
        evm->refund += GAS_CC_NET_SSTORE_CLEAR_REFUND;
      }

      subgas(GAS_CC_NET_SSTORE_CLEAN_GAS);
    } else {
      if (l_original) {
        if (l_current == 0)
          evm->gas -= GAS_CC_NET_SSTORE_CLEAR_REFUND;
        else
          evm->refund += GAS_CC_NET_SSTORE_CLEAR_REFUND;
      }

      if (big_cmp(original, l_original, value, l_val) == 0) {
        if (l_original == 0)
          evm->refund += GAS_CC_NET_SSTORE_RESET_CLEAR_REFUND;
        else
          evm->refund += GAS_CC_NET_SSTORE_RESET_REFUND;
      }
      subgas(GAS_CC_NET_SSTORE_DIRTY_GAS);
    }
  } else {
    if (el == 0 && created) {
      subgas(G_SRESET);
    } else if (el == 0 && !created) {
      subgas(G_SRESET);
      evm->refund += R_SCLEAR;
    } else if (el && created) {
      subgas(G_SSET);
    } else if (el && !created) {
      subgas(G_SRESET);
    }
  }

  uint256_set(value, l_val, s->value);
  return 0;
}
#endif
