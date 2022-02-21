#include "btc_merkle.h"
#include "../../core/util/crypto.h"
#include "../../core/util/mem.h"
#include "btc_serialize.h"
#include <stdint.h>
#include <string.h>

// creates the parent hash by calling this function recursivly until we end up with only one root hash.
static void create_parent_hashes(uint8_t* hashes, int hashes_len) {
  int      res_count = (hashes_len + 1) >> 1;
  uint8_t* dst;
  for (int i = 0, j = 0; i < res_count; i++, j += 64) {
    dst            = hashes + (i << 5);
    in3_digest_t d = crypto_create_hash(DIGEST_SHA256_BTC);
    crypto_update_hash(d, bytes(hashes + j, 32));
    crypto_update_hash(d, bytes(hashes + (((i << 1) + 1) == hashes_len ? j : j + 32), 32));
    crypto_finalize_hash(d, dst);
  }
  if (res_count > 1) create_parent_hashes(hashes, res_count);
}

in3_ret_t btc_merkle_create_root(bytes32_t* hashes, int hashes_len, bytes32_t dst) {
  uint8_t* tmp = _malloc(hashes_len << 5); // we create an byte array with hashes_len*32 to store all hashes
  if (hashes_len == 0)                     // emptyList = NULL hash
    memset(dst, 0, 32);
  else {
    for (int i = 0; i < hashes_len; i++) rev_copy(tmp + (i << 5), hashes[i]); // copy the hashes in reverse order into the buffer
    create_parent_hashes(tmp, hashes_len);                                    // reduce the roothash until we have only one left.
    rev_copy(dst, tmp);                                                       // the first hash in the buffer is the root hash, which copy reverse again.
  }
  _free(tmp);
  return IN3_OK; // no way to create an exception here.
}

bool btc_merkle_verify_proof(bytes32_t target, bytes_t proof, int index, bytes32_t start_hash) {
  bytes32_t hash;
  rev_copy(hash, start_hash);

  for (uint8_t* p = proof.data; proof.len; index = index >> 1, p += 32, proof.len -= 32) {
    if (memcmp(target, hash, 32) == 0) return true;
    in3_digest_t d = crypto_create_hash(DIGEST_SHA256_BTC);
    crypto_update_hash(d, bytes(index % 2 ? p : hash, 32));
    crypto_update_hash(d, bytes(index % 2 ? hash : p, 32));
    crypto_finalize_hash(d, hash);
  }
  return memcmp(target, hash, 32) == 0;
}
