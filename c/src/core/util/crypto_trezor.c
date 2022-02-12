#include "bytes.h"
#include "crypto.h"
#include "debug.h"
#include "mem.h"
#include "utils.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
