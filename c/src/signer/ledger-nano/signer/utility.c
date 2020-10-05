#include "utility.h"
#include "../../../core/util/log.h"
#include "types.h"

void extract_signture(bytes_t i_raw_sig, uint8_t* o_sig) {

  //ECDSA signature encoded as TLV:  30 L 02 Lr r 02 Ls s
  int lr     = i_raw_sig.data[3];
  int ls     = i_raw_sig.data[lr + 5];
  int offset = 0;
  in3_log_debug("lr %d, ls %d \n", lr, ls);
  if (lr > 0x20) {
    memcpy(o_sig + offset, i_raw_sig.data + 5, lr - 1);
    offset = lr - 1;
  }
  else {
    memcpy(o_sig, i_raw_sig.data + 4, lr);
    offset = lr;
  }

  if (ls > 0x20) {
    memcpy(o_sig + offset, i_raw_sig.data + lr + 7, ls - 1);
  }
  else {
    memcpy(o_sig + offset, i_raw_sig.data + lr + 6, ls);
  }
}

int get_recid_from_pub_key(const ecdsa_curve* curve, uint8_t* pub_key, const uint8_t* sig, const uint8_t* digest) {

  int     i = 0;
  uint8_t p_key[65];
  int     ret   = 0;
  int     recid = -1;
  for (i = 0; i < 4; i++) {
    ret = ecdsa_recover_pub_from_sig(curve, p_key, sig, digest, i);

    if (ret == 0) {
      if (memcmp(pub_key, p_key, 65) == 0) {
        recid = i;
        break;
      }
    }
  }
  return recid;
}

uint32_t reverse_bytes(uint32_t bytes) {
  uint32_t aux = 0;
  uint8_t  byte;
  int      i;

  for (i = 0; i < 32; i += 8) {
    byte = (bytes >> i) & 0xff;
    aux |= byte << (32 - 8 - i);
  }
  return aux;
}
