
#include <util/bytes.h>

typedef enum evm_state {
  EVM_STATE_INIT    = 0,
  EVM_STATE_RUNNING = 1,
  EVM_STATE_STOPPED = 2
} evm_state_t;

#define EVM_ERROR_EMPTY_STACK -1
#define EVM_ERROR_INVALID_OPCODE -2
#define EVM_ERROR_BUFFER_TOO_SMALL -3

#define EVM_EIP_CONSTANTINOPL 1

typedef struct evm {
  bytes_builder_t stack;
  bytes_builder_t memory;
  int             stack_size;
  uint8_t*        code;
  int             code_len;
  evm_state_t     state;
  int             pos;
  uint32_t        properties;

} evm_t;

int evm_stack_push(evm_t* evm, uint8_t* data, uint8_t len);
int evm_stack_push_bn(evm_t* evm, bignum256* val);

int evm_stack_pop(evm_t* evm, uint8_t* dst, uint8_t len);
int evm_stack_pop_bn(evm_t* evm, bignum256* dst);
int evm_stack_pop_ref(evm_t* evm, uint8_t** dst);
int evm_stack_pop_byte(evm_t* evm, uint8_t* dst);
