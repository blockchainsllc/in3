#include "mem.h"
#include "../eth_basic/eth_basic.h"
#include "../eth_nano/eth_nano.h"
#include "../eth_nano/merkle.h"
#include "../eth_nano/rlp.h"
#include "../eth_nano/serialize.h"
#include "big.h"
#include "eth_full.h"
#include "evm.h"
#include "gas.h"
#include <client/context.h>
#include <crypto/bignum.h>
#include <crypto/ecdsa.h>
#include <crypto/secp256k1.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/data.h>
#include <util/mem.h>
#include <util/utils.h>

int mem_check(evm_t* evm, uint32_t max_pos, uint8_t read_only) {
  if (max_pos >= MEM_LIMIT) return EVM_ERROR_OUT_OF_GAS;
  if (max_pos > evm->memory.b.len) {

#ifdef EVM_GAS
    uint64_t old_wc = (evm->memory.b.len + 31) / 32;
    uint64_t new_wc = (max_pos + 31) / 32;
    if (new_wc > old_wc) {
      uint64_t old_cost = old_wc * G_MEMORY + (old_wc * old_wc) / 512;
      uint64_t new_cost = new_wc * G_MEMORY + (new_wc * new_wc) / 512;
      if (new_cost > old_cost)
        subgas(new_cost - old_cost);
      max_pos = new_wc * 32;
    }
#endif
    evm->memory.b.len = max_pos;
  }

  if (!read_only && max_pos > evm->memory.bsize) {
    uint32_t old_l = evm->memory.bsize, msize = evm->memory.b.len;
    evm->memory.b.len = 0;
    int err           = bb_check_size(&evm->memory, max_pos);
    evm->memory.b.len = msize;
    if (old_l < evm->memory.bsize)
      memset(evm->memory.b.data + old_l, 0, evm->memory.bsize - old_l);
    return err;
  }

  return 0;
}

int evm_mem_readi(evm_t* evm, uint32_t off, uint8_t* dst, uint32_t len) {
  if (!len) return 0;
  uint8_t* src     = NULL;
  uint32_t max_len = 0;
  if (mem_check(evm, off + len, 1) < 0) return EVM_ERROR_OUT_OF_GAS;
  if (off < evm->memory.bsize) {
    src     = evm->memory.b.data + off;
    max_len = evm->memory.bsize - off;
  }

  if (src == NULL)
    memset(dst, 0, len);
  else {
    if (max_len < len) {
      memset(dst + max_len, 0, len - max_len);
      memcpy(dst, src, max_len);
    } else
      memcpy(dst, src, len);
  }
  return 0;
}

int evm_mem_read(evm_t* evm, bytes_t mem_off, uint8_t* dst, uint32_t len) {
  b_optimize_len(&mem_off);
  if (mem_off.len > 4) return EVM_ERROR_OUT_OF_GAS;
  return evm_mem_readi(evm, bytes_to_int(mem_off.data, mem_off.len), dst, len);
}
int evm_mem_read_ref(evm_t* evm, uint32_t off, uint32_t len, bytes_t* src) {
  src->data = NULL;
  src->len  = 0;
  if (mem_check(evm, off + len, 1) < 0) return EVM_ERROR_OUT_OF_GAS;
  if (off < evm->memory.bsize) {
    src->data = evm->memory.b.data + off;
    src->len  = min(len, evm->memory.bsize - off);
  }
  return 0;
}

int evm_mem_write(evm_t* evm, uint32_t off, bytes_t src, uint32_t len) {
  if (mem_check(evm, off + len, 0) < 0) return EVM_ERROR_OUT_OF_GAS;
  if (evm->properties & EVM_PROP_DEBUG) {
    printf("\n   MEM: writing %i bytes to %i : ", len, off);
    b_print(&src);
  }

  if (src.data == NULL)
    memset(evm->memory.b.data + off, 0, len);
  else {
    if (src.len >= len)
      memcpy(evm->memory.b.data + off, src.data + src.len - len, len);
    else {
      memset(evm->memory.b.data + off, 0, len - src.len);
      memcpy(evm->memory.b.data + off + len - src.len, src.data, src.len);
    }
  }
  return 0;
}
