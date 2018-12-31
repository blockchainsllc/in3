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

bytes_t* eth_get_validator(in3_vctx_t* vc, bytes_t* header, d_token_t* spec, int* val_len) {
  d_token_t* tmp;
  bytes_t    b;
  bytes_t**  validators = NULL;
  bytes_t*   proposer;
  int        validator_len, i;
  if ((tmp = d_get(spec, K_VALIDATOR_LIST))) {
    validator_len = d_len(tmp);
    if (val_len) *val_len = validator_len;
    validators = _malloc(sizeof(bytes_t*) * validator_len);
    for (i = 0, tmp += 1; i < validator_len; i++, tmp = d_next(tmp)) validators[i] = d_bytes(tmp);
  } else {
    vc_err(vc, "currently only static validators are supported");
    return NULL;
  }

  rlp_decode_in_list(header, 13, &b);
  proposer = validators[bytes_to_long(b.data, 4) % validator_len];
  _free(validators);
  return proposer;
}

static int get_signer(in3_vctx_t* vc, bytes_t* header, uint8_t* dst) {
  bytes_t         sig, bare;
  bytes_builder_t ll;
  uint8_t         d[4], bare_hash[32], pub_key[65];

  // get the raw data without the sealed field
  rlp_decode(header, 0, &bare);
  rlp_decode(&bare, 12, &sig);
  bare.len = sig.len + sig.data - bare.data;

  // calculate the list prefix
  ll.bsize  = 4;
  ll.b.len  = 0;
  ll.b.data = (uint8_t*) &d;
  rlp_add_length(&ll, bare.len, 0xc0);

  // calculate the bare hash
  struct SHA3_CTX ctx;
  sha3_256_Init(&ctx);
  sha3_Update(&ctx, ll.b.data, ll.b.len);
  sha3_Update(&ctx, bare.data, bare.len);
  keccak_Final(&ctx, bare_hash);

  if (rlp_decode(header, 15, &sig) == 1) { // we have 3 sealed fields the messagehash is calculated hash = sha3( concat ( bare_hash | rlp_encode ( sealed_fields[2] ) ) )
    bb_clear(&ll);
    rlp_add_length(&ll, sig.len, 0xc0);

    sha3_256_Init(&ctx);
    sha3_Update(&ctx, bare_hash, 32);
    sha3_Update(&ctx, ll.b.data, ll.b.len);
    sha3_Update(&ctx, sig.data, sig.len);
    keccak_Final(&ctx, bare_hash);
  }

  rlp_decode(header, 14, &sig);

  // recover signature
  if (ecdsa_recover_pub_from_sig(&secp256k1, pub_key, sig.data, bare_hash, sig.data[64]))
    return vc_err(vc, "The signature of a validator could not recover!");

  sha3_256_Init(&ctx);
  sha3_Update(&ctx, pub_key + 1, 64);
  keccak_Final(&ctx, bare_hash);

  memcpy(dst, bare_hash + 12, 20);

  return 0;
}
/*
export function getSigner(data: Block):Buffer {
  const signature: Buffer = data.sealedFields[1];
  const message = data.sealedFields.length === 3 ? hash(Buffer.concat([data.bareHash(), rlp.encode(data.sealedFields[2])])) : data.bareHash();
  return publicToAddress(recover(message, signature.slice(0, 64), signature[64]), true);
}
*/

int eth_verify_authority(in3_vctx_t* vc, bytes_t** blocks, d_token_t* spec, uint16_t needed_finality) {
  bytes_t* b = blocks[0];
  uint8_t  hash[32], signer[20];
  bytes_t  tmp;
  bytes_t* proposer;
  int      val_len = 0, passed = 0, i = 0;

  // check if the parent hashes match
  while (b) {
    if ((proposer = eth_get_validator(vc, b, spec, b == blocks[0] ? &val_len : NULL)) == NULL) return vc_err(vc, "could not find the validator for the block");

    if (needed_finality > 100) { // for now it is just deactivated
      // check signature of proposer
      if (get_signer(vc, b, signer)) return vc_err(vc, "could not get the signer");

      // check if it was signed by the right validator
      if (memcmp(signer, proposer->data, 20) != 0) return vc_err(vc, "the block was signed by the wrong key");
    }
    // calculate the blockhash
    sha3_to(b, &hash);

    // next block
    b = blocks[++i];
    // check if the next blocks parent_hash matches
    if (b && (rlp_decode_in_list(b, 0, &tmp) != 1 || memcmp(hash, tmp.data, 32) != 0))
      return vc_err(vc, "The parent hashes of the finality blocks don't match");

    passed++;
  }

  if (val_len == 0) return vc_err(vc, "no validators");

  return passed * 100 / val_len >= needed_finality ? 0 : vc_err(vc, "not enought blocks to reach finality");
}

int eth_verify_blockheader(in3_vctx_t* vc, bytes_t* header, bytes_t* expected_blockhash) {
  int res = 0, i;

  if (!header)
    return vc_err(vc, "no header found");

  uint64_t   header_number = 0;
  d_token_t *sig, *signatures;
  bytes_t*   block_hash = sha3(header);
  bytes_t    temp, *sig_hash;

  if (!res && rlp_decode(header, 0, &temp) && rlp_decode(&temp, 8, &temp))
    header_number = bytes_to_long(temp.data, temp.len);
  else
    res = vc_err(vc, "Could not rlpdecode the blocknumber");

  if (res == 0 && expected_blockhash && !b_cmp(block_hash, expected_blockhash))
    res = vc_err(vc, "wrong blockhash");

  if (res == 0 && vc->config->signaturesCount == 0) {
    if (vc->chain && vc->chain->spec && (sig = d_get(vc->chain->spec->items, K_ENGINE)) && strcmp(d_string(sig), "authorityRound") == 0) {
      sig              = d_get(vc->proof, K_FINALITY_BLOCKS);
      bytes_t** blocks = _malloc((sig ? d_len(sig) + 1 : 2) * sizeof(bytes_t*));
      blocks[0]        = header;
      if (sig) {
        for (i = 0; i < d_len(sig); i++) blocks[i + 1] = d_get_bytes_at(sig, i);
      }
      blocks[sig ? d_len(sig) : 1] = NULL;
      res                          = eth_verify_authority(vc, blocks, vc->chain->spec->items, vc->config->finality);
      _free(blocks);
    } else // we didn't request signatures so blockheader should be ok.
      res = 0;
  } else if (res == 0 && (!(signatures = d_get(vc->proof, K_SIGNATURES)) || d_len(signatures) < vc->config->signaturesCount))
    res = vc_err(vc, "missing signatures");
  else if (res == 0) {
    // prepare the message to be sigfned
    bytes_t msg;
    uint8_t msg_data[64];
    msg.data = (uint8_t*) &msg_data;
    msg.len  = 64;
    // first the blockhash + blocknumbero
    memcpy(msg_data, block_hash->data, 32);
    memset(msg_data + 32, 0, 32);
    long_to_bytes(header_number, msg_data + 56);
    bytes_t* msg_hash = sha3(&msg);

    int confirmed = 0; // confiremd is a bitmask for each signature one bit on order to ensure we have all requested signatures
    for (i = 0, sig = signatures + 1; i < d_len(signatures); i++, sig = d_next(sig)) {
      if (d_get_longk(sig, K_BLOCK) == header_number && ((sig_hash = d_get_bytesk(sig, K_BLOCK_HASH)) ? b_cmp(sig_hash, block_hash) : 1))
        confirmed |= eth_verify_signature(vc, msg_hash, sig);
    }

    b_free(msg_hash);
    if (confirmed != (1 << vc->config->signaturesCount) - 1) // we must collect all signatures!
      res = vc_err(vc, "missing signatures");
  }

  b_free(block_hash);

  return res;
}
