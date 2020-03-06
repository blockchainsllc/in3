/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 * 
 * 
 * COMMERCIAL LICENSE USAGE
 * 
 * Licensees holding a valid commercial license may use this file in accordance 
 * with the commercial license agreement provided with the Software or, alternatively, 
 * in accordance with the terms contained in a written agreement between you and 
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
 * information please contact slock.it at in3@slock.it.
 * 	
 * Alternatively, this file may be used under the AGPL license as follows:
 *    
 * AGPL LICENSE USAGE
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

#include "../../../core/client/context.h"
#include "../../../core/client/keys.h"
#include "../../../core/util/data.h"
#include "../../../core/util/mem.h"
#include "../../../verifier/eth1/nano/eth_nano.h"
#include "../../../verifier/eth1/nano/merkle.h"
#include "../../../verifier/eth1/nano/rlp.h"
#include "../../../verifier/eth1/nano/serialize.h"
#include "eth_basic.h"
#include "trie.h"
#include <string.h>

static in3_ret_t eth_verify_uncles(in3_vctx_t* vc, bytes32_t uncle_hash, d_token_t* uncles_headers, d_token_t* uncle_hashes) {
  if (!uncles_headers || !uncle_hashes || d_len(uncles_headers) != d_len(uncle_hashes) || d_type(uncles_headers) != d_type(uncle_hashes) || d_type(uncle_hashes) != T_ARRAY)
    return vc_err(vc, "invalid uncles proofs");

  bytes32_t        hash2;
  bytes_t          hash, header;
  bytes_builder_t* bb = bb_new();
  for (d_iterator_t iter_hash = d_iter(uncle_hashes), iter_header = d_iter(uncles_headers); iter_header.left && iter_hash.left; d_iter_next(&iter_hash), d_iter_next(&iter_header)) {
    hash   = d_to_bytes(iter_hash.token);
    header = d_to_bytes(iter_header.token);
    sha3_to(&header, hash2);
    if (memcmp(hash.data, hash2, 32)) {
      bb_free(bb);
      return vc_err(vc, "invalid uncles blockheader");
    }
    bb_write_raw_bytes(bb, header.data, header.len);
  }
  rlp_encode_to_list(bb);
  sha3_to(&bb->b, hash2);
  bb_free(bb);
  return memcmp(hash2, uncle_hash, 32) ? vc_err(vc, "invalid uncles root") : IN3_OK;
}

in3_ret_t eth_verify_eth_getBlockTransactionCount(in3_vctx_t* vc, bytes_t* block_hash, uint64_t blockNumber) {

  in3_ret_t  res = IN3_OK;
  int        i;
  d_token_t *transactions, *t = NULL;
  bytes_t    t_root, tmp;
  if (d_type(vc->result) != T_INTEGER)
    return vc_err(vc, "Invalid transaction count");

  //transactions count
  int32_t count = d_int(vc->result);

  // this means result: null, which is ok, since we can not verify a transaction that does not exists
  if (!vc->proof)
    return vc_err(vc, "Proof is missing!");

  // verify the blockdata
  bytes_t* header = d_get_bytesk(vc->proof, K_BLOCK);
  if (!header)
    return vc_err(vc, "no blockheader");
  if (eth_verify_blockheader(vc, header, block_hash) != IN3_OK)
    return vc_err(vc, "invalid blockheader");

  // check blocknumber
  if (!block_hash && (rlp_decode_in_list(header, BLOCKHEADER_NUMBER, &tmp) != 1 || bytes_to_long(tmp.data, tmp.len) != blockNumber))
    return vc_err(vc, "Invalid blocknumber");

  // extract transaction root
  if (rlp_decode_in_list(header, BLOCKHEADER_TRANSACTIONS_ROOT, &t_root) != 1)
    return vc_err(vc, "invalid transaction root");

  // if we have transaction, we need to verify them as well
  if (!(transactions = d_get(vc->proof, K_TRANSACTIONS)))
    vc_err(vc, "Missing transaction-properties");

  // verify transaction count
  if (d_len(transactions) != count)
    return vc_err(vc, "Transaction count mismatch");

  trie_t* trie = trie_new();
  for (i = 0, t = transactions + 1; i < d_len(transactions); i++, t = d_next(t)) {
    bool     is_raw_tx = d_type(t) == T_BYTES;
    bytes_t* path      = create_tx_path(i);
    bytes_t* tx        = is_raw_tx ? d_bytes(t) : serialize_tx(t);
    trie_set_value(trie, path, tx);
    if (!is_raw_tx) b_free(tx);
    b_free(path);
  }

  // check tx root
  if (t_root.len != 32 || memcmp(t_root.data, trie->root, 32))
    res = vc_err(vc, "Wrong Transaction root");

  trie_free(trie);

  return res;
}

in3_ret_t eth_verify_eth_getBlock(in3_vctx_t* vc, bytes_t* block_hash, uint64_t blockNumber) {

  in3_ret_t  res = IN3_OK;
  int        i;
  d_token_t *transactions, *t, *t2, *tx_hashs, *txh = NULL;
  bytes_t    tmp, *bhash;
  uint64_t   bnumber = d_get_longk(vc->result, K_NUMBER);
  bhash              = d_get_byteskl(vc->result, K_HASH, 32);
  if (block_hash && !b_cmp(block_hash, bhash))
    return vc_err(vc, "The transactionHash does not match the required");

  if (blockNumber && blockNumber != bnumber)
    return vc_err(vc, "The blockNumber  does not match the required");

  // this means result: null, which is ok, since we can not verify a transaction that does not exists
  if (!vc->proof)
    return vc_err(vc, "Proof is missing!");

  // verify the blockdata
  bytes_t* header_from_data = serialize_block_header(vc->result);
  if (eth_verify_blockheader(vc, header_from_data, d_get_byteskl(vc->result, K_HASH, 32))) {
    b_free(header_from_data);
    return vc_err(vc, "invalid blockheader!");
  }
  b_free(header_from_data);

  // check additional props
  if ((t = d_getl(vc->result, K_MINER, 20)) && (t2 = d_getl(vc->result, K_AUTHOR, 20)) && !d_eq(t, t2))
    return vc_err(vc, "invalid author");

  if ((t = d_getl(vc->result, K_MIX_HASH, 32)) && (t2 = d_get(vc->result, K_SEAL_FIELDS))) {
    if (rlp_decode(d_get_bytes_at(t2, 0), 0, &tmp) != 1 || !b_cmp(d_bytes(t), &tmp))
      return vc_err(vc, "invalid mixhash");
    if (rlp_decode(d_get_bytes_at(t2, 1), 0, &tmp) != 1 || !b_cmp(d_get_bytesk(vc->result, K_NONCE), &tmp))
      return vc_err(vc, "invalid nonce");
  }

  bool include_full_tx = d_get_int_at(d_get(vc->request, K_PARAMS), 1);
  bool full_proof      = vc->config->use_full_proof;

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
      bool     is_raw_tx = d_type(t) == T_BYTES;
      bytes_t* path      = create_tx_path(i);
      bytes_t* tx        = is_raw_tx ? d_bytes(t) : serialize_tx(t);
      bytes_t* h         = (full_proof || !include_full_tx) ? sha3(tx) : NULL;

      if (!is_raw_tx) {
        if (eth_verify_tx_values(vc, t, tx))
          res = IN3_EUNKNOWN;

        if ((t2 = d_getl(t, K_BLOCK_HASH, 32)) && !b_cmp(d_bytes(t2), bhash))
          res = vc_err(vc, "Wrong Blockhash in tx");

        if ((t2 = d_get(t, K_BLOCK_NUMBER)) && d_long(t2) != bnumber)
          res = vc_err(vc, "Wrong Blocknumber in tx");

        if ((t2 = d_get(t, K_TRANSACTION_INDEX)) && d_int(t2) != i)
          res = vc_err(vc, "Wrong Transaction index in tx");
      }

      if (h && txh) {
        if (!b_cmp(d_bytes(txh), h))
          res = vc_err(vc, "Wrong Transactionhash");
        txh = d_next(txh);
      }
      trie_set_value(trie, path, tx);
      if (!is_raw_tx) b_free(tx);
      b_free(path);
      if (h) b_free(h);
    }

    bytes_t t_root = d_to_bytes(d_getl(vc->result, K_TRANSACTIONS_ROOT, 32));

    if (t_root.len != 32 || memcmp(t_root.data, trie->root, 32))
      res = vc_err(vc, "Wrong Transaction root");

    trie_free(trie);

    // verify uncles
    if (res == IN3_OK && full_proof)
      return eth_verify_uncles(vc, d_get_bytesk(vc->result, K_SHA3_UNCLES)->data, d_get(vc->proof, K_UNCLES), d_get(vc->result, K_UNCLES));

  } else
    res = vc_err(vc, "Missing transaction-properties");

  return res;
}
