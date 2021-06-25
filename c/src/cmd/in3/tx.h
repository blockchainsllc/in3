#include "../../api/eth1/abi.h"
#include "helper.h"
typedef struct tx {
  bytes_t*   data;
  char*      block;
  char*      from;
  char*      to;
  uint64_t   gas;
  uint64_t   gas_price;
  uint64_t   nonce;
  char*      value;
  uint32_t   wait;
  char*      signtype;
  char*      sig;
  abi_sig_t* abi_sig;
  char*      token;

} tx_t;

tx_t* get_txdata();
void  encode_abi(in3_t* c, sb_t* args, bool with_block);