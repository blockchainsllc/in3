#include "../../../core/util/bytes.h"
#include "evm.h"
#ifndef evm_mem_h__
#define evm_mem_h__

#define MEM_LIMIT 0xFFFFFFF // this cost about 8M gas
//#define MEM_INT_LIMIT 3   // bytes
int mem_check(evm_t* evm, uint32_t max_pos, uint8_t read_only);

int evm_mem_read_ref(evm_t* evm, uint32_t off, uint32_t len, bytes_t* src);
int evm_mem_read(evm_t* evm, bytes_t mem_off, uint8_t* dst, uint32_t len);
int evm_mem_readi(evm_t* evm, uint32_t off, uint8_t* dst, uint32_t len);
int evm_mem_write(evm_t* evm, uint32_t mem_off, bytes_t src, uint32_t len);

#endif