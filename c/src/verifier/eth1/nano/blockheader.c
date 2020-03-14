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
#include "../../../core/util/mem.h"
#include "../../../third-party/crypto/ecdsa.h"
#include "../../../third-party/crypto/secp256k1.h"
#include "../../../verifier/eth1/nano/eth_nano.h"
#include "../../../verifier/eth1/nano/merkle.h"
#include "../../../verifier/eth1/nano/rlp.h"
#include "../../../verifier/eth1/nano/serialize.h"
#include "../../../verifier/eth1/nano/vhist.h"
#include <string.h>

#ifdef POA
/** gets the signer from a blockheader in a aura chain.*/
static in3_ret_t get_aura_signer(in3_vctx_t* vc, bytes_t* header, uint8_t* dst) {
  bytes_t         sig, bare;
  uint8_t         d[4], bare_hash[32], pub_key[65];
  bytes_builder_t ll = {.bsize = 4, .b = {.len = 0, .data = (uint8_t*) &d}};
  struct SHA3_CTX ctx;

  // get the raw data without the sealed field
  rlp_decode(header, 0, &bare);
  rlp_decode(&bare, 12, &sig);
  bare.len = sig.len + sig.data - bare.data;

  // calculate the list prefix
  rlp_add_length(&ll, bare.len, 0xc0);

  // calculate the bare hash
  sha3_256_Init(&ctx);
  sha3_Update(&ctx, ll.b.data, ll.b.len);
  sha3_Update(&ctx, bare.data, bare.len);
  keccak_Final(&ctx, bare_hash);

  // copy the whole header
  rlp_decode(header, 0, &bare);

  // we have 3 sealed fields the messagehash is calculated hash = sha3( concat ( bare_hash | rlp_encode ( sealed_fields[2] ) ) )
  if (rlp_decode(&bare, BLOCKHEADER_SEALED_FIELD3, &sig) == 1) {
    bb_clear(&ll);
    rlp_add_length(&ll, sig.len, 0xc0);

    sha3_256_Init(&ctx);
    sha3_Update(&ctx, bare_hash, 32);
    sha3_Update(&ctx, ll.b.data, ll.b.len);
    sha3_Update(&ctx, sig.data, sig.len);
    keccak_Final(&ctx, bare_hash);
  }
  // get the signature
  rlp_decode(&bare, BLOCKHEADER_SEALED_FIELD2, &sig);

  // recover signature
  if (ecdsa_recover_pub_from_sig(&secp256k1, pub_key, sig.data, bare_hash, sig.data[64]))
    return vc_err(vc, "The signature of a validator could not recover!");

  // hash the public key
  sha3_256_Init(&ctx);
  sha3_Update(&ctx, pub_key + 1, 64);
  keccak_Final(&ctx, bare_hash);

  // and take the last 20 bytes as address
  memcpy(dst, bare_hash + 12, 20);

  return IN3_OK;
}

static in3_ret_t add_aura_validators(in3_vctx_t* vc, vhist_t** vhp) {
  uint64_t  blk = 0;
  bytes_t   tmp;
  in3_ret_t res = IN3_OK;
  int       i   = 0;
  vhist_t*  vh  = *vhp;

  // get validators from contract
  in3_proof_t proof_     = vc->ctx->client->proof;
  vc->ctx->client->proof = PROOF_NONE;
  in3_ctx_t* ctx_        = in3_client_rpc_ctx(vc->ctx->client, "in3_validatorList", "[]");
  vc->ctx->client->proof = proof_;
  res                    = ctx_get_error(ctx_, 0);
  if (res != IN3_OK) {
    ctx_free(ctx_);
    return vc_err(vc, ctx_->error);
  }

  // Validate proof
  d_token_t *ss = d_get(d_get(ctx_->responses[0], K_RESULT), K_STATES), *prf = NULL;
  for (d_iterator_t sitr = d_iter(ss); sitr.left; d_iter_next(&sitr)) {
    blk = d_get_longk(sitr.token, K_BLOCK);
    if (blk <= vh->last_change_block) continue;

    prf = d_get(sitr.token, K_PROOF);
    if (prf == NULL) {
      return vc_err(vc, "validator list has no proof");
    }

    bytes_t* prf_blk = d_get_bytesk(prf, K_BLOCK);
    rlp_decode_in_list(prf_blk, BLOCKHEADER_NUMBER, &tmp);
    uint64_t prf_blkno = bytes_to_long(tmp.data, tmp.len);
    if (blk != prf_blkno) {
      return vc_err(vc, "block number mismatch");
    }

    d_token_t* sig    = d_get(prf, K_FINALITY_BLOCKS);
    bytes_t**  blocks = _malloc((sig ? d_len(sig) + 1 : 2) * sizeof(bytes_t*));
    blocks[0]         = prf_blk;
    if (sig) {
      for (i = 0; i < d_len(sig); i++) blocks[i + 1] = d_get_bytes_at(sig, i);
    }
    blocks[sig ? d_len(sig) : 1] = NULL;

    bytes_t*         fblk = blocks[0];
    uint8_t          hash[32], signer[20];
    int              passed   = 0;
    uint8_t*         proposer = NULL;
    bytes_builder_t* curr     = vh_get_validators_for_block(vh, prf_blkno);
    size_t           currl    = curr->b.len / 20;
    i                         = 0;

    while (fblk) {
      // check signature of proposer
      if (get_aura_signer(vc, fblk, signer))
        return vc_err(vc, "could not get the signer");

      // check if it was signed by the right validator
      rlp_decode_in_list(fblk, BLOCKHEADER_SEALED_FIELD1, &tmp);
      proposer = &curr->b.data[(bytes_to_long(tmp.data, tmp.len) % currl) * 20];
      bb_free(curr);
      if (memcmp(signer, proposer, 20) != 0) {
        _free(blocks);
        return vc_err(vc, "the block was signed by the wrong key");
      }

      // calculate the blockhash
      sha3_to(fblk, &hash);

      // next block
      fblk = blocks[++i];

      // check if the next blocks parent_hash matches
      if (fblk && (rlp_decode_in_list(fblk, BLOCKHEADER_PARENT_HASH, &tmp) != 1 || memcmp(hash, tmp.data, 32) != 0)) {
        _free(blocks);
        return vc_err(vc, "The parent hashes of the finality blocks don't match");
      }

      passed++;
    }
    _free(blocks);

    if (passed * 100 / currl < vc->config->finality)
      return vc_err(vc, "not enough finality to accept state");

    // Verify receipt
    bytes_t* path = create_tx_path(d_get_intk(prf, K_TX_INDEX));

    // verify the merkle proof for the receipt
    if (rlp_decode_in_list(prf_blk, BLOCKHEADER_RECEIPT_ROOT, &tmp) != 1)
      return vc_err(vc, "no receipt_root");

    bytes_t** proof       = d_create_bytes_vec(d_get(prf, K_PROOF));
    bytes_t   raw_receipt = {.len = 0, .data = NULL};
    if (!proof || !trie_verify_proof(&tmp, path, proof, &raw_receipt))
      return vc_err(vc, "Could not verify the merkle proof");

    rlp_decode(&raw_receipt, 0, &raw_receipt);

    bytes_t log_data;
    rlp_decode(&raw_receipt, rlp_decode_len(&raw_receipt) - 1, &log_data);
    rlp_decode(&log_data, d_get_intk(prf, K_LOG_INDEX), &log_data);

    rlp_decode(&log_data, 0, &tmp);
    if (!b_cmp(&tmp, d_get_bytesk(vc->chain->spec->result, K_VALIDATOR_CONTRACT)))
      return vc_err(vc, "Wrong address in log");

    rlp_decode(&log_data, 1, &tmp);
    rlp_decode_in_list(&tmp, 0, &tmp);
    bytes_t* t = hex_to_new_bytes("55252fa6eee4741b4e24a74a70e9c11fd2c2281df8d6ea13126ff845f7825c89", 64);
    if (!bytes_cmp(tmp, *t))
      return vc_err(vc, "Wrong topic in log");
    b_free(t);

    rlp_decode(&log_data, 2, &tmp);

    bytes_t*         b;
    bytes_builder_t* vbb     = bb_new();
    uint8_t          abi[32] = {0};
    int_to_bytes(32, abi + 28);
    bb_write_raw_bytes(vbb, abi, 32);
    d_token_t* vs = d_get(sitr.token, K_VALIDATORS);
    int_to_bytes(d_len(vs), abi + 28);
    bb_write_raw_bytes(vbb, abi, 32);

    for (d_iterator_t vitr = d_iter(vs); vitr.left; d_iter_next(&vitr)) {
      b = (d_type(vitr.token) == T_STRING) ? hex_to_new_bytes(d_string(vitr.token), 40) : d_bytesl(vitr.token, 20);
      memset(abi, 0, 32 - b->len);
      memcpy(abi + 32 - b->len, b->data, b->len);
      bb_write_raw_bytes(vbb, abi, 32);
      if (d_type(vitr.token) == T_STRING) _free(b->data);
    }

    if (!bytes_cmp(tmp, vbb->b))
      return vc_err(vc, "wrong data in log");

    _free(proof);
    bb_free(vbb);

    vh_add_state(vh, sitr.token, false);
  }

  vh_free(vh);
  *vhp = vh_init_nodelist(d_get(ctx_->responses[0], K_RESULT));
  ctx_free(ctx_);
  return res;
}

static vhist_engine_t eth_get_engine(in3_vctx_t* vc, bytes_t* header, d_token_t* spec, vhist_t** vh) {
  bytes_t b;

  // try to get from cache
  *vh = vh_cache_retrieve(vc->ctx->client);

  // if no validators in cache, get them from spec
  if (!*vh) {
    *vh = vh_init_spec(spec);
    if (*vh == NULL) {
      vc_err(vc, "Invalid spec");
      return ENGINE_UNKNOWN;
    }
    vh_cache_save(*vh, vc->ctx->client);
  }

  if (vc->last_validator_change > (*vh)->last_change_block) {
    in3_ret_t res = add_aura_validators(vc, vh);
    if (res != IN3_OK) return res;
    vh_cache_save(*vh, vc->ctx->client);
  }

  rlp_decode_in_list(header, BLOCKHEADER_NUMBER, &b);
  return vh_get_engine_for_block(*vh, bytes_to_long(b.data, b.len));
}

static bytes_t* eth_get_validator(bytes_t* header, int* val_len, vhist_t* vh) {
  bytes_builder_t* validators = NULL;
  bytes_t *        proposer, b;

  rlp_decode_in_list(header, BLOCKHEADER_NUMBER, &b);
  validators = vh_get_validators_for_block(vh, bytes_to_long(b.data, b.len));
  if (val_len) *val_len = validators->b.len / 20;

  // the nonce used to find out who's turn it is to sign.
  rlp_decode_in_list(header, BLOCKHEADER_SEALED_FIELD1, &b);

  b.data   = &validators->b.data[(bytes_to_long(b.data, b.len) % (validators->b.len / 20)) * 20];
  b.len    = 20;
  proposer = b_dup(&b);
  bb_free(validators);
  return proposer;
}

in3_ret_t eth_verify_authority(in3_vctx_t* vc, bytes_t** blocks, uint16_t needed_finality, vhist_t* vh) {
  bytes_t tmp, *proposer, *b = blocks[0];
  uint8_t hash[32], signer[20];
  int     val_len = 0, passed = 0, i = 0, ret = 0;

  // check if the parent hashes match
  while (b) {
    // find the validator with permission to sign this block.
    if ((proposer = eth_get_validator(b, b == blocks[0] ? &val_len : NULL, vh)) == NULL)
      return vc_err(vc, "could not find the validator for the block");

    // check signature of proposer
    if (get_aura_signer(vc, b, signer))
      return vc_err(vc, "could not get the signer");

    // check if it was signed by the right validator
    ret = memcmp(signer, proposer->data, 20);
    b_free(proposer);
    if (ret != 0)
      return vc_err(vc, "the block was signed by the wrong key");

    // calculate the blockhash
    sha3_to(b, &hash);

    // next block
    b = blocks[++i];

    // check if the next blocks parent_hash matches
    if (b && (rlp_decode_in_list(b, BLOCKHEADER_PARENT_HASH, &tmp) != 1 || memcmp(hash, tmp.data, 32) != 0))
      return vc_err(vc, "The parent hashes of the finality blocks don't match");

    passed++;
  }

  // we could not find any validators
  if (val_len == 0)
    return vc_err(vc, "no validators");

  return passed * 100 / val_len >= needed_finality ? IN3_OK : vc_err(vc, "not enough blocks to reach finality");
}
#endif

static void add_verified(int max, in3_chain_t* chain, uint64_t number, bytes32_t hash) {
  if (!max) return;
  if (!chain->verified_hashes) chain->verified_hashes = _calloc(max, sizeof(in3_verified_hash_t));
  int      oldest_index  = 0;
  uint64_t oldest_number = 0xFFFFFFFFFFFFFFFFLL;
  for (int i = 0; i < max; i++) {
    if (chain->verified_hashes[i].block_number < oldest_number) {
      oldest_index  = i;
      oldest_number = chain->verified_hashes[i].block_number;
      if (oldest_number == 0) break;
    }
  }
  chain->verified_hashes[oldest_index].block_number = number;
  memcpy(chain->verified_hashes[oldest_index].hash, hash, 32);
}

/** verify the header */
in3_ret_t eth_verify_blockheader(in3_vctx_t* vc, bytes_t* header, bytes_t* expected_blockhash) {

  if (!header)
    return vc_err(vc, "no header found");

  unsigned int i;
  bytes32_t    block_hash;
  uint64_t     header_number = 0;
  d_token_t *  sig, *signatures;
  bytes_t      temp, *sig_hash;

  // generate the blockhash;
  sha3_to(header, &block_hash);

  // if we expect a certain blocknumber, it must match the 8th field in the BlockHeader
  if (rlp_decode_in_list(header, BLOCKHEADER_NUMBER, &temp) == 1)
    header_number = bytes_to_long(temp.data, temp.len);
  else
    return vc_err(vc, "Could not rlpdecode the blocknumber");

  // if we have a blockhash we verify it
  if (expected_blockhash && memcmp(block_hash, expected_blockhash->data, 32))
    return vc_err(vc, "wrong blockhash");

  // already verified?
  if (vc->chain->verified_hashes) {
    for (i = 0; i < vc->ctx->client->max_verified_hashes; i++) {
      if (vc->chain->verified_hashes[i].block_number == header_number) {
        if (memcmp(vc->chain->verified_hashes[i].hash, block_hash, 32))
          return vc_err(vc, "invalid blockhash");
        else
          return IN3_OK;
      }
    }
  }

  // if we expect no signatures ...
  if (vc->config->signers_length == 0) {
#ifdef POA
    in3_ret_t res = IN3_OK;
    vhist_t*  vh  = NULL;
    // ... and the chain is a authority chain....
    if (vc->chain && vc->chain->spec && eth_get_engine(vc, header, vc->chain->spec->result, &vh) == ENGINE_AURA) {
      // we merge the current header + finality blocks
      sig              = d_get(vc->proof, K_FINALITY_BLOCKS);
      bytes_t** blocks = _malloc((sig ? d_len(sig) + 1 : 2) * sizeof(bytes_t*));
      blocks[0]        = header;
      if (sig) {
        for (i = 0; i < d_len(sig); i++) blocks[i + 1] = d_get_bytes_at(sig, i);
      }
      blocks[sig ? d_len(sig) : 1] = NULL;
      // now we verify these block headers
      res = eth_verify_authority(vc, blocks, vc->config->finality, vh);
      _free(blocks);
    }
    vh_free(vh);
    return res;
#endif
  } else if (!(signatures = d_get(vc->proof, K_SIGNATURES)) || d_len(signatures) < vc->config->signers_length)
    // no signatures found,even though we expected some.
    return vc_err(vc, "missing signatures");
  else {
    // prepare the message to be sigfned

    bytes_t msg;
    uint8_t msg_data[96];
    msg.data = (uint8_t*) &msg_data;
    msg.len  = vc->chain->version > 1 ? 96 : 64;

    // first the blockhash + blocknumber
    memcpy(msg_data, block_hash, 32);
    memset(msg_data + 32, 0, 32);
    long_to_bytes(header_number, msg_data + 56);
    if (vc->chain->version > 1) memcpy(msg_data + 64, vc->chain->registry_id, 32);

    // hash it to create the message hash
    sha3_to(&msg, msg_data);
    msg.data = msg_data;
    msg.len  = 32;

    int confirmed = 0; // confirmed is a bitmask for each signature one bit on order to ensure we have all requested signatures
    for (i = 0, sig = signatures + 1; i < (uint32_t) d_len(signatures); i++, sig = d_next(sig)) {
      // only if this signature has the correct blockhash and blocknumber we will verify it.
      if (d_get_longk(sig, K_BLOCK) == header_number && ((sig_hash = d_get_byteskl(sig, K_BLOCK_HASH, 32)) ? memcmp(sig_hash->data, block_hash, 32) == 0 : 1))
        confirmed |= eth_verify_signature(vc, &msg, sig);
    }

    if (confirmed != (1 << vc->config->signers_length) - 1) // we must collect all signatures!
      return vc_err(vc, "missing signatures");

    // ok, is is verified, so we should add it to the verified hashes
    add_verified(vc->ctx->client->max_verified_hashes, vc->chain, header_number, block_hash);
  }

  return IN3_OK;
}
