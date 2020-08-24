
#include "../../core/util/bytes.h"
#include "../../core/util/error.h"
#ifndef __ZK_CRYPTO_H__
#define __ZK_CRYPTO_H__

void zkcrypto_initialize();

in3_ret_t zkcrypto_pk_from_seed(bytes_t seed, bytes32_t dst);
in3_ret_t zkcrypto_pk_to_pubkey(bytes32_t pk, uint8_t* dst);
in3_ret_t zkcrypto_sign_musig(bytes32_t pk, bytes_t msg, uint8_t* dst);

#endif