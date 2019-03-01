
#include "../eth_nano/eth_nano.h"
#include "../eth_nano/merkle.h"
#include "../eth_nano/rlp.h"
#include "../eth_nano/serialize.h"
#include "eth_basic.h"
#include <client/context.h>
#include <client/keys.h>
#include <crypto/bignum.h>
#include <crypto/ecdsa.h>
#include <crypto/secp256k1.h>
#include <string.h>
#include <util/data.h>
#include <util/mem.h>
#include <util/utils.h>

static uint8_t* secp256k1n_2 = (uint8_t*) "\x7F\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x5D\x57\x6E\x73\x57\xA4\x50\x1D\xDF\xE9\x2F\x46\x68\x1B\x20\xA0";

int eth_verify_tx_values(in3_vctx_t* vc, d_token_t* tx, bytes_t* raw) {
  d_token_t* t;
  uint8_t    hash[32], pubkey[65], sdata[64];
  bytes_t    pubkey_bytes = {.len = 64, .data = ((uint8_t*) &pubkey) + 1};

  bytes_t* r        = d_get_bytesk(tx, K_R);
  bytes_t* s        = d_get_bytesk(tx, K_S);
  uint32_t v        = d_get_intk(tx, K_V);
  uint32_t chain_id = v > 35 ? (v - 35) / 2 : 0;

  // check transaction hash
  if (sha3_to(raw ? raw : d_get_bytesk(tx, K_RAW), &hash) == 0 && memcmp(hash, d_get_bytesk(tx, K_HASH)->data, 32))
    return vc_err(vc, "wrong transactionHash");

  // check raw data
  if ((t = d_get(tx, K_RAW)) && raw && !b_cmp(raw, d_bytes(t)))
    return vc_err(vc, "invalid raw-value");

  // check standardV
  if ((t = d_get(tx, K_STANDARD_V)) && raw && (uint32_t)((chain_id ? (v - chain_id * 2 - 8) : v) - 27) != d_int(t))
    return vc_err(vc, "standardV is invalid");

  // check chain id
  if ((t = d_get(tx, K_CHAIN_ID)) && d_int(t) != chain_id)
    return vc_err(vc, "wrong chainID");

  // All transaction signatures whose s-value is greater than secp256k1n/2 are considered invalid.
  if (!s || s->len > 32 || (s->len == 32 && memcmp(s->data, secp256k1n_2, 32) > 0))
    return vc_err(vc, "invalid v-value of the signature");

  // r & s have valid length?
  if (r == NULL || s == NULL || r->len + s->len != 64)
    return vc_err(vc, "invalid r/s-value of the signature");

  // combine r+s
  memcpy(sdata, r->data, r->len);
  memcpy(sdata + r->len, s->data, s->len);

  // calculate the  messagehash
  bytes_builder_t* bb = bb_new();
  bytes_t          raw_list, item;
  rlp_decode(raw ? raw : d_get_bytesk(tx, K_RAW), 0, &raw_list);
  rlp_decode(&raw_list, 5, &item);
  bb_write_raw_bytes(bb, raw_list.data, item.data + item.len - raw_list.data);
  if (chain_id) {
    uint8_t v_chain = chain_id;
    item.len        = 1;
    item.data       = &v_chain;
    rlp_encode_item(bb, &item);
    item.len = 0;
    rlp_encode_item(bb, &item);
    rlp_encode_item(bb, &item);
  }
  rlp_encode_to_list(bb);
  sha3_to(&bb->b, hash);
  bb_free(bb);

  // verify signature
  if (ecdsa_recover_pub_from_sig(&secp256k1, pubkey, sdata, hash, (chain_id ? v - chain_id * 2 - 8 : v) - 27))
    return vc_err(vc, "could not recover signature");

  if ((t = d_get(tx, K_PUBLIC_KEY)) && memcmp(pubkey_bytes.data, t->data, t->len) != 0)
    return vc_err(vc, "invalid public Key");

  if ((t = d_get(tx, K_FROM)) && sha3_to(&pubkey_bytes, &hash) == 0 && memcmp(hash + 12, t->data, 20))
    return vc_err(vc, "invalid from address");
  return 0;
}

int eth_verify_eth_getTransaction(in3_vctx_t* vc, bytes_t* tx_hash) {

  int res = 0;

  if (!tx_hash) return vc_err(vc, "No Transaction Hash found");
  if (tx_hash->len != 32) return vc_err(vc, "The transactionHash has the wrong length!");

  // this means result: null, which is ok, since we can not verify a transaction that does not exists
  if (!vc->proof) return vc_err(vc, "Proof is missing!");

  bytes_t* blockHeader = d_get_bytesk(vc->proof, K_BLOCK);
  if (!blockHeader)
    return vc_err(vc, "No Block-Proof!");

  res = eth_verify_blockheader(vc, blockHeader, d_get_bytesk(vc->result, K_BLOCK_HASH));
  if (res == 0) {
    bytes_t*  path = create_tx_path(d_get_intk(vc->proof, K_TX_INDEX));
    bytes_t   root, raw_transaction = {.len = 0, .data = NULL};
    bytes_t** proof = d_create_bytes_vec(d_get(vc->proof, K_MERKLE_PROOF));

    if (rlp_decode_in_list(blockHeader, 4, &root) != 1)
      res = vc_err(vc, "no tx root");
    else {
      if (!proof || !trie_verify_proof(&root, path, proof, &raw_transaction) || raw_transaction.data == NULL)
        res = vc_err(vc, "Could not verify the tx proof");
      else {
        uint8_t proofed_hash[32];
        sha3_to(&raw_transaction, proofed_hash);
        if (memcmp(proofed_hash, tx_hash->data, 32))
          res = vc_err(vc, "The TransactionHash is not the same as expected");
      }
    }

    if (res == 0)
      res = eth_verify_tx_values(vc, vc->result, &raw_transaction);

    if (res == 0 && !d_eq(d_get(vc->result, K_TRANSACTION_INDEX), d_get(vc->proof, K_TX_INDEX)))
      res = vc_err(vc, "wrong transaction index");
    if (res == 0 && (rlp_decode_in_list(blockHeader, BLOCKHEADER_NUMBER, &root) != 1 || d_get_longk(vc->result, K_BLOCK_NUMBER) != bytes_to_long(root.data, root.len)))
      res = vc_err(vc, "wrong block number");

    if (proof) _free(proof);
    b_free(path);

    bytes_t* tx_data = serialize_tx(vc->result);
    if (res == 0 && !b_cmp(tx_data, &raw_transaction))
      res = vc_err(vc, "Could not verify the transaction data");

    b_free(tx_data);
  }
  return res;
}
