#ifndef _BTC_MERKLE_H
#define _BTC_MERKLE_H

#include "../../core/util/bytes.h"
#include "../../core/util/error.h"
#include <stdint.h>

in3_ret_t btc_merkle_create_root(bytes32_t* hashes, int hashes_len, bytes32_t dst);
bytes_t*  btc_merkle_create_proof(bytes32_t* hashes, int hashes_len, int index);
int       btc_merkle_verify_proof(bytes32_t root_hash, bytes_t proof, int index, bytes32_t start_hash);

#endif // _BTC_MERKLE_H