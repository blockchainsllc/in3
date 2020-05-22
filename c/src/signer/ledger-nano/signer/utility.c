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
  } else {
    memcpy(o_sig, i_raw_sig.data + 4, lr);
    offset = lr;
  }

  if (ls > 0x20) {
    memcpy(o_sig + offset, i_raw_sig.data + lr + 7, ls - 1);
  } else {
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
#ifdef DEBUG
        in3_log_debug("public key matched with recid value\n");
        ba_print(p_key, 65, "get_recid_from_pub_key :keys matched");
#endif
        break;
      }
    }
  }
  return recid;
}

int decode_txn_values(bytes_t message, TXN* txn) {
  int     returnV = 0;
  bytes_t tmp;
  printf("parsing transaction\n");
  if ((returnV = rlp_decode_in_list(&message, 0, &tmp)) == 1) {
    txn->nonce.data = malloc(tmp.len);
    memcpy(txn->nonce.data, tmp.data, tmp.len);
    txn->nonce.len = tmp.len;
  }

  if ((returnV = rlp_decode_in_list(&message, 1, &tmp)) == 1) {
    txn->gasprice.data = malloc(tmp.len);
    memcpy(txn->gasprice.data, tmp.data, tmp.len);
    txn->gasprice.len = tmp.len;
  }

  if ((returnV = rlp_decode_in_list(&message, 2, &tmp)) == 1) {
    txn->startgas.data = malloc(tmp.len);
    memcpy(txn->startgas.data, tmp.data, tmp.len);
    txn->startgas.len = tmp.len;
  }

  if ((returnV = rlp_decode_in_list(&message, 3, &tmp)) == 1) {
    txn->to.data = malloc(tmp.len);
    memcpy(txn->to.data, tmp.data, tmp.len);
    txn->to.len = tmp.len;
  }

  if ((returnV = rlp_decode_in_list(&message, 4, &tmp)) == 1) {
    txn->value.data = malloc(tmp.len);
    memcpy(txn->value.data, tmp.data, tmp.len);
    txn->value.len = tmp.len;
  }

  if ((returnV = rlp_decode_in_list(&message, 4, &tmp)) == 1) {
    txn->data.data = malloc(tmp.len);
    memcpy(txn->data.data, tmp.data, tmp.len);
    txn->data.len = tmp.len;
  }
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