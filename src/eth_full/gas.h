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
#define op_exec(m, g) \
  {                   \
    evm->gas -= (g);  \
    return m;         \
  }
#else
#define op_exec(m, gas) return m;
#define subgas(g)
#endif

#define GAS_CC_NET_SSTORE_NOOP_GAS 200             // Once per SSTORE operation if the value doesn't change
#define GAS_CC_NET_SSTORE_INIT_GAS 20000           // Once per SSTORE operation from clean zero
#define GAS_CC_NET_SSTORE_CLEAN_GAS 5000           // Once per SSTORE operation from clean non-zero
#define GAS_CC_NET_SSTORE_DIRTY_GAS 200            // Once per SSTORE operation from dirty
#define GAS_CC_NET_SSTORE_CLEAR_REFUND 15000       // Once per SSTORE operation for clearing an originally existing storage slot
#define GAS_CC_NET_SSTORE_RESET_REFUND 4800        // Once per SSTORE operation for resetting to the original non-zero value
#define GAS_CC_NET_SSTORE_RESET_CLEAR_REFUND 19800 // Once per SSTORE operation for resetting to the original zero value

#define G_ZERO 0             // Nothing is paid for operations of the set Wzero.
#define G_JUMPDEST 1         // JUMP DEST
#define G_BASE 2             // This is the amount of gas to pay for operations of the set Wbase.
#define G_VERY_LOW 3         // This is the amount of gas to pay for operations of the set Wverylow.
#define G_LOW 5              // This is the amount of gas to pay for operations of the set Wlow.
#define G_MID 8              // This is the amount of gas to pay for operations of the set Wmid.
#define G_HIGH 10            // This is the amount of gas to pay for operations of the set Whigh.
#define G_EXTCODE 700        //  This is the amount of gas to pay for operations of the set Wextcode.
#define G_BALANCE 400        // This is the amount of gas to pay for a BALANCE operation.
#define G_SLOAD 50           // This is paid for an SLOAD operation.
#define G_SSET 20000         // This is paid for an SSTORE operation when the storage value is set to non-zero from zero.
#define G_SRESET 5000        // This is the amount for an SSTORE operation when the storage value’s zeroness remains unchanged or is set to zero.
#define R_SCLEAR 15000       // This is the refund given (added into the refund counter) when the storage value is set to zero from non-zero.
#define R_SELFDESTRUCT 24000 // This is the refund given (added into the refund counter) for self-destructing an account.
#define G_SELFDESTRUCT 5000  // This is the amount of gas to pay for a SELFDESTRUCT operation.
#define G_CREATE 32000       // This is paid for a CREATE operation.
#define G_CODEDEPOSIT 200    // This is paid per byte for a CREATE operation to succeed in placing code into the state.
#define G_CALL 700           //  This is paid for a CALL operation.
#define G_CALLVALUE 9000     // This is paid for a non-zero value transfer as part of the CALL operation.
#define G_CALLSTIPEND 2300   // This is a stipend for the called contract subtracted from Gcallvalue for a non-zero value transfer.
#define G_NEWACCOUNT 25000   // This is paid for a CALL or for a SELFDESTRUCT operation which creates an account.
#define G_EXP 10             // This is a partial payment for an EXP operation.
#define G_EXPBYTE 10         // This is a partial payment when multiplied by dlog256(exponent)e for the EXP operation.
#define G_MEMORY 3           // This is paid for every additional word when expanding memory.
#define G_TXCREATE 32000     // This is paid by all contract-creating transactions after the Homestead transition
#define G_TXDATA_ZERO 4      // This is paid for every zero byte of data or code for a transaction.
#define G_TXDATA_NONZERO 68  // This is paid for every non-zero byte of data or code for a transaction
#define G_TRANSACTION 21000  // This is paid for every transaction.
#define G_LOG 375            // This is a partial payment for a LOG operation.
#define G_LOGDATA 8          // This is paid for each byte in a LOG operation’s data.
#define G_LOGTOPIC 375       // This is paid for each topic of a LOG operation.
#define G_SHA3 30            // This is paid for each SHA3 operation.
#define G_SHA3WORD 6         // This is paid for each word (rounded up) for input data to a SHA3 operation.
#define G_COPY 3             // This is a partial payment for *COPY operations, multiplied by the number of words copied, rounded up.
#define G_BLOCKHASH 20       // This is a payment for a BLOCKHASH operation.
