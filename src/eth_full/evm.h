
#include <util/bytes.h>

typedef enum evm_state {
  EVM_STATE_INIT    = 0,
  EVM_STATE_RUNNING = 1,
  EVM_STATE_STOPPED = 2
} evm_state_t;

#define EVM_ERROR_EMPTY_STACK -1
#define EVM_ERROR_INVALID_OPCODE -2
#define EVM_ERROR_BUFFER_TOO_SMALL -3
#define EVM_ERROR_ILLEGAL_MEMORY_ACCESS -4
#define EVM_ERROR_INVALID_JUMPDEST -5
#define EVM_ERROR_INVALID_PUSH -6
#define EVM_ERROR_UNSUPPORTED_CALL_OPCODE -7

#define EVM_EIP_CONSTANTINOPL 1

#define EVM_ENV_BALANCE 1
#define EVM_ENV_GASPRICE 2
#define EVM_ENV_CODE_SIZE 3
#define EVM_ENV_CODE_COPY 4
#define EVM_ENV_BLOCKHASH 5
#define EVM_ENV_STORAGE 6

typedef int (*evm_get_env)(void* evm, uint16_t evm_key, uint8_t* in_data, int in_len, uint8_t* out_data, int offset, int len);

typedef struct evm {
  bytes_builder_t stack;
  bytes_builder_t memory;
  int             stack_size;
  bytes_t         code;
  int             pos;
  evm_state_t     state;

  uint32_t    properties;
  uint8_t*    address;
  uint8_t*    account;
  uint8_t*    origin;
  uint8_t*    caller;
  bytes_t     call_value;
  bytes_t     call_data;
  bytes_t     gas_price;
  bytes_t*    block_header;
  bytes_t     last_returned;
  evm_get_env env;

} evm_t;

int evm_stack_push(evm_t* evm, uint8_t* data, uint8_t len);
int evm_stack_push_bn(evm_t* evm, bignum256* val);
int evm_stack_push_int(evm_t* evm, uint32_t val);

int     evm_stack_get_ref(evm_t* evm, uint8_t pos, uint8_t** dst);
int     evm_stack_pop(evm_t* evm, uint8_t* dst, uint8_t len);
int     evm_stack_pop_bn(evm_t* evm, bignum256* dst);
int     evm_stack_pop_ref(evm_t* evm, uint8_t** dst);
int     evm_stack_pop_byte(evm_t* evm, uint8_t* dst);
int32_t evm_stack_pop_int(evm_t* evm);
