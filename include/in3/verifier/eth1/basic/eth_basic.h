// @PUBLIC_HEADER
/** @file
 * Ethereum Nanon verification.
 * */

#ifndef in3_eth_basic_h__
#define in3_eth_basic_h__

#include "../../../core/client/verifier.h"

/** entry-function to execute the verification context. */
in3_ret_t in3_verify_eth_basic(in3_vctx_t* v);
/**
 * verifies internal tx-values.
 */
in3_ret_t eth_verify_tx_values(in3_vctx_t* vc, d_token_t* tx, bytes_t* raw);

/**
 * verifies a transaction.
 */
in3_ret_t eth_verify_eth_getTransaction(in3_vctx_t* vc, bytes_t* tx_hash);

/**
 * verify account-proofs
 */
in3_ret_t eth_verify_account_proof(in3_vctx_t* vc);

/**
 * verifies a block
 */
in3_ret_t eth_verify_eth_getBlock(in3_vctx_t* vc, bytes_t* block_hash, uint64_t blockNumber);

/**
 * this function should only be called once and will register the eth-nano verifier.
 */
void in3_register_eth_basic();

/**
 *  verify logs
 */
in3_ret_t eth_verify_eth_getLog(in3_vctx_t* vc, int l_logs);

/**
 * this is called before a request is send
 */
in3_ret_t eth_handle_intern(in3_ctx_t* ctx, in3_response_t** response);

#endif // in3_eth_basic_h__