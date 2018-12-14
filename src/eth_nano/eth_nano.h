#ifndef in3_eth_nano_h__
#define in3_eth_nano_h__

#include "../core/client/verifier.h"

int in3_verify_eth_nano( in3_ctx_t* ctx , in3_chain_t* chain, jsmntok_t* request, in3_request_config_t* request_config, jsmntok_t* response);



int eth_verify_blockheader(  in3_ctx_t* ctx, in3_chain_t* chain,in3_request_config_t* request_config, bytes_t* header, jsmntok_t* expected_blockhash, jsmntok_t* proof);


void in3_register_eth_nano();

#endif  // in3_eth_nano_h__