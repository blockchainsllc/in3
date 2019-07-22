#ifndef in3_signer_priv_h__
#define in3_signer_priv_h__

#include "../../../core/client/context.h"

in3_ret_t eth_sign(void* pk, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst);

bytes_t sign_tx(d_token_t* tx, in3_ctx_t* ctx);

#endif
