#include "eth_nano.h"
#include "merkle.h"
#include "rlp.h"
#include "serialize.h"
#include "vhist.h"
#include <client/context.h>
#include <client/keys.h>
#include <crypto/ecdsa.h>
#include <crypto/secp256k1.h>
#include <string.h>
#include <util/log.h>
#include <util/mem.h>
#include <util/utils.h>

#define VALIDATOR_LIST_KEY ("validatorlist_%" PRIx64)
#define VALIDATOR_DIFF_KEY ("validatordiff_%" PRIx64)

static in3_ret_t get_signer(in3_vctx_t* vc, bytes_t* header, uint8_t* dst) {
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
  in3_ctx_t* ctx_        = in3_client_rpc_ctx(vc->ctx->client, "in3_validatorlist", "[]");
  vc->ctx->client->proof = proof_;
  res                    = ctx_get_error(ctx_, 0);
  if (res != IN3_OK) {
    free_ctx(ctx_);
    return vc_err(vc, ctx_->error);
  }

  // Validate proof
  d_token_t *ss = d_get(ctx_->responses[0], K_STATES), *prf = NULL;
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
    bytes_builder_t* curr     = vh_get_for_block(vh, prf_blkno);

    while (fblk) {
      // check signature of proposer
      if (get_signer(vc, fblk, signer))
        return vc_err(vc, "could not get the signer");

      // check if it was signed by the right validator
      rlp_decode_in_list(fblk, BLOCKHEADER_SEALED_FIELD1, &tmp);
      proposer = &curr->b.data[bytes_to_long(tmp.data, tmp.len) % (curr->b.len / 20)];
      if (memcmp(signer, proposer, 20) != 0) return vc_err(vc, "the block was signed by the wrong key");

      // calculate the blockhash
      sha3_to(fblk, &hash);

      // next block
      fblk = blocks[++i];

      // check if the next blocks parent_hash matches
      if (fblk && (rlp_decode_in_list(fblk, BLOCKHEADER_PARENT_HASH, &tmp) != 1 || memcmp(hash, tmp.data, 32) != 0))
        return vc_err(vc, "The parent hashes of the finality blocks don't match");

      passed++;
    }

    if (passed * 100 / (curr->b.len / 20) < vc->config->finality)
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

    bytes_t log_data;
    rlp_decode_in_list(&raw_receipt, rlp_decode_len(&raw_receipt) - 1, &log_data);
    rlp_decode_in_list(&log_data, 0, &tmp);
    if (!b_cmp(&tmp, d_get_bytesk(vc->chain->spec->result, K_VALIDATOR_CONTRACT)))
      return vc_err(vc, "Wrong address in log");

    rlp_decode_in_list(&log_data, 1, &tmp);
    rlp_decode_in_list(&tmp, 0, &tmp);
    bytes_t t = b_from_hexstr("0x55252fa6eee4741b4e24a74a70e9c11fd2c2281df8d6ea13126ff845f7825c89");
    if (!bytes_cmp(tmp, t))
      return vc_err(vc, "Wrong topic in log");

    _free(t.data);

    rlp_decode_in_list(&log_data, 2, &tmp);
    bytes_t         b;
    bytes_builder_t vbb;
    for (d_iterator_t vitr = d_iter(d_get(sitr.token, K_VALIDATORS)); vitr.left; d_iter_next(&vitr)) {
      b = (d_type(vitr.token) == T_STRING) ? b_from_hexstr(d_string(vitr.token)) : *d_bytesl(vitr.token, 20);
      bb_write_fixed_bytes(&vbb, &b);
      if (d_type(vitr.token) == T_STRING) _free(b.data);
    }
    if (!bytes_cmp(tmp, vbb.b))
      return vc_err(vc, "wrong data in log");

    _free(raw_receipt.data);
    _free(proof);

    vh_add_state(vh, sitr.token);
  }

  vh_free(vh);
  vh = vh_init(d_get(ctx_->responses[0], K_RESULT));
  free_ctx(ctx_);
  return res;
}

static bytes_t* eth_get_validator(in3_vctx_t* vc, bytes_t* header, d_token_t* spec, int* val_len) {
  d_token_t*       tmp        = NULL;
  bytes_builder_t* validators = NULL;
  bytes_t *        proposer, b;
  size_t           i;
  vhist_t*         vh     = NULL;
  char             k[200] = {0};

  // if we have a fixed validator list,
  if ((tmp = d_get(spec, K_VALIDATOR_LIST)) && !d_get(spec, K_VALIDATOR_CONTRACT)) {
    size_t l = d_len(tmp);
    // create a validator list from spec.
    validators = bb_newl(l * 20);
    // copy references
    for (i = 0, tmp += 1; i < l; i++, tmp = d_next(tmp))
      bb_write_fixed_bytes(validators, d_bytesl(tmp, 20));
    // copy the size of the validators to the given pointer, because it will be needed to ensure finality.
    if (val_len) *val_len = l;
  } else {
    // try to get from cache
    bytes_t *v_ = NULL, *d_ = NULL;
    if (vc->ctx->client->cacheStorage) {
      sprintf(k, VALIDATOR_LIST_KEY, vc->chain->chainId);
      v_ = vc->ctx->client->cacheStorage->get_item(vc->ctx->client->cacheStorage->cptr, k);
      sprintf(k, VALIDATOR_DIFF_KEY, vc->chain->chainId);
      d_ = vc->ctx->client->cacheStorage->get_item(vc->ctx->client->cacheStorage->cptr, k);
    }

    // if no validators in cache, get them from spec
    if (v_) {
      vh = _malloc(sizeof(*vh));
      if (vh == NULL) return NULL;
      vh->vldtrs->b.data = v_->data;
      vh->vldtrs->b.len  = v_->len;
      vh->diffs->b.data  = d_->data;
      vh->diffs->b.len   = d_->len;
      _free(v_);
      _free(d_);
    } else {
      vh = vh_init(spec);
      if (vh == NULL) {
        vc_err(vc, "Invalid spec");
        return NULL;
      }
    }

    if (vc->ctx->last_validator_change > vh->last_change_block) {
      add_aura_validators(vc, &vh);

      // save to cache
      sprintf(k, VALIDATOR_LIST_KEY, vc->chain->chainId);
      vc->ctx->client->cacheStorage->set_item(vc->ctx->client->cacheStorage->cptr, k, &vh->vldtrs->b);
      sprintf(k, VALIDATOR_DIFF_KEY, vc->chain->chainId);
      vc->ctx->client->cacheStorage->set_item(vc->ctx->client->cacheStorage->cptr, k, &vh->diffs->b);
    }

    vh_free(vh);

    rlp_decode_in_list(header, BLOCKHEADER_NUMBER, &b);
    validators = vh_get_for_block(vh, bytes_to_long(b.data, b.len));
  }

  // the nonce used to find out who's turn it is to sign.
  rlp_decode_in_list(header, BLOCKHEADER_SEALED_FIELD1, &b);
  proposer = b_dup(&validators[bytes_to_long(b.data, 4) % (validators->b.len / 20)].b);
  bb_free(validators);
  return proposer;
}

in3_ret_t eth_verify_authority(in3_vctx_t* vc, bytes_t** blocks, d_token_t* spec, uint16_t needed_finality) {
  bytes_t tmp, *proposer, *b = blocks[0];
  uint8_t hash[32], signer[20];
  int     val_len = 0, passed = 0, i = 0;

  // check if the parent hashes match
  while (b) {
    // find the validator with permission to sign this block.
    if ((proposer = eth_get_validator(vc, b, spec, b == blocks[0] ? &val_len : NULL)) == NULL)
      return vc_err(vc, "could not find the validator for the block");

    // check signature of proposer
    if (get_signer(vc, b, signer))
      return vc_err(vc, "could not get the signer");

    // check if it was signed by the right validator
    if (memcmp(signer, proposer->data, 20) != 0)
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

  return passed * 100 / val_len >= needed_finality ? IN3_OK : vc_err(vc, "not enought blocks to reach finality");
}

/** verify the header */
in3_ret_t eth_verify_blockheader(in3_vctx_t* vc, bytes_t* header, bytes_t* expected_blockhash) {

  if (!header)
    return vc_err(vc, "no header found");

  in3_ret_t  res = IN3_OK;
  int        i;
  uint8_t    block_hash[32];
  uint64_t   header_number = 0;
  d_token_t *sig, *signatures;
  bytes_t    temp, *sig_hash;

  // generate the blockhash;
  sha3_to(header, &block_hash);

  // if we expect a certain blocknumber, it must match the 8th field in the BlockHeader
  if (res == IN3_OK && (rlp_decode_in_list(header, BLOCKHEADER_NUMBER, &temp) == 1))
    header_number = bytes_to_long(temp.data, temp.len);
  else
    res = vc_err(vc, "Could not rlpdecode the blocknumber");

  // if we have a blockhash we verify it
  if (res == IN3_OK && expected_blockhash && memcmp(block_hash, expected_blockhash->data, 32))
    res = vc_err(vc, "wrong blockhash");

  // if we expect no signatures ...
  if (res == IN3_OK && vc->config->signaturesCount == 0) {

    // ... and the chain is a authority chain....
    if (vc->chain && vc->chain->spec && (sig = d_get(vc->chain->spec->result, K_ENGINE)) && strcmp(d_string(sig), "authorityRound") == 0) {
      // we merge the current header + finality blocks
      sig              = d_get(vc->proof, K_FINALITY_BLOCKS);
      bytes_t** blocks = _malloc((sig ? d_len(sig) + 1 : 2) * sizeof(bytes_t*));
      blocks[0]        = header;
      if (sig) {
        for (i = 0; i < d_len(sig); i++) blocks[i + 1] = d_get_bytes_at(sig, i);
      }
      blocks[sig ? d_len(sig) : 1] = NULL;
      // now we verify these block headers
      res = eth_verify_authority(vc, blocks, vc->chain->spec->result, vc->config->finality);
      _free(blocks);
    } else // we didn't request signatures so blockheader should be ok.
      res = IN3_OK;
  } else if (res == IN3_OK && (!(signatures = d_get(vc->proof, K_SIGNATURES)) || d_len(signatures) < vc->config->signaturesCount))
    // no signatures found,even though we expected some.
    res = vc_err(vc, "missing signatures");
  else if (res == IN3_OK) {
    // prepare the message to be sigfned
    bytes_t msg;
    uint8_t msg_data[64];
    msg.data = (uint8_t*) &msg_data;
    msg.len  = 64;
    // first the blockhash + blocknumber
    memcpy(msg_data, block_hash, 32);
    memset(msg_data + 32, 0, 32);
    long_to_bytes(header_number, msg_data + 56);

    // hash it to create the message hash
    sha3_to(&msg, msg_data);
    msg.data = msg_data;
    msg.len  = 32;

    int confirmed = 0; // confirmed is a bitmask for each signature one bit on order to ensure we have all requested signatures
    for (i = 0, sig = signatures + 1; i < d_len(signatures); i++, sig = d_next(sig)) {
      // only if this signature has the correct blockhash and blocknumber we will verify it.
      if (d_get_longk(sig, K_BLOCK) == header_number && ((sig_hash = d_get_byteskl(sig, K_BLOCK_HASH, 32)) ? memcmp(sig_hash->data, block_hash, 32) == 0 : 1))
        confirmed |= eth_verify_signature(vc, &msg, sig);
    }

    if (confirmed != (1 << vc->config->signaturesCount) - 1) // we must collect all signatures!
      res = vc_err(vc, "missing signatures");
  }

  return res;
}
