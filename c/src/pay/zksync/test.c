

#include "../../third-party/crypto/ecdsa.h"
#include "bjj.h"

#include "zksync.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  bytes32_t pk;
  uint8_t   seed[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3};
  //  bjj_create_pk_from_seed(bytes(seed, 32), pk);
  bjj_sign_musig(NULL, bytes(seed, 32), pk);
}