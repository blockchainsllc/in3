#include "gas.h"





#ifdef EVM_GAS
void init_gas(evm_t *evm) {
    // prepare evm gas
    evm->refund = 0;
    if (!evm->init_gas) evm->init_gas = evm->gas;
}


 void evm_init(evm_t *evm) {
    evm->accounts = NULL;
    evm->gas = 0;
    evm->logs = NULL;
    evm->parent = NULL;
    evm->refund = 0;
    evm->init_gas = 0;
}


 void finalize_and_refund_gas(evm_t *evm) {
    uint64_t gas_used = evm->init_gas - evm->gas;
    if ((evm->properties & EVM_PROP_NO_FINALIZE) == 0) {
        // finalize and refund
        if (evm->refund && evm->parent) {
            evm->parent->gas -= gas_used;
            evm->gas += gas_used + evm->refund;
        } else {
            evm->gas += min(evm->refund, gas_used >> 1);
        }
    }
}

 void finalize_subcall_gas(evm_t evm, int success, evm_t *parent) {
    // if it was successfull we copy the new state to the parent
    if (success == 0 && evm.state != EVM_STATE_REVERTED)
        copy_state(parent, &evm);
    // if we have gas left and it was successfull we returen it to the parent process.
    if (success == 0) parent->gas += evm.gas;

}
account_t * evm_create_account(evm_t* evm, uint8_t* data, uint32_t l_data, address_t code_address, address_t caller){

    account_t* new_account      = NULL;
    new_account = evm_get_account(evm, code_address, 1);
    // this is a create-call
    evm->code               = bytes(data, l_data);
    evm->call_data.len      = 0;
    evm->address            = code_address;
    new_account->nonce[31] = 1;

    // increment the nonce of the sender
    account_t* sender_account = evm_get_account(evm, caller, 1);
    bytes32_t  new_nonce;
    uint8_t    one = 1;
    uint256_set(new_nonce, big_add(sender_account->nonce, 32, &one, 1, new_nonce, 32), sender_account->nonce);
    return new_account;
}
 void
update_gas(evm_t* evm, int *res, evm_t *parent, address_t address, address_t code_address, address_t caller, uint64_t gas,
           wlen_t mode) {
    evm->parent = parent;
     account_t* new_account      = NULL;

    uint64_t max_gas_provided = parent->gas - (parent->gas >> 6);

    if (!address) {
        new_account = evm_create_account(evm, evm->call_data.data, evm->call_data.len, code_address, caller);
        // handle gas
        gas = max_gas_provided;
    } else
        gas = min(gas, max_gas_provided);

    // give the call the amount of gas
    evm->gas = gas;
    evm->gas_price.data = parent->gas_price.data;
    evm->gas_price.len = parent->gas_price.len;

    // and try to transfer the value
    if (res == 0 && !big_is_zero(evm->call_value.data, evm->call_value.len)) {
        // if we have a value and this should be we throw
        if (mode == EVM_CALL_MODE_STATIC)
            *res = EVM_ERROR_UNSUPPORTED_CALL_OPCODE;
        else {
            // only for CALL or CALLCODE we add the CALLSTIPEND
            uint32_t gas_call_value = 0;
            if (mode == EVM_CALL_MODE_CALL || mode == EVM_CALL_MODE_CALLCODE) {
                evm->gas += G_CALLSTIPEND;
                gas_call_value = G_CALLVALUE;
            }
            *res = transfer_value(evm, parent->address, evm->address, evm->call_value.data, evm->call_value.len, gas_call_value);
        }
    }
    if (res == 0) {
        // if we don't even have enough gas
        if (parent->gas < gas)
            *res = EVM_ERROR_OUT_OF_GAS;
        else
            parent->gas -= gas;
    }
}

 int selfdestruct_gas(evm_t *evm) {
    uint8_t adr[20], l, *p;
    if (evm_stack_pop(evm, adr, 20) < 0) return EVM_ERROR_EMPTY_STACK;
    account_t *self_account = evm_get_account(evm, evm->address, 1);
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
    storage_t *s = NULL;
    while (self_account->storage) {
        s = self_account->storage;
        self_account->storage = s->next;
        _free(s);
    }
    evm->state = EVM_STATE_STOPPED;
    return 0;
}

#endif