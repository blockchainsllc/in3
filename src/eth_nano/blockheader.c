#include "eth_nano.h"
#include "merkle.h"
#include "rlp.h"
#include "serialize.h"
#include <client/context.h>
#include <client/keys.h>
#include <crypto/ecdsa.h>
#include <crypto/secp256k1.h>
#include <string.h>
#include <util/mem.h>
#include <util/utils.h>

static bytes_t* eth_get_validator(in3_vctx_t* vc, bytes_t* header, d_token_t* spec, int* val_len) {
  d_token_t* tmp        = NULL;
  bytes_t ** validators = NULL, *proposer, b;
  int        validator_len, i;

  // if we have a fixed validator list,
  if ((tmp = d_get(spec, K_VALIDATOR_LIST))) {
    // create a validator list from spec.
    validator_len = d_len(tmp);
    validators    = _malloc(sizeof(bytes_t*) * validator_len);
    // copy references
    for (i = 0, tmp += 1; i < validator_len; i++, tmp = d_next(tmp)) validators[i] = d_bytes(tmp);
    // copy the size of the validators to the given pointer, because it will be needed to ensure finality.
    if (val_len) *val_len = validator_len;

  } else {
    // TODO read the validators from the chain.
    vc_err(vc, "currently only static validators are supported");
    return NULL;
  }

  // the nonce used to find out who's turn it is to sign.
  rlp_decode_in_list(header, BLOCKHEADER_SEALED_FIELD1, &b);
  proposer = validators[bytes_to_long(b.data, 4) % validator_len];
  _free(validators);
  return proposer;
}

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
    if (b && (rlp_decode_in_list(b, 0, &tmp) != 1 || memcmp(hash, tmp.data, 32) != 0))
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
