
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

int eth_verify_eth_getBlock(in3_vctx_t* vc, bytes_t* block_hash, uint64_t blockNumber) {

  int        res = 0, i;
  d_token_t *transactions, *t, *t2, *tx_hashs, *txh = NULL;
  bytes_t    tmp, *bhash;
  uint64_t   bnumber = d_get_longk(vc->result, K_NUMBER);
  bhash              = d_get_bytesk(vc->result, K_HASH);
  if (block_hash && !b_cmp(block_hash, bhash))
    return vc_err(vc, "The transactionHash does not match the required");

  if (blockNumber && blockNumber != bnumber)
    return vc_err(vc, "The blockNumber  does not match the required");

  // this means result: null, which is ok, since we can not verify a transaction that does not exists
  if (!vc->proof)
    return vc_err(vc, "Proof is missing!");

  // verify the blockdata
  bytes_t* header_from_data = serialize_block_header(vc->result);
  if (eth_verify_blockheader(vc, header_from_data, d_get_bytesk(vc->result, K_HASH))) {
    b_free(header_from_data);
    return vc_err(vc, "invalid blockheader!");
  }
  b_free(header_from_data);

  // check additional props
  if ((t = d_get(vc->result, K_MINER)) && (t2 = d_get(vc->result, K_AUTHOR)) && !d_eq(t, t2))
    return vc_err(vc, "invalid author");

  if ((t = d_get(vc->result, K_MIX_HASH)) && (t2 = d_get(vc->result, K_SEAL_FIELDS))) {
    if (rlp_decode(d_get_bytes_at(t2, 0), 0, &tmp) != 1 || !b_cmp(d_bytes(t), &tmp))
      return vc_err(vc, "invalid mixhash");
    if (rlp_decode(d_get_bytes_at(t2, 1), 0, &tmp) != 1 || !b_cmp(d_get_bytesk(vc->result, K_NONCE), &tmp))
      return vc_err(vc, "invalid nonce");
  }

  bool include_full_tx = d_get_int_at(d_get(vc->request, K_PARAMS), 1);
  bool full_proof      = vc->config->useFullProof;

  if (!include_full_tx) {
    tx_hashs = d_get(vc->result, K_TRANSACTIONS);
    txh      = tx_hashs + 1;
  }
  // if we have transaction, we need to verify them as well
  if ((transactions = d_get(include_full_tx ? vc->result : vc->proof, K_TRANSACTIONS))) {

    if (!include_full_tx && (!tx_hashs || d_len(transactions) != d_len(tx_hashs)))
      return vc_err(vc, "no transactionhashes found!");

    trie_t* trie = trie_new();
    for (i = 0, t = transactions + 1; i < d_len(transactions); i++, t = d_next(t)) {
      bytes_t* path = create_tx_path(i);
      bytes_t* tx   = serialize_tx(t);
      bytes_t* h    = (full_proof || !include_full_tx) ? sha3(tx) : NULL;

      if (eth_verify_tx_values(vc, t, tx)) res = -1;

      if ((t2 = d_get(t, K_BLOCK_HASH)) && !b_cmp(d_bytes(t2), bhash))
        res = vc_err(vc, "Wrong Blockhash in tx");

      if ((t2 = d_get(t, K_BLOCK_NUMBER)) && d_long(t2) != bnumber)
        res = vc_err(vc, "Wrong Blocknumber in tx");

      if ((t2 = d_get(t, K_TRANSACTION_INDEX)) && d_int(t2) != (uint32_t) i)
        res = vc_err(vc, "Wrong Transaction index in tx");

      if (h && txh) {
        if (!b_cmp(d_bytes(txh), h))
          res = vc_err(vc, "Wrong Transactionhash");
        txh = d_next(txh);
      }
      trie_set_value(trie, path, tx);
      b_free(path);
      b_free(tx);
      if (h) b_free(h);
    }

    bytes_t t_root = d_to_bytes(d_get(vc->result, K_TRANSACTIONS_ROOT));

    if (t_root.len != 32 || memcmp(t_root.data, trie->root, 32))
      res = vc_err(vc, "Wrong Transaction root");

    trie_free(trie);
  } else
    res = vc_err(vc, "Missing transaction-properties");

  return res;
}
