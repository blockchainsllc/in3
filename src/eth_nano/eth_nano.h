#ifndef in3_eth_nano_h__
#define in3_eth_nano_h__

#include "../core/client/verifier.h"

int in3_verify_eth_nano( in3_vctx_t* v);


int eth_verify_blockheader( in3_vctx_t* vc, bytes_t* header, jsmntok_t* expected_blockhash);


void in3_register_eth_nano();

#endif  // in3_eth_nano_h__