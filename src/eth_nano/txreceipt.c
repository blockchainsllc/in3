#include "eth_nano.h"
#include "merkle.h"
#include "rlp.h"
#include "serialize.h"
#include <client/context.h>
#include <client/keys.h>
#include <crypto/ecdsa.h>
#include <crypto/secp256k1.h>
#include <string.h>
#include <util/data.h>
#include <util/mem.h>
#include <util/utils.h>

bytes_t* create_tx_path(uint32_t index) {
  uint8_t data[4];
  int     i;
  bytes_t b = {.len = 4, .data = data};
  if (index == 0)
    b.len = 0;
  else {
    int_to_bytes(index, data);
    for (i = 3; i >= 0; i--) {
      if (data[i] == 0) {
        b.data += i + 1;
        b.len -= i + 1;
        break;
      }
    }
  }

  bytes_builder_t* bb = bb_new();
  rlp_encode_item(bb, &b);
  return bb_move_to_bytes(bb);
}

int eth_verify_eth_getTransactionReceipt(in3_vctx_t* vc, bytes_t* tx_hash) {

  int res = 0;

  if (!tx_hash)
    return vc_err(vc, "No Transaction Hash found");
  if (tx_hash->len != 32)
    return vc_err(vc, "The transactionHash has the wrong length!");

  // this means result: null, which is ok, since we can not verify a transaction that does not exists
  if (!vc->result || d_type(vc->result) == T_NULL)
    return 0;

  if (!vc->proof)
    return vc_err(vc, "Proof is missing!");

  bytes_t* blockHeader = d_get_bytesk(vc->proof, K_BLOCK);
  if (!blockHeader)
    return vc_err(vc, "No Block-Proof!");

  // verify the header
  res = eth_verify_blockheader(vc, blockHeader, d_get_bytesk(vc->result, K_BLOCK_HASH));
  if (res == 0) {
    bytes_t* path = create_tx_path(d_get_intk(vc->proof, K_TX_INDEX));
    bytes_t  root;
    if (rlp_decode_in_list(blockHeader, 5, &root) != 1)
      res = vc_err(vc, "no receipt_root");
    else {
      bytes_t*  receipt_raw = serialize_tx_receipt(vc->result);
      bytes_t** proof       = d_create_bytes_vec(d_get(vc->proof, K_MERKLE_PROOF));

      if (!proof || !trie_verify_proof(&root, path, proof, receipt_raw))
        res = vc_err(vc, "Could not verify the merkle proof");

      b_free(receipt_raw);
      if (proof) _free(proof);
    }

    if (res == 0) {
      // now we need to verify the transactionIndex
      bytes_t   raw_transaction = {.len = 0, .data = NULL};
      bytes_t** proof           = d_create_bytes_vec(d_get(vc->proof, K_TX_PROOF));
      if (rlp_decode_in_list(blockHeader, 4, &root) != 1)
        res = vc_err(vc, "no tx root");
      else {
        if (!proof || !trie_verify_proof(&root, path, proof, &raw_transaction))
          res = vc_err(vc, "Could not verify the tx proof");
        else if (raw_transaction.data == NULL)
          res = vc_err(vc, "No value returned after verification");
        else {
          bytes_t* proofed_hash = sha3(&raw_transaction);
          if (!b_cmp(proofed_hash, tx_hash))
            res = vc_err(vc, "The TransactionHash is not the same as expected");
          b_free(proofed_hash);
        }
      }
      if (proof) _free(proof);
    }
    b_free(path);
  }
  if (res == 0) {

    // check rest iof the values
    if (!d_eq(d_get(vc->proof, K_TX_INDEX), d_get(vc->result, K_TRANSACTION_INDEX)))
      return vc_err(vc, "wrong transactionIndex");
    if (!b_cmp(tx_hash, d_get_bytesk(vc->result, K_TRANSACTION_HASH)))
      return vc_err(vc, "wrong transactionHash");
  }

  return res;
}
