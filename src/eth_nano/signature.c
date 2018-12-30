#include "eth_nano.h"
#include <client/context.h>
#include <client/keys.h>
#include <crypto/ecdsa.h>
#include <crypto/secp256k1.h>
#include <string.h>
#include <util/data.h>
#include <util/mem.h>
#include <util/utils.h>

bytes_t* ecrecover_signature(bytes_t* msg_hash, d_token_t* sig) {

  // check messagehash
  bytes_t* sig_msg_hash = d_get_bytesk(sig, K_MSG_HASH);
  if (sig_msg_hash && !b_cmp(sig_msg_hash, msg_hash)) return NULL;

  uint8_t pubkey[65];
  bytes_t pubkey_bytes = {.len = 64, .data = ((uint8_t*) &pubkey) + 1};
  uint8_t sdata[64];

  bytes_t* r = d_get_bytesk(sig, K_R);
  bytes_t* s = d_get_bytesk(sig, K_S);
  int      v = d_get_intk(sig, K_V);
  if (v >= 27) v -= 27;
  if (r == NULL || s == NULL || r->len + s->len != 64)
    return NULL;
  memcpy(sdata, r->data, r->len);
  memcpy(sdata + r->len, s->data, s->len);

  // verify signature
  ecdsa_recover_pub_from_sig(&secp256k1, pubkey, sdata, msg_hash->data, v);

  // now create the address
  bytes_t* hash = sha3(&pubkey_bytes);
  bytes_t* addr = b_new((char*) hash->data + 12, 20);
  b_free(hash);

  return addr;
}

int eth_verify_signature(in3_vctx_t* vc, bytes_t* msg_hash, d_token_t* sig) {
  int      res  = 0, i;
  bytes_t* addr = ecrecover_signature(msg_hash, sig);
  if (addr == NULL) return 0;
  for (i = 0; i < vc->config->signaturesCount; i++) {
    if (b_cmp(vc->config->signatures + i, addr)) {
      res = 1 << i;
      break;
    }
  }

  b_free(addr);

  return res;
}
