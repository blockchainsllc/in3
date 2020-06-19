#ifndef _BTC_MERKLE_H
#define _BTC_MERKLE_H

#include "../../core/util/bytes.h"
#include "../../core/util/error.h"
#include <stdint.h>

/** 
 * creates the merkle root based on the given hashes
 */
in3_ret_t btc_merkle_create_root(
    bytes32_t* hashes,     /**< the hashes */
    int        hashes_len, /**< the number  of hashes */
    bytes32_t  dst         /**< the dst where to write the proof*/
);

/**
 * verify a merkle proof.
 * @returns true if successful
 */
bool btc_merkle_verify_proof(
    bytes32_t root_hash, /**< the expected root hash */
    bytes_t   proof,     /**< the proof-data */
    int       index,     /**< the expected inde x*/
    bytes32_t start_hash /**< the start hash */
);

#endif // _BTC_MERKLE_H
