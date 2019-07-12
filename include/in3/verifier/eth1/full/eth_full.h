// @PUBLIC_HEADER
/** @file
 * Ethereum Nanon verification.
 * */

#ifndef in3_eth_full_h__
#define in3_eth_full_h__

#include "../../../core/client/verifier.h"

/** entry-function to execute the verification context. */
int in3_verify_eth_full(in3_vctx_t* v);

/**
 * this function should only be called once and will register the eth-full verifier.
 */
void in3_register_eth_full();

#endif // in3_eth_full_h__