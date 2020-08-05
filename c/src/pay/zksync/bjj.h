

#include "../../core/client/context.h"
#include "../../core/util/bytes.h"
#include "../../core/util/error.h"
#include "../../third-party/crypto/bignum.h"

typedef struct curve_point {
  bignum256 x;
  bignum256 y;
  bignum256 t;
  bignum256 z;
} bjj_point_t;

in3_ret_t bjj_create_pk_from_seed(bytes_t seed, bytes32_t pk);

in3_ret_t bjj_sign_musig(in3_ctx_t* ctx, bytes_t data, bytes32_t pk);