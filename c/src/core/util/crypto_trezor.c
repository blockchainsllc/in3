#include "bytes.h"
#include "crypto.h"
#include "debug.h"
#include "mem.h"
#include "utils.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../third-party/crypto/sha2.h"
#include "../../third-party/crypto/sha3.h"

/** writes 32 bytes to the pointer. */
in3_ret_t keccak(bytes_t data, void* dst) {
#ifdef CRYPTO_LIB
  struct SHA3_CTX ctx;
  sha3_256_Init(&ctx);
  if (data.len) sha3_Update(&ctx, data.data, data.len);
  keccak_Final(&ctx, dst);
  return 0;
#else
  UNUSED_VAR(data);
  UNUSED_VAR(dst);
  return -1;
#endif
}

in3_digest_t crypto_create_hash(in3_digest_type_t type) {
  in3_digest_t d = {.ctx = NULL, .type = type};
  switch (type) {
    case DIGEST_KECCAK: {
      d.ctx = _calloc(1, sizeof(struct SHA3_CTX));
      sha3_256_Init(d.ctx);
      return d;
    }
    case DIGEST_SHA256:
    case DIGEST_SHA256_BTC: {
      d.ctx = _calloc(1, sizeof(SHA256_CTX));
      sha256_Init(d.ctx);
      return d;
    }
    default: return d;
  }
}
void crypto_update_hash(in3_digest_t digest, bytes_t data) {
  switch (digest.type) {
    case DIGEST_KECCAK: {
      if (data.len) sha3_Update(digest.ctx, data.data, data.len);
      return;
    }
    case DIGEST_SHA256:
    case DIGEST_SHA256_BTC: {
      if (data.len) sha256_Update(digest.ctx, data.data, data.len);
      return;
    }
    default: return;
  }
}
void crypto_finalize_hash(in3_digest_t digest, void* dst) {
  if (dst && digest.ctx) {
    switch (digest.type) {
      case DIGEST_KECCAK: {
        keccak_Final(digest.ctx, dst);
        return;
      }
      case DIGEST_SHA256:
      case DIGEST_SHA256_BTC: {
        if (digest.type == DIGEST_SHA256_BTC) {
          bytes32_t tmp;
          sha256_Final(digest.ctx, tmp);
          sha256_Init(digest.ctx);
          sha256_Update(digest.ctx, tmp, 32);
        }
        sha256_Final(digest.ctx, dst);
        return;
      }
      default: return;
    }
  }
  _free(digest.ctx);
}