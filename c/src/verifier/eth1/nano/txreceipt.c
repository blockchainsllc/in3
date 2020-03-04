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
#include "../../../third-party/crypto/ecdsa.h"
#include "../../../verifier/eth1/nano/eth_nano.h"
#include "../../../verifier/eth1/nano/merkle.h"
#include "../../../verifier/eth1/nano/rlp.h"
#include "../../../verifier/eth1/nano/serialize.h"
#include <string.h>

bytes_t* create_tx_path(uint32_t index) {
  uint8_t data[4];
  bytes_t b = {.len = 4, .data = data};

  if (index == 0)
    b.len = 0;
  else {
    int_to_bytes(index, data);
    b_optimize_len(&b);
  }

  bytes_builder_t* bb = bb_new();
  rlp_encode_item(bb, &b);
  return bb_move_to_bytes(bb);
}

in3_ret_t eth_verify_eth_getTransactionReceipt(in3_vctx_t* vc, bytes_t* tx_hash) {

  in3_ret_t  res = IN3_OK;
  int        i;
  bytes_t    root;
  d_token_t* block_hash = d_getl(vc->result, K_BLOCK_HASH, 32);

  if (!tx_hash)
    return vc_err(vc, "No Transaction Hash found");
  if (tx_hash->len != 32)
    return vc_err(vc, "The transactionHash has the wrong length!");

  // this means result: null, which is ok, since we can not verify a transaction that does not exists
  if (!vc->result || d_type(vc->result) == T_NULL)
    return IN3_OK;

  if (!vc->proof)
    return vc_err(vc, "Proof is missing!");

  bytes_t* blockHeader = d_get_bytesk(vc->proof, K_BLOCK);
  if (!blockHeader)
    return vc_err(vc, "No Block-Proof!");

  // verify the header
  res = eth_verify_blockheader(vc, blockHeader, d_bytes(block_hash));

  // make sure the blocknumner on the receipt is correct
  if (res == IN3_OK && (rlp_decode_in_list(blockHeader, BLOCKHEADER_NUMBER, &root) != 1 || bytes_to_long(root.data, root.len) != d_get_longk(vc->result, K_BLOCK_NUMBER)))
    res = vc_err(vc, "wrong blocknumber in the result");

  if (res == IN3_OK) {
    // encode the tx_path
    bytes_t* path = create_tx_path(d_get_intk(vc->proof, K_TX_INDEX));

    // verify the merkle proof for the receipt
    if (rlp_decode_in_list(blockHeader, BLOCKHEADER_RECEIPT_ROOT, &root) != 1)
      res = vc_err(vc, "no receipt_root");
    else {
      bytes_t*  receipt_raw = serialize_tx_receipt(vc->result);
      bytes_t** proof       = d_create_bytes_vec(d_get(vc->proof, K_MERKLE_PROOF));

      if (!proof || !trie_verify_proof(&root, path, proof, receipt_raw))
        res = vc_err(vc, "Could not verify the merkle proof");

      b_free(receipt_raw);
      if (proof) _free(proof);
    }

    // now we need to verify the transactionIndex by making sure we can do the merkle proof for the same transactionhash and transaction index.
    if (res == IN3_OK) {
      bytes_t   raw_transaction = {.len = 0, .data = NULL};
      bytes_t** proof           = d_create_bytes_vec(d_get(vc->proof, K_TX_PROOF));

      // get the transaction root and do the merkle proof.
      if (rlp_decode_in_list(blockHeader, BLOCKHEADER_TRANSACTIONS_ROOT, &root) != 1)
        res = vc_err(vc, "no tx root");
      else {
        if (!proof || !trie_verify_proof(&root, path, proof, &raw_transaction))
          res = vc_err(vc, "Could not verify the tx proof");
        else if (raw_transaction.data == NULL)
          res = vc_err(vc, "No value returned after verification");
        else {
          // after a successfull merkle proof, we want to make sure the transaction hash we asked for matches the hash of the last value.
          uint8_t proofed_hash[32];
          sha3_to(&raw_transaction, proofed_hash);
          if (memcmp(proofed_hash, tx_hash->data, 32))
            res = vc_err(vc, "The TransactionHash is not the same as expected");
        }
      }
      if (proof) _free(proof);
    }
    b_free(path);
  }

  // if this was all successfull, we still need to make sure all values are correct in the result.
  if (res == IN3_OK) {

    // check rest of the values
    if (!d_eq(d_get(vc->proof, K_TX_INDEX), d_get(vc->result, K_TRANSACTION_INDEX)))
      return vc_err(vc, "wrong transactionIndex");
    if (!b_cmp(tx_hash, d_get_bytesk(vc->result, K_TRANSACTION_HASH)))
      return vc_err(vc, "wrong transactionHash");

    d_token_t *l, *logs = d_get(vc->result, K_LOGS), *block_number = d_get(vc->result, K_BLOCK_NUMBER);
    for (i = 0, l = logs + 1; i < d_len(logs); i++, l = d_next(l)) {
      if (!d_eq(block_number, d_get(l, K_BLOCK_NUMBER)))
        return vc_err(vc, "wrong block number in log");
      if (!d_eq(block_hash, d_getl(l, K_BLOCK_HASH, 32)))
        return vc_err(vc, "wrong block hash in log");
      if (vc->config->use_full_proof && d_get_intk(l, K_LOG_INDEX) != i)
        return vc_err(vc, "wrong log index");
      if (!b_cmp(d_get_bytesk(l, K_TRANSACTION_HASH), tx_hash))
        return vc_err(vc, "wrong tx Hash");
      if (!d_eq(d_get(vc->proof, K_TX_INDEX), d_get(l, K_TRANSACTION_INDEX)))
        return vc_err(vc, "wrong tx index");
    }
  }

  return res;
}
