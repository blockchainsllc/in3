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
    // for proof of authorities we can verify the signatures
    // TODO
    //        if (ctx && ctx.chainSpec && ctx.chainSpec.engine==='authorityRound') {
    //        const finality = await checkBlockSignatures([b, ...(proof.proof && proof.proof.finalityBlocks || [])],_=>getChainSpec(_,ctx))
    //        if  (proof.finality &&  proof.finality > finality)
    //            throw new Error('we have only a finality of '+finality+' but expected was '+proof.finality)
    //        }
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
