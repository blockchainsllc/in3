#include "btc_merkle.h"
#include "../../core/util/mem.h"
#include "../../third-party/crypto/sha2.h"
#include "btc_serialize.h"
#include <stdint.h>
#include <string.h>

// creates the root hash by calling this function recursivly until we end up with only one root hash.
static void create_proofs(uint8_t* hashes, int hashes_len, SHA256_CTX* ctx, bytes_builder_t* bb, int index) {
  int      res_count = (hashes_len + 1) >> 1;
  uint8_t* dst;
  for (int i = 0, j = 0; i < res_count; i++, j += 64) {
    if (bb && index >> 1 == i) bb_write_raw_bytes(bb, hashes + (i << 1 == index ? (hashes_len == index + 1 ? j : j + 32) : j), 32);

    dst = hashes + (i << 5);
    sha256_Init(ctx);
    sha256_Update(ctx, hashes + j, 32);
    sha256_Update(ctx, hashes + (((i << 1) + 1) == hashes_len ? j : j + 32), 32);
    sha256_Final(ctx, dst);
    sha256_Init(ctx);
    sha256_Update(ctx, dst, 32);
    sha256_Final(ctx, dst);
  }
  if (res_count > 1) create_proofs(hashes, res_count, ctx, bb, index >> 1);
}

in3_ret_t btc_merkle_create_root(bytes32_t* hashes, int hashes_len, bytes32_t dst) {
  uint8_t*   tmp = alloca(hashes_len << 5); // we create an byte array with hashes_len*32 to store all hashes
  SHA256_CTX ctx;                           // we want to reuse the struct later
  if (hashes_len == 0) {                    // emptyList = NULL hash
    memset(dst, 0, 32);
    return IN3_OK;
  }
  for (int i = 0; i < hashes_len; i++) rev_copy(tmp + (i << 5), hashes[i]); // copy the hashes in reverse order into the buffer
  create_proofs(tmp, hashes_len, &ctx, NULL, -1);                           // reduce the roothash until we have only one left.
  rev_copy(dst, tmp);                                                       // the first hash in the buffer is the root hash, which copy reverse again.
  return IN3_OK;                                                            // no way to create an exception here.
}

bytes_t* btc_merkle_create_proof(bytes32_t* hashes, int hashes_len, int index) {
  uint8_t*   tmp = alloca(hashes_len << 5); // we create an byte array with hashes_len*32 to store all hashes
  SHA256_CTX ctx;                           // we want to reuse the struct later
  if (hashes_len == 0 || index >= hashes_len) return NULL;
  bytes_builder_t* bb = bb_new();
  for (int i = 0; i < hashes_len; i++) rev_copy(tmp + (i << 5), hashes[i]); // copy the hashes in reverse order into the buffer
  create_proofs(tmp, hashes_len, &ctx, bb, index);                          // reduce the roothash until we have only one left.
  return bb_move_to_bytes(bb);
}

int btc_merkle_verify_proof(bytes32_t root_hash, bytes_t proof, int index, bytes32_t start_hash) {
  SHA256_CTX ctx; // we want to reuse the struct later
  bytes32_t  hash, target;
  rev_copy(hash, start_hash);
  rev_copy(target, root_hash);
  uint8_t* p = proof.data;
  for (; proof.len; index = index >> 1, p += 32, proof.len -= 32) {
    if (memcmp(target, hash, 32) == 0) return 1;
    sha256_Init(&ctx);
    sha256_Update(&ctx, index % 2 ? p : hash, 32);
    sha256_Update(&ctx, index % 2 ? hash : p, 32);
    sha256_Final(&ctx, hash);
    sha256_Init(&ctx);
    sha256_Update(&ctx, hash, 32);
    sha256_Final(&ctx, hash);
  }
  return memcmp(target, hash, 32) == 0 ? 1 : 0;
}
