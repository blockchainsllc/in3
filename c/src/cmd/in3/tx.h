#include "helper.h"

typedef struct tx {
  bytes_t* data;
  char*    block;
  char*    from;
  char*    to;
  uint64_t gas;
  uint64_t gas_price;
  uint64_t nonce;
  char*    value;
  uint32_t wait;
  char*    signtype;
  char*    sig;

} tx_t;

tx_t* get_txdata();