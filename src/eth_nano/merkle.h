/** @file 
 * Merkle Proof Verification.
 * */ 

#include <util/bytes.h>


#ifndef MERKLE_H
#define MERKLE_H

int verifyMerkleProof(bytes_t* rootHash, bytes_t* path, bytes_t** proof, bytes_t* expectedValue) ;



void free_proof(bytes_t** proof);
#endif
