#include "evm.h"
#include <string.h>


/**
 * sets a variable value to 32byte word.
 */
void uint256_set(uint8_t* src, wlen_t src_len, uint8_t dst[32]);

/** reads a account from the enviroment. */
account_t* evm_get_account(evm_t* evm, address_t adr, wlen_t create);

/** get account storage */
storage_t* evm_get_storage(evm_t* evm, address_t adr, uint8_t* s_key, wlen_t s_key_len, wlen_t create);

/** copy state. */
void copy_state(evm_t* dst, evm_t* src);

int transfer_value(evm_t* current, address_t from_account, address_t to_account, uint8_t* value, wlen_t value_len, uint32_t base_gas);

account_t * evm_create_account(evm_t* evm, uint8_t* data, uint32_t l_data, address_t code_address, address_t caller);

