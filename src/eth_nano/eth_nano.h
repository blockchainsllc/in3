/** @file 
 * Ethereum Nanon verification.
 * */ 

#ifndef in3_eth_nano_h__
#define in3_eth_nano_h__

#include "../core/client/verifier.h"

/** entry-function to execute the verification context. */
int in3_verify_eth_nano( in3_vctx_t* v);

/** verifies a blockheader. */
int eth_verify_blockheader( in3_vctx_t* vc, bytes_t* header, bytes_t* expected_blockhash);

/** 
 * verifies a single signature blockheader.
 * 
 * This function will return a positive integer with a bitmask holding the bit set according to the address that signed it. 
 * This is based on the signatiures in the request-config.
 * 
 */
int eth_verify_signature(in3_vctx_t *vc, bytes_t *msg_hash, d_token_t *sig);

/**
 *  returns the address of the signature if the msg_hash is correct
 */
bytes_t* ecrecover_signature(bytes_t *msg_hash, d_token_t *sig);

/**
 * verifies a transaction receipt.
 */
int eth_verify_eth_getTransactionReceipt(in3_vctx_t *vc, bytes_t* tx_hash);

/**
 * this function should only be called once and will register the eth-nano verifier.
 */
void in3_register_eth_nano();

/**
 * helper function to rlp-encode the transaction_index.
 * 
 * The result must be freed after use!
 */
bytes_t* create_tx_path(uint32_t index);
#endif  // in3_eth_nano_h__