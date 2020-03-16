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
#include "../../../third-party/crypto/bignum.h"
#include "../../../third-party/crypto/ecdsa.h"
#include "../../../third-party/crypto/secp256k1.h"
#include "../../../verifier/eth1/nano/eth_nano.h"
#include "../../../verifier/eth1/nano/merkle.h"
#include "../../../verifier/eth1/nano/rlp.h"
#include "../../../verifier/eth1/nano/serialize.h"
#include <string.h>

static uint8_t* secp256k1n_2 = (uint8_t*) "\x7F\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x5D\x57\x6E\x73\x57\xA4\x50\x1D\xDF\xE9\x2F\x46\x68\x1B\x20\xA0";

in3_ret_t eth_verify_tx_values(in3_vctx_t* vc, d_token_t* tx, bytes_t* raw) {
  d_token_t* t = NULL;
  uint8_t    hash[32], pubkey[65], sdata[64];
  bytes_t    pubkey_bytes = {.len = 64, .data = ((uint8_t*) &pubkey) + 1};

  bytes_t* r        = d_get_byteskl(tx, K_R, 32);
  bytes_t* s        = d_get_byteskl(tx, K_S, 32);
  uint32_t v        = d_get_intk(tx, K_V);
  uint32_t chain_id = v > 35 ? (v - 35) / 2 : 0;

  // check transaction hash
  if (sha3_to(raw ? raw : d_get_bytesk(tx, K_RAW), &hash) == 0 && memcmp(hash, d_get_byteskl(tx, K_HASH, 32)->data, 32))
    return vc_err(vc, "wrong transactionHash");

  // check raw data
  if ((t = d_get(tx, K_RAW)) && raw && !b_cmp(raw, d_bytes(t)))
    return vc_err(vc, "invalid raw-value");

  // check standardV
  if ((t = d_get(tx, K_STANDARD_V)) && raw && ((chain_id ? (v - chain_id * 2 - 8) : v) - 27) != (unsigned) d_int(t))
    return vc_err(vc, "standardV is invalid");

  // check chain id
  if ((t = d_get(tx, K_CHAIN_ID)) && (unsigned) d_int(t) != chain_id)
    return vc_err(vc, "wrong chain_id");

  // All transaction signatures whose s-value is greater than secp256k1n/2 are considered invalid.
  if (!s || s->len > 32 || (s->len == 32 && memcmp(s->data, secp256k1n_2, 32) > 0))
    return vc_err(vc, "invalid v-value of the signature");

  // r & s have valid length?
  if (r == NULL || s == NULL || r->len + s->len > 64)
    return vc_err(vc, "invalid r/s-value of the signature");

  // combine r+s
  memset(sdata, 0, 64);
  memcpy(sdata + 32 - r->len, r->data, r->len);
  memcpy(sdata + 64 - s->len, s->data, s->len);

  // calculate the  messagehash
  bytes_builder_t* bb = bb_new();
  bytes_t          raw_list, item;
  rlp_decode(raw ? raw : d_get_bytesk(tx, K_RAW), 0, &raw_list);
  rlp_decode(&raw_list, 5, &item);
  bb_write_raw_bytes(bb, raw_list.data, item.data + item.len - raw_list.data);
  if (chain_id) {
    uint8_t  chain_data[4];
    uint8_t *pc = chain_data, lc = 4;
    int_to_bytes(chain_id, chain_data);
    optimize_len(pc, lc);
    item.len  = lc;
    item.data = pc;
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

  if ((t = d_getl(tx, K_PUBLIC_KEY, 64)) && memcmp(pubkey_bytes.data, t->data, t->len) != 0)
    return vc_err(vc, "invalid public Key");

  if ((t = d_getl(tx, K_FROM, 20)) && sha3_to(&pubkey_bytes, &hash) == 0 && memcmp(hash + 12, t->data, 20))
    return vc_err(vc, "invalid from address");
  return IN3_OK;
}

in3_ret_t eth_verify_eth_getTransaction(in3_vctx_t* vc, bytes_t* tx_hash) {

  in3_ret_t res = IN3_OK;

  if (!tx_hash) return vc_err(vc, "No Transaction Hash found");
  if (tx_hash->len != 32) return vc_err(vc, "The transactionHash has the wrong length!");

  // this means result: null, which is ok, since we can not verify a transaction that does not exists
  if (!vc->proof) return vc_err(vc, "Proof is missing!");

  bytes_t* blockHeader = d_get_bytesk(vc->proof, K_BLOCK);
  if (!blockHeader)
    return vc_err(vc, "No Block-Proof!");

  res = eth_verify_blockheader(vc, blockHeader, d_get_byteskl(vc->result, K_BLOCK_HASH, 32));
  if (res == IN3_OK) {
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

    if (res == IN3_OK)
      res = eth_verify_tx_values(vc, vc->result, &raw_transaction);

    if (res == IN3_OK && !d_eq(d_get(vc->result, K_TRANSACTION_INDEX), d_get(vc->proof, K_TX_INDEX)))
      res = vc_err(vc, "wrong transaction index");
    if (res == IN3_OK && (rlp_decode_in_list(blockHeader, BLOCKHEADER_NUMBER, &root) != 1 || d_get_longk(vc->result, K_BLOCK_NUMBER) != bytes_to_long(root.data, root.len)))
      res = vc_err(vc, "wrong block number");

    if (proof) _free(proof);
    b_free(path);

    bytes_t* tx_data = serialize_tx(vc->result);
    if (res == IN3_OK && !b_cmp(tx_data, &raw_transaction))
      res = vc_err(vc, "Could not verify the transaction data");

    b_free(tx_data);
  }
  return res;
}

in3_ret_t eth_verify_eth_getTransactionByBlock(in3_vctx_t* vc, d_token_t* blk, uint32_t tx_idx) {
  in3_ret_t res   = IN3_OK;
  bytes_t*  hash_ = d_get_byteskl(vc->result, K_BLOCK_HASH, 32);

  // this means result: null, which is ok, since we can not verify a transaction that does not exists
  if (!vc->proof) return vc_err(vc, "Proof is missing!");

  bytes_t* blockHeader = d_get_bytesk(vc->proof, K_BLOCK);
  if (!blockHeader) return vc_err(vc, "No Block-Proof!");

  // verify that the block matches the block as described in the transaction
  if (d_type(blk) == T_BYTES) {
    bytes32_t bhash;
    bytes_t*  blk_hash = d_bytes(blk);

    if (!blk_hash || blk_hash->len != 32)
      return vc_err(vc, "No block hash found");
    else if (hash_ && !b_cmp(blk_hash, hash_))
      return vc_err(vc, "The block hash does not match the required");
    else if (sha3_to(blockHeader, bhash) || memcmp(bhash, blk_hash->data, 32))
      return vc_err(vc, "The block header does not match the required");
  } else if (d_type(blk) == T_INTEGER) {
    uint64_t blk_num = d_long(blk);
    bytes_t  number_in_header;
    if (!blk_num)
      return vc_err(vc, "No block number found");
    else if (d_get(vc->result, K_BLOCK_NUMBER) && blk_num != d_get_longk(vc->result, K_BLOCK_NUMBER))
      return vc_err(vc, "The block number does not match the required");
    else if (rlp_decode_in_list(blockHeader, BLOCKHEADER_NUMBER, &number_in_header) != 1 || bytes_to_long(number_in_header.data, number_in_header.len) != blk_num)
      return vc_err(vc, "The block number in the header does not match the required");
  } else if (d_type(blk) == T_STRING && !strcmp(d_string(blk), "latest")) {
    // fall-through to continue verification
  } else {
    return vc_err(vc, "No block hash & number found");
  }

  if (d_get(vc->result, K_TRANSACTION_INDEX) && tx_idx != (uint32_t) d_get_intk(vc->result, K_TRANSACTION_INDEX))
    return vc_err(vc, "The transaction index does not match the required");

  res = eth_verify_blockheader(vc, blockHeader, d_get_byteskl(vc->result, K_BLOCK_HASH, 32));
  if (res == IN3_OK) {
    bytes_t*  path = create_tx_path(d_get_intk(vc->proof, K_TX_INDEX));
    bytes_t   root, raw_transaction = {.len = 0, .data = NULL};
    bytes_t** proof = d_create_bytes_vec(d_get(vc->proof, K_MERKLE_PROOF));

    if (rlp_decode_in_list(blockHeader, BLOCKHEADER_TRANSACTIONS_ROOT, &root) != 1)
      res = vc_err(vc, "no tx root");
    else {
      if (!proof) {
        res = vc_err(vc, "No merkle proof");
      } else {
        int verified = trie_verify_proof(&root, path, proof, d_type(vc->result) == T_NULL ? NULL : &raw_transaction);
        if (d_type(vc->result) == T_NULL && !verified)
          res = vc_err(vc, "Could not prove non-existence of transaction");
        else if (!verified && raw_transaction.data == NULL)
          res = vc_err(vc, "Could not verify the tx proof");
      }
    }

    if (proof) _free(proof);
    b_free(path);

    if (d_type(vc->result) != T_NULL) {
      if (res == IN3_OK)
        res = eth_verify_tx_values(vc, vc->result, &raw_transaction);

      if (res == IN3_OK && !d_eq(d_get(vc->result, K_TRANSACTION_INDEX), d_get(vc->proof, K_TX_INDEX)))
        res = vc_err(vc, "wrong transaction index");
      if (res == IN3_OK && (rlp_decode_in_list(blockHeader, BLOCKHEADER_NUMBER, &root) != 1 || d_get_longk(vc->result, K_BLOCK_NUMBER) != bytes_to_long(root.data, root.len)))
        res = vc_err(vc, "wrong block number");

      bytes_t* tx_data = serialize_tx(vc->result);
      if (res == IN3_OK && !b_cmp(tx_data, &raw_transaction))
        res = vc_err(vc, "Could not verify the transaction data");
      b_free(tx_data);
    }
  }
  return res;
}
