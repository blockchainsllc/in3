/** @file 
 * Ethereum Nanon verification.
 * */

#ifndef in3_eth_basic_h__
#define in3_eth_basic_h__

#include "../core/client/verifier.h"

/** entry-function to execute the verification context. */
int in3_verify_eth_basic(in3_vctx_t* v);
/**
 * verifies internal tx-values.
 */
int eth_verify_tx_values(in3_vctx_t* vc, d_token_t* tx, bytes_t* raw);

/**
 * verifies a transaction.
 */
int eth_verify_eth_getTransaction(in3_vctx_t* vc, bytes_t* tx_hash);

/**
 * verify account-proofs
 */
int eth_verify_account_proof(in3_vctx_t* vc);

/**
 * verifies a block
 */
int eth_verify_eth_getBlock(in3_vctx_t* vc, bytes_t* block_hash, uint64_t blockNumber);

/**
 * this function should only be called once and will register the eth-nano verifier.
 */
void in3_register_eth_basic();

#endif // in3_eth_basic_h__