#include "bytes.h"
#include "crypto.h"
#include "debug.h"
#include "mem.h"
#include "utils.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../third-party/crypto/bip32.h"
#include "../../third-party/crypto/bip39.h"
#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/ripemd160.h"
#include "../../third-party/crypto/secp256k1.h"
#include "../../third-party/crypto/sha2.h"
#include "../../third-party/crypto/sha3.h"

#ifdef WASM
#include "emscripten.h"
#endif

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
    case DIGEST_RIPEMD_160: {
      d.ctx = _calloc(1, sizeof(RIPEMD160_CTX));
      ripemd160_Init(d.ctx);
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
    case DIGEST_RIPEMD_160: {
      if (data.len) ripemd160_Update(digest.ctx, data.data, data.len);
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
      case DIGEST_RIPEMD_160: {
        ripemd160_Final(digest.ctx, dst);
        return;
      }
      default: return;
    }
  }
  _free(digest.ctx);
}

in3_ret_t crypto_sign_digest(in3_curve_type_t type, const uint8_t* digest, const uint8_t* pk, uint8_t* dst) {
  switch (type) {
    case ECDSA_SECP256K1: return ecdsa_sign_digest(&secp256k1, pk, digest, dst, dst + 64, NULL) < 0 ? IN3_EINVAL : IN3_OK;
    default: return IN3_ENOTSUP;
  }
}
in3_ret_t crypto_recover(in3_curve_type_t type, const uint8_t* digest, bytes_t signature, uint8_t* dst) {
  switch (type) {
    case ECDSA_SECP256K1: {
      uint8_t pub[65] = {0};
      if (ecdsa_recover_pub_from_sig(&secp256k1, pub, signature.data, digest, signature.data[64] % 27)) return IN3_EINVAL;
      memcpy(dst, pub + 1, 64);
      return IN3_OK;
    }
    default: return IN3_ENOTSUP;
  }
}
static in3_ret_t crypto_pk_to_public_key(in3_curve_type_t type, const uint8_t* pk, uint8_t* dst) {
  switch (type) {
    case ECDSA_SECP256K1: {
      uint8_t public_key[65];
      ecdsa_get_public_key65(&secp256k1, pk, public_key);
      memcpy(dst, public_key + 1, 64);
      return IN3_OK;
    }
    default: return IN3_ENOTSUP;
  }
}

in3_ret_t crypto_convert(in3_curve_type_t type, in3_convert_type_t conv_type, bytes_t src, uint8_t* dst, int* dst_len) {
  switch (conv_type) {
    case CONV_PK32_TO_PUB64: {
      if (dst_len) *dst_len = 64;
      return src.len == 32 ? crypto_pk_to_public_key(type, src.data, dst) : IN3_EINVAL;
    }
    case CONV_SIG65_TO_DER: {
      if (src.len != 65) return IN3_EINVAL;
      int l = ecdsa_sig_to_der(src.data, dst);
      if (dst_len) *dst_len = l;
      return l >= 0 ? IN3_OK : IN3_EINVAL;
    }
    default: return IN3_ENOTSUP;
  }
}

#ifdef WASM
EM_JS(void, wasm_random_buffer, (uint8_t * dst, size_t len), {
  // unload len
  var res = randomBytes(len);
  for (var i = 0; i < len; i++) {
    HEAPU8[dst + i] = res[i];
  }
})
#endif
void random_buffer(uint8_t* dst, size_t len) {
#ifdef WASM
  wasm_random_buffer(dst, len);
  return;
#else
  FILE* r = fopen("/dev/urandom", "r");
  if (r) {
    for (size_t i = 0; i < len; i++) dst[i] = (uint8_t) fgetc(r);
    fclose(r);
    return;
  }
#endif
  srand(current_ms() % 0xFFFFFFFF);
#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__)
  unsigned int number;
  for (size_t i = 0; i < len; i++) dst[i] = (rand_s(&number) ? rand() : (int) number) % 256;
#else
  for (size_t i = 0; i < len; i++) dst[i] = rand() % 256;
#endif
}

static void bip32_add_path(HDNode node, char* path, uint8_t* pk) {
  char* tmp = alloca(strlen(path) + 1);
  strcpy(tmp, path);
  char* p = NULL;
  while ((p = strtok(p ? NULL : tmp, "/"))) {
    if (strcmp(p, "m") == 0) continue;
    if (p[0] == '\'')
      hdnode_private_ckd_prime(&node, atoi(p + 1));
    else if (p[strlen(p) - 1] == '\'') {
      char tt[50];
      strcpy(tt, p);
      tt[strlen(p) - 1] = 0;
      hdnode_private_ckd_prime(&node, atoi(p + 1));
    }
    else
      hdnode_private_ckd(&node, atoi(p));
  }
  memcpy(pk, node.private_key, 32);
  memzero(&node, sizeof(node));
}

in3_ret_t bip32(bytes_t seed, in3_curve_type_t curve, const char* path, uint8_t* dst) {
  UNUSED_VAR(curve);
  char*  curvename = "secp256k1";
  HDNode node      = {0};
  if (!hdnode_from_seed(seed.data, (int) seed.len, curvename, &node)) return IN3_EINVAL;
  if (!path)
    memcpy(dst, node.private_key, 32);
  else {
    char* p = _strdupn(path, -1);
    char* s = strtok(p, ",|; \n");
    for (uint8_t* pp = dst; s; s = strtok(NULL, ",|; \n"), pp += 32)
      bip32_add_path(node, s, pp);
  }
  memzero(&node, sizeof(node));
  return IN3_OK;
}

char* mnemonic_create(bytes_t seed) {
  const char* res = mnemonic_from_data(seed.data, seed.len);
  char*       r   = _strdupn(res, -1);
  mnemonic_clear();
  return r;
}

in3_ret_t mnemonic_verify(const char* mnemonic) {
  return mnemonic_check(mnemonic) ? IN3_OK : IN3_EINVAL;
}