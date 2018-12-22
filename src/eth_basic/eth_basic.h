/** @file 
 * Ethereum Nanon verification.
 * */ 

#ifndef in3_eth_basic_h__
#define in3_eth_basic_h__

#include "../core/client/verifier.h"

/** entry-function to execute the verification context. */
int in3_verify_eth_basic( in3_vctx_t* v);

/**
 * verifies a transaction.
 */
int eth_verify_eth_getTransaction(in3_vctx_t *vc, jsmntok_t *tx_hash);

/**
 * this function should only be called once and will register the eth-nano verifier.
 */
void in3_register_eth_basic();

#endif  // in3_eth_basic_h__