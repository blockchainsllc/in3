
#include "../eth_nano/eth_nano.h"
#include "../eth_nano/merkle.h"
#include "../eth_nano/rlp.h"
#include "../eth_nano/serialize.h"
#include "eth_basic.h"
#include "trie.h"
#include <client/context.h>
#include <client/keys.h>
#include <string.h>
#include <util/data.h>
#include <util/mem.h>
#include <util/utils.h>

typedef struct receipt {
  bytes32_t tx_hash;
  bytes_t   data;
  bytes_t   block_number;
  bytes32_t block_hash;
  uint32_t  transaction_index;
} receipt_t;

in3_error_t eth_verify_eth_getLog(in3_vctx_t* vc, int l_logs) {
  in3_error_t res = IN3_OK, i = 0;
  receipt_t   receipts[l_logs];
  bytes_t     logddata, tmp, tops;

  // invalid result-token
  if (!vc->result || d_type(vc->result) != T_ARRAY) return vc_err(vc, "The result must be an array");
  // no results -> nothing to verify
  if (l_logs == 0) return IN3_OK;
  // we require proof
  if (!vc->proof) return vc_err(vc, "no proof for logs found");

  for (d_iterator_t it = d_iter(d_get(vc->proof, K_LOG_PROOF)); it.left; d_iter_next(&it)) {

    // verify the blockheader of the log entry
    bytes_t block = d_to_bytes(d_get(it.token, K_BLOCK)), tx_root, receipt_root;
    int     bl    = i;
    if (!block.len || eth_verify_blockheader(vc, &block, NULL) < 0) return vc_err(vc, "invalid blockheader");
    sha3_to(&block, receipts[i].block_hash);
    rlp_decode(&block, 0, &block);
    if (rlp_decode(&block, BLOCKHEADER_RECEIPT_ROOT, &receipt_root) != 1) return vc_err(vc, "invalid receipt root");
    if (rlp_decode(&block, BLOCKHEADER_TRANSACTIONS_ROOT, &tx_root) != 1) return vc_err(vc, "invalid tx root");
    if (rlp_decode(&block, BLOCKHEADER_NUMBER, &receipts[i].block_number) != 1) return vc_err(vc, "invalid block number");
    receipts[i].data = bytes(NULL, 0);

    // verify all receipts
    for (d_iterator_t receipt = d_iter(d_get(it.token, K_RECEIPTS)); receipt.left; d_iter_next(&receipt)) {
      if (i == l_logs) return vc_err(vc, "too many receipts in the proof");
      receipt_t* r = receipts + i;
      if (i != bl) memcpy(r, receipts + bl, sizeof(receipt_t)); // copy blocknumber and blockhash
      i++;

      // verify tx data first
      r->transaction_index = d_get_intk(receipt.token, K_TX_INDEX);
      bytes_t** proof      = d_create_bytes_vec(d_get(receipt.token, K_TX_PROOF));
      bytes_t*  path       = create_tx_path(r->transaction_index);

      if (!proof || !trie_verify_proof(&tx_root, path, proof, &r->data))
        res = vc_err(vc, "invalid tx merkle proof");
      if (proof) _free(proof);
      if (res != IN3_OK) {
        if (path) b_free(path);
        return res;
      }

      // check txhash

      sha3_to(&r->data, r->tx_hash);
      if (!bytes_cmp(d_to_bytes(d_getl(receipt.token, K_TX_HASH, 32)), bytes(r->tx_hash, 32))) {
        if (path) b_free(path);
        return vc_err(vc, "invalid tx hash");
      }

      // verify receipt data
      proof   = d_create_bytes_vec(d_get(receipt.token, K_PROOF));
      r->data = bytes(NULL, 0);

      if (!proof || !trie_verify_proof(&receipt_root, path, proof, &r->data))
        res = vc_err(vc, "invalid receipt proof");
      if (proof) _free(proof);
      if (path) b_free(path);
      if (res != IN3_OK) return res;
    }
  }

  if (i != l_logs) return vc_err(vc, "invalid receipts len in proof");
  for (d_iterator_t it = d_iter(vc->result); it.left; d_iter_next(&it)) {
    receipt_t* r = NULL;
    i            = 0;
    for (int n = 0; n < l_logs; n++) {
      if (bytes_cmp(d_to_bytes(d_get(it.token, K_TRANSACTION_HASH)), bytes(receipts[n].tx_hash, 32))) {
        r = receipts + n;
        break;
      }
    }

    if (!r) return vc_err(vc, "missing proof for log");
    d_token_t* topics = d_get(it.token, K_TOPICS);
    rlp_decode(&r->data, 0, &r->data);

    // verify the log-data
    if (rlp_decode(&r->data, 3, &logddata) != 2) return vc_err(vc, "invalid log-data");
    if (rlp_decode(&logddata, d_get_intk(it.token, K_TRANSACTION_LOG_INDEX), &logddata) != 2) return vc_err(vc, "invalid log index");

    // check address
    if (!rlp_decode(&logddata, 0, &tmp) || !bytes_cmp(tmp, d_to_bytes(d_getl(it.token, K_ADDRESS, 20)))) return vc_err(vc, "invalid address");
    if (!rlp_decode(&logddata, 2, &tmp) || !bytes_cmp(tmp, d_to_bytes(d_get(it.token, K_DATA)))) return vc_err(vc, "invalid data");
    if (rlp_decode(&logddata, 1, &tops) != 2) return vc_err(vc, "invalid topics");
    if (rlp_decode_len(&tops) != d_len(topics)) return vc_err(vc, "invalid topics len");

    for (d_iterator_t t = d_iter(topics); t.left; d_iter_next(&t)) {
      if (!rlp_decode(&tops, i++, &tmp) || !bytes_cmp(tmp, *d_bytesl(t.token, 32))) return vc_err(vc, "invalid topic");
    }

    if (d_get_longk(it.token, K_BLOCK_NUMBER) != bytes_to_long(r->block_number.data, r->block_number.len)) return vc_err(vc, "invalid blocknumber");
    if (!bytes_cmp(d_to_bytes(d_getl(it.token, K_BLOCK_HASH, 32)), bytes(r->block_hash, 32))) return vc_err(vc, "invalid blockhash");
    if (d_get_intk(it.token, K_REMOVED)) return vc_err(vc, "must be removed=false");
    if (d_get_intk(it.token, K_TRANSACTION_INDEX) != r->transaction_index) return vc_err(vc, "wrong transactionIndex");
  }

  return res;
}
