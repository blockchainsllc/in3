#include "btc_merkle.h"
#include "../../third-party/crypto/sha2.h"
#include "btc_serialize.h"
#include <stdint.h>
#include <string.h>

// creates the root hash by calling this function recursivly until we end up with only one root hash.
static void create_roots(uint8_t* hashes, int hashes_len, SHA256_CTX* ctx) {
  int      res_count = (hashes_len + 1) >> 1;
  uint8_t* dst;
  for (int i = 0, j = 0; i < res_count; i++, j += 64) {
    dst = hashes + (i << 5);
    sha256_Init(ctx);
    sha256_Update(ctx, hashes + j, 32);
    sha256_Update(ctx, hashes + (((i << 1) + 1) == hashes_len ? j : j + 32), 32);
    sha256_Final(ctx, dst);
    sha256_Init(ctx);
    sha256_Update(ctx, dst, 32);
    sha256_Final(ctx, dst);
  }
  if (res_count > 1) create_roots(hashes, res_count, ctx);
}

in3_ret_t btc_merkle_create_root(bytes32_t* hashes, int hashes_len, bytes32_t dst) {
  uint8_t    tmp[hashes_len << 5]; // we create an byte array with hashes_len*32 to store all hashes
  SHA256_CTX ctx;                  // we want to reuse the struct later
  if (hashes_len == 0) {           // emptyList = NULL hash
    memset(dst, 0, 32);
    return IN3_OK;
  }
  for (int i = 0; i < hashes_len; i++) rev_copy(tmp + (i << 5), hashes[i]); // copy the hashes in reverse order into the buffer
  create_roots(tmp, hashes_len, &ctx);                                      // reduce the roothash until we have only one left.
  rev_copy(dst, tmp);                                                       // the first hash in the buffer is the root hash, which copy reverse again.
  return IN3_OK;                                                            // no way to create an exception here.
}
