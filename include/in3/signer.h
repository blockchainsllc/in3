// @PUBLIC_HEADER
/** @file
 * Ethereum Nanon verification.
 * */

#ifndef in3_signer_h__
#define in3_signer_h__

#include "context.h"

in3_ret_t eth_sign(void* pk, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst);

in3_ret_t eth_set_pk_signer(in3_t* in3, bytes32_t pk);

bytes_t sign_tx(d_token_t* tx, in3_ctx_t* ctx);

#endif
