/** @file 
 * Merkle Proof Verification.
 * 
 * */ 

#include <util/bytes.h>


#ifndef MERKLE_H
#define MERKLE_H

/**
 *  verifies a merkle proof.
 * 
 * \param rootHash the expected root hash of the trie.
 * \param path the path the the value
 * \param proof a array of bytes of rlp encoded nodes. This array must be terminatzed with a NULL-Pointer.
 * \param expectedValue a byte-object with the raw value. If the data-pointer is NULL, it will be set to the last leaf and can be checked afterwards. 
 */
int trie_verify_proof(bytes_t* rootHash, bytes_t* path, bytes_t** proof, bytes_t* expectedValue) ;

/**
 * helper function split a path into 4-bit nibbles.
 * 
 * The result must be freed after use!
 * 
 * \param path the path of bytes.
 * \param use_prefix if true (or 1) the first byte of the path is interpreded as the leaf or extension marker.
 * 
 * \return the resulting bytes represent a 4bit-number each and are terminated with a 0xFF. 
 * 
 */
uint8_t* trie_path_to_nibbles(bytes_t *path, int use_prefix) ;

/**
 * helper function to find the number of nibbles matching both paths.
 */
int trie_matching_nibbles(uint8_t *a,uint8_t *b  );

/**
 * used to free the NULL-terminated proof-array.
 */
void trie_free_proof(bytes_t** proof);
#endif
