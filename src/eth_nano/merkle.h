/** @file 
 * Merkle Proof Verification.
 * */ 

#include <util/bytes.h>


#ifndef MERKLE_H
#define MERKLE_H

int verifyMerkleProof(bytes_t* rootHash, bytes_t* path, bytes_t** proof, bytes_t* expectedValue) ;

uint8_t* str_to_nibbles(bytes_t *path, int use_prefix) ;
int matching_nibbles(uint8_t *a,uint8_t *b  );

void free_proof(bytes_t** proof);
#endif
