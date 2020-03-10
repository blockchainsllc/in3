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

/** @file
 * main evm-file.
 * */

#include "../../../core/util/bytes.h"
#ifndef evm_h__
#define evm_h__
int exit_zero(void);
//#define EVM_GAS
/** the current state of the evm*/
typedef enum evm_state {
  EVM_STATE_INIT     = 0, /**< just initialised, but not yet started */
  EVM_STATE_RUNNING  = 1, /**< started and still running */
  EVM_STATE_STOPPED  = 2, /**< successfully stopped */
  EVM_STATE_REVERTED = 3  /**< stopped, but results must be reverted */
} evm_state_t;

#ifdef EVM_GAS
#define gas_options       \
  struct {                \
    account_t*  accounts; \
    struct evm* parent;   \
    logs_t*     logs;     \
    uint64_t    refund;   \
    uint64_t    init_gas; \
  }
#else
#define gas_options
#endif

#define EVM_ERROR_EMPTY_STACK -20             /**< the no more elements on the stack  */
#define EVM_ERROR_INVALID_OPCODE -21          /**< the opcode is not supported  */
#define EVM_ERROR_BUFFER_TOO_SMALL -22        /**< reading data from a position, which is not initialized  */
#define EVM_ERROR_ILLEGAL_MEMORY_ACCESS -23   /**< the memory-offset does not exist  */
#define EVM_ERROR_INVALID_JUMPDEST -24        /**< the jump destination is not marked as valid destination  */
#define EVM_ERROR_INVALID_PUSH -25            /**< the push data is empy */
#define EVM_ERROR_UNSUPPORTED_CALL_OPCODE -26 /**< error handling the call, usually because static-calls are not allowed to change state  */
#define EVM_ERROR_TIMEOUT -27                 /**< the evm ran into a loop  */
#define EVM_ERROR_INVALID_ENV -28             /**< the enviroment could not deliver the data  */
#define EVM_ERROR_OUT_OF_GAS -29              /**< not enough gas to exewcute the opcode  */
#define EVM_ERROR_BALANCE_TOO_LOW -30         /**< not enough funds to transfer the requested value.  */
#define EVM_ERROR_STACK_LIMIT -31             /**< stack limit reached  */
#define EVM_ERROR_SUCCESS_CONSUME_GAS -32     /**< write success but consume all gas */

#define EVM_PROP_FRONTIER 1
#define EVM_PROP_EIP150 2
#define EVM_PROP_EIP158 4
#define EVM_PROP_CONSTANTINOPL 16
#define EVM_PROP_ISTANBUL 32
#define EVM_PROP_NO_FINALIZE 32768
#define EVM_PROP_STATIC 256

#define EVM_ENV_BALANCE 1
#define EVM_ENV_CODE_SIZE 2
#define EVM_ENV_CODE_COPY 3
#define EVM_ENV_BLOCKHASH 4
#define EVM_ENV_STORAGE 5
#define EVM_ENV_BLOCKHEADER 6
#define EVM_ENV_CODE_HASH 7
#define EVM_ENV_NONCE 8

#define MATH_ADD 1
#define MATH_SUB 2
#define MATH_MUL 3
#define MATH_DIV 4
#define MATH_SDIV 5
#define MATH_MOD 6
#define MATH_SMOD 7
#define MATH_EXP 8
#define MATH_SIGNEXP 9

#define CALL_CALL 0
#define CALL_CODE 1
#define CALL_DELEGATE 2
#define CALL_STATIC 3
#define OP_AND 0
#define OP_OR 1
#define OP_XOR 2

#if defined(DEBUG)
#define EVM_DEBUG_BLOCK(_code_block_) \
  if (in3_log_level_is(LOG_TRACE)) (_code_block_)
#else
#define EVM_DEBUG_BLOCK(...)
#endif

#ifdef EVM_GAS

#define OP_EXTCODECOPY_GAS(evm)                                                                                                                        \
  do {                                                                                                                                                 \
    account_t* ac = evm_get_account(evm, address, 0);                                                                                                  \
    if (ac && ac->code.len)                                                                                                                            \
      return evm_mem_write(evm, mem_pos, bytes(ac->code.data + code_pos, ac->code.len > (uint32_t) code_pos ? ac->code.len - code_pos : 0), data_len); \
  } while (0)

#define OP_SLOAD_GAS(evm)                                         \
  do {                                                            \
    storage_t* s = evm_get_storage(evm, evm->account, key, l, 0); \
    if (s) {                                                      \
      value = s->value;                                           \
      l     = 32;                                                 \
      while (value[0] == 0 && l > 1) {                            \
        l--;                                                      \
        value++;                                                  \
      }                                                           \
      return evm_stack_push(evm, value, l);                       \
    }                                                             \
  } while (0)

#define OP_ACCOUNT_GAS(evm, key, address, data, l)             \
  do {                                                         \
    if (key != EVM_ENV_BLOCKHASH) {                            \
      account_t* ac = evm_get_account(evm, address, 0);        \
      uint8_t    tmp[4];                                       \
      uint8_t    hash[32];                                     \
      if (ac) {                                                \
        data = NULL;                                           \
        if (key == EVM_ENV_BALANCE) {                          \
          data = ac->balance;                                  \
          l    = 32;                                           \
        } else if (key == EVM_ENV_CODE_SIZE && ac->code.len) { \
          int_to_bytes(ac->code.len, tmp);                     \
          data = tmp;                                          \
          l    = 4;                                            \
        } else if (key == EVM_ENV_CODE_COPY && ac->code.len) { \
          data = ac->code.data;                                \
          l    = ac->code.len;                                 \
        } else if (key == EVM_ENV_CODE_HASH && ac->code.len) { \
          sha3_to(&ac->code, hash);                            \
          data = hash;                                         \
          l    = 32;                                           \
        }                                                      \
        if (data) {                                            \
          while (data[0] == 0 && l > 1) {                      \
            l--;                                               \
            data++;                                            \
          }                                                    \
          return evm_stack_push(evm, data, l);                 \
        }                                                      \
      }                                                        \
    }                                                          \
  } while (0)

#define OP_CREATE(evm, use_salt) op_create(evm, use_salt)
#define OP_SELFDESTRUCT(evm) op_selfdestruct(evm)
#define OP_LOG(evm, len) op_log(evm, len)
#define OP_SSTORE(evm) op_sstore(evm)
#else
#define OP_LOG(...) EVM_ERROR_UNSUPPORTED_CALL_OPCODE
#define OP_SLOAD_GAS(...)
#define OP_CREATE(...) EVM_ERROR_UNSUPPORTED_CALL_OPCODE
#define OP_ACCOUNT_GAS(...) exit_zero()
#define OP_SELFDESTRUCT(...) EVM_ERROR_UNSUPPORTED_CALL_OPCODE
#define OP_EXTCODECOPY_GAS(evm)
#define OP_SSTORE(...) EVM_ERROR_UNSUPPORTED_CALL_OPCODE
#endif

/**
 * This function provides data from the enviroment.
 * 
 * depending on the key the function will set the out_data-pointer to the result. 
 * This means the enviroment is responsible for memory management and also to clean up resources afterwards.
 * 
 * @param evm passes the current evm-pointer
 * @param evm_key the requested value
 * @param in_data the pointer to the argument like the address
 * @param  
 * 
 * 
 */
typedef int (*evm_get_env)(void* evm, uint16_t evm_key, uint8_t* in_data, int in_len, uint8_t** out_data, int offset, int len);

typedef struct account_storage {
  bytes32_t               key;
  bytes32_t               value;
  struct account_storage* next;
} storage_t;
typedef struct logs {
  bytes_t      topics;
  bytes_t      data;
  struct logs* next;
} logs_t;

typedef struct account {
  address_t       address;
  bytes32_t       balance;
  bytes32_t       nonce;
  bytes_t         code;
  storage_t*      storage;
  struct account* next;
} account_t;

typedef struct evm {
  // internal data
  bytes_builder_t stack;
  //  bytes_builder_t memory;
  bytes_builder_t memory;
  int             stack_size;
  bytes_t         code;
  uint32_t        pos;
  evm_state_t     state;
  bytes_t         last_returned;
  bytes_t         return_data;
  uint32_t*       invalid_jumpdest;

  // set properties as to which EIPs to use.
  uint32_t properties;

  // define the enviroment-function.
  evm_get_env env;
  void*       env_ptr;
  uint64_t    chain_id; /**< the chain_id as returned by the opcode */

  // tx values
  uint8_t* address;    /**< the address of the current storage*/
  uint8_t* account;    /**< the address of the code */
  uint8_t* origin;     /**< the address of original sender of the root-transaction */
  uint8_t* caller;     /**< the address of the parent sender */
  bytes_t  call_value; /**< value send */
  bytes_t  call_data;  /**< data send in the tx */
  bytes_t  gas_price;  /**< current gasprice */
  uint64_t gas;
  gas_options;

} evm_t;

int evm_stack_push(evm_t* evm, uint8_t* data, uint8_t len);
int evm_stack_push_ref(evm_t* evm, uint8_t** dst, uint8_t len);
int evm_stack_push_int(evm_t* evm, uint32_t val);
int evm_stack_push_long(evm_t* evm, uint64_t val);

int     evm_stack_get_ref(evm_t* evm, uint8_t pos, uint8_t** dst);
int     evm_stack_pop(evm_t* evm, uint8_t* dst, uint8_t len);
int     evm_stack_pop_ref(evm_t* evm, uint8_t** dst);
int     evm_stack_pop_byte(evm_t* evm, uint8_t* dst);
int32_t evm_stack_pop_int(evm_t* evm);
int     evm_stack_peek_len(evm_t* evm);

int evm_run(evm_t* evm, address_t code_address);
#define EVM_CALL_MODE_STATIC 1
#define EVM_CALL_MODE_DELEGATE 2
#define EVM_CALL_MODE_CALLCODE 3
#define EVM_CALL_MODE_CALL 4

int evm_sub_call(evm_t*   parent,
                 uint8_t  address[20],
                 uint8_t  account[20],
                 uint8_t* value, wlen_t l_value,
                 uint8_t* data, uint32_t l_data,
                 uint8_t  caller[20],
                 uint8_t  origin[20],
                 uint64_t gas,
                 wlen_t   mode,
                 uint32_t out_offset, uint32_t out_len);

int  evm_ensure_memory(evm_t* evm, uint32_t max_pos);
int  in3_get_env(void* evm_ptr, uint16_t evm_key, uint8_t* in_data, int in_len, uint8_t** out_data, int offset, int len);
int  evm_call(void*    vc,
              uint8_t  address[20],
              uint8_t* value, wlen_t l_value,
              uint8_t* data, uint32_t l_data,
              uint8_t   caller[20],
              uint64_t  gas,
              uint64_t  chain_id,
              bytes_t** result);
void evm_print_stack(evm_t* evm, uint64_t last_gas, uint32_t pos);
void evm_free(evm_t* evm);

int evm_execute(evm_t* evm);

int evm_run(evm_t* evm, address_t code_address);

#ifdef EVM_GAS
account_t* evm_get_account(evm_t* evm, uint8_t adr[20], wlen_t create);
storage_t* evm_get_storage(evm_t* evm, uint8_t adr[20], uint8_t* key, wlen_t keylen, wlen_t create);
int        transfer_value(evm_t* evm, uint8_t from_account[20], uint8_t to_account[20], uint8_t* value, wlen_t value_len, uint32_t base_gas);

#endif
#endif