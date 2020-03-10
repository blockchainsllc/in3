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
 * evm gas defines.
 * */

#include "../../../core/util/mem.h"
#include "../../../core/util/utils.h"
#include "accounts.h"
#include "big.h"
#include "evm.h"

#ifdef EVM_GAS
#define subgas(g)                  \
  {                                \
    uint64_t gas = (g);            \
    if (evm->gas < gas)            \
      return EVM_ERROR_OUT_OF_GAS; \
    else                           \
      evm->gas -= gas;             \
  }
#define op_exec(m, gc)   \
  {                      \
    subgas(gc) return m; \
  }
#else
#define op_exec(m, gas) return m;
#define subgas(g)
#endif

#define GAS_CC_NET_SSTORE_NOOP_GAS 200             /**< Once per SSTORE operation if the value doesn't change*/
#define GAS_CC_NET_SSTORE_INIT_GAS 20000           /**< Once per SSTORE operation from clean zero*/
#define GAS_CC_NET_SSTORE_CLEAN_GAS 5000           /**< Once per SSTORE operation from clean non-zero*/
#define GAS_CC_NET_SSTORE_DIRTY_GAS 200            /**< Once per SSTORE operation from dirty*/
#define GAS_CC_NET_SSTORE_CLEAR_REFUND 15000       /**< Once per SSTORE operation for clearing an originally existing storage slot*/
#define GAS_CC_NET_SSTORE_RESET_REFUND 4800        /**< Once per SSTORE operation for resetting to the original non-zero value*/
#define GAS_CC_NET_SSTORE_RESET_CLEAR_REFUND 19800 /**< Once per SSTORE operation for resetting to the original zero valuev*/

#define G_ZERO 0             /**< Nothing is paid for operations of the set Wzero.*/
#define G_JUMPDEST 1         /**< JUMP DEST*/
#define G_BASE 2             /**< This is the amount of gas to pay for operations of the set Wbase.*/
#define G_VERY_LOW 3         /**< This is the amount of gas to pay for operations of the set Wverylow.*/
#define G_LOW 5              /**< This is the amount of gas to pay for operations of the set Wlow.*/
#define G_MID 8              /**< This is the amount of gas to pay for operations of the set Wmid.*/
#define G_HIGH 10            /**< This is the amount of gas to pay for operations of the set Whigh.*/
#define G_EXTCODE 700        /**<  This is the amount of gas to pay for operations of the set Wextcode.*/
#define G_BALANCE 400        /**< This is the amount of gas to pay for a BALANCE operation.*/
#define G_SLOAD 200          /**< This is paid for an SLOAD operation.*/
#define G_SSET 20000         /**< This is paid for an SSTORE operation when the storage value is set to non-zero from zero.*/
#define G_SRESET 5000        /**< This is the amount for an SSTORE operation when the storage value's zeroness remains unchanged or is set to zero.*/
#define R_SCLEAR 15000       /**< This is the refund given (added into the refund counter) when the storage value is set to zero from non-zero.*/
#define R_SELFDESTRUCT 24000 /**< This is the refund given (added into the refund counter) for self-destructing an account.*/
#define G_SELFDESTRUCT 5000  /**< This is the amount of gas to pay for a SELFDESTRUCT operation.*/
#define G_CREATE 32000       /**< This is paid for a CREATE operation.*/
#define G_CODEDEPOSIT 200    /**< This is paid per byte for a CREATE operation to succeed in placing code into the state.*/
#define G_CALL 700           /**<  This is paid for a CALL operation.*/
#define G_CALLVALUE 9000     /**< This is paid for a non-zero value transfer as part of the CALL operation.*/
#define G_CALLSTIPEND 2300   /**< This is a stipend for the called contract subtracted from Gcallvalue for a non-zero value transfer.*/
#define G_NEWACCOUNT 25000   /**< This is paid for a CALL or for a SELFDESTRUCT operation which creates an account.*/
#define G_EXP 10             /**< This is a partial payment for an EXP operation.*/
#define G_EXPBYTE 50         /**< This is a partial payment when multiplied by dlog256(exponent)e for the EXP operation.*/
#define G_MEMORY 3           /**< This is paid for every additional word when expanding memory.*/
#define G_TXCREATE 32000     /**< This is paid by all contract-creating transactions after the Homestead transition*/
#define G_TXDATA_ZERO 4      /**< This is paid for every zero byte of data or code for a transaction.*/
#define G_TXDATA_NONZERO 68  /**< This is paid for every non-zero byte of data or code for a transaction*/
#define G_TRANSACTION 21000  /**< This is paid for every transaction.*/
#define G_LOG 375            /**< This is a partial payment for a LOG operation.*/
#define G_LOGDATA 8          /**< This is paid for each byte in a LOG operation's data.*/
#define G_LOGTOPIC 375       /**< This is paid for each topic of a LOG operation.*/
#define G_SHA3 30            /**< This is paid for each SHA3 operation.*/
#define G_SHA3WORD 6         /**< This is paid for each word (rounded up) for input data to a SHA3 operation.*/
#define G_COPY 3             /**< This is a partial payment for *COPY operations, multiplied by the number of words copied, rounded up.*/
#define G_BLOCKHASH 20       /**< This is a payment for a BLOCKHASH operation.*/

#define G_PRE_EC_RECOVER 3000         /**< Precompile EC RECOVER*/
#define G_PRE_SHA256 60               /**< Precompile SHA256*/
#define G_PRE_SHA256_WORD 12          /**< Precompile SHA256 per word*/
#define G_PRE_RIPEMD160 600           /**< Precompile RIPEMD160*/
#define G_PRE_RIPEMD160_WORD 120      /**< Precompile RIPEMD160 per word*/
#define G_PRE_IDENTITY 15             /**< Precompile IDENTIY (copyies data)*/
#define G_PRE_IDENTITY_WORD 3         /**< Precompile IDENTIY per word*/
#define G_PRE_MODEXP_GQUAD_DIVISOR 20 /**< Gquaddivisor from modexp precompile for gas calculation*/
#define G_PRE_ECADD 500               /**< Gas costs for curve addition precompile*/
#define G_PRE_ECMUL 40000             /**< Gas costs for curve multiplication precompile*/
#define G_PRE_ECPAIRING 100000        /**< Base gas costs for curve pairing precompile*/
#define G_PRE_ECPAIRING_WORD 80000    /**< Gas costs regarding curve pairing precompile input length*/

#define EVM_STACK_LIMIT 1024    /**< max elements of the stack*/
#define EVM_MAX_CODE_SIZE 24576 /**< max size of the code*/

///  fork values
#define FRONTIER_G_EXPBYTE 10 /**< This is a partial payment when multiplied by dlog256(exponent)e for the EXP operation.*/
#define FRONTIER_G_SLOAD 50   /**< This is a partial payment when multiplied by dlog256(exponent)e for the EXP operation.*/

#ifdef EVM_GAS
void init_gas(evm_t* evm);
void evm_init(evm_t* evm);
void finalize_and_refund_gas(evm_t* evm);
void finalize_subcall_gas(evm_t* evm, int success, evm_t* parent);

void update_gas(evm_t* evm, int* res, evm_t* parent, address_t address, address_t code_address, address_t caller, uint64_t gas,
                wlen_t mode, uint8_t* value, wlen_t l_value);
void update_account_code(evm_t* evm, account_t* new_account);
#endif

#ifdef EVM_GAS
#define FREE_EVM(evm) evm_free(evm)
#define INIT_EVM(evm) evm_init(evm)
#define INIT_GAS(evm) init_gas(evm)
#define SUBGAS(evm, g) subgas(evm, g)
#define KEEP_TRACK_GAS(evm) evm->gas
#define FINALIZE_SUBCALL_GAS(evm, success, parent) finalize_subcall_gas(evm, success, parent)
#define UPDATE_SUBCALL_GAS(evm, parent, address, code_address, caller, gas, mode, value, l_value)          \
  do {                                                                                                     \
    evm.parent                = parent;                                                                    \
    uint64_t max_gas_provided = parent->gas - (parent->gas >> 6);                                          \
    if (!address) {                                                                                        \
      new_account = evm_create_account(&evm, evm.call_data.data, evm.call_data.len, code_address, caller); \
      gas         = max_gas_provided;                                                                      \
    } else                                                                                                 \
      gas = min(gas, max_gas_provided);                                                                    \
    evm.gas            = gas;                                                                              \
    evm.gas_price.data = parent->gas_price.data;                                                           \
    evm.gas_price.len  = parent->gas_price.len;                                                            \
    if (res == 0 && !big_is_zero(value, l_value)) {                                                        \
      if (mode == EVM_CALL_MODE_STATIC)                                                                    \
        res = EVM_ERROR_UNSUPPORTED_CALL_OPCODE;                                                           \
      else {                                                                                               \
        uint32_t gas_call_value = 0;                                                                       \
        if (mode == EVM_CALL_MODE_CALL || mode == EVM_CALL_MODE_CALLCODE) {                                \
          evm.gas += G_CALLSTIPEND;                                                                        \
          gas_call_value = G_CALLVALUE;                                                                    \
        }                                                                                                  \
        res = transfer_value(&evm, parent->address, evm.address, value, l_value, gas_call_value);          \
      }                                                                                                    \
    }                                                                                                      \
    if (res == 0) {                                                                                        \
      if (parent->gas < gas)                                                                               \
        res = EVM_ERROR_OUT_OF_GAS;                                                                        \
      else                                                                                                 \
        parent->gas -= gas;                                                                                \
    }                                                                                                      \
  } while (0)

#define FINALIZE_AND_REFUND_GAS(evm) finalize_and_refund_gas(evm)
#define UPDATE_ACCOUNT_CODE(evm, new_account) update_account_code(evm, new_account)
#else
#define FREE_EVM(...)
#define INIT_EVM(...)
#define INIT_GAS(...)
#define SUBGAS(...)
#define FINALIZE_SUBCALL_GAS(...)
#define UPDATE_SUBCALL_GAS(...)
#define FINALIZE_AND_REFUND_GAS(...)
#define KEEP_TRACK_GAS(evm) 0
#define UPDATE_ACCOUNT_CODE(...)
#endif