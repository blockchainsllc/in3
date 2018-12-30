#include "eth_full.h"
#include "../eth_basic/eth_basic.h"
#include "../eth_nano/eth_nano.h"
#include "../eth_nano/merkle.h"
#include "../eth_nano/rlp.h"
#include "../eth_nano/serialize.h"
#include <client/context.h>
#include <client/keys.h>
#include <crypto/ecdsa.h>
#include <crypto/secp256k1.h>
#include <string.h>
#include <util/data.h>
#include <util/mem.h>
#include <util/utils.h>

int in3_verify_eth_full(in3_vctx_t* vc) {
  char* method = d_get_stringk(vc->request, K_METHOD);
  if (vc->config->verification == VERIFICATION_NEVER)
    return 0;

  // do we have a result? if not it is a vaslid error-response
  if (!vc->result)
    return 0;

  // do we support this request?
  if (!method)
    return vc_err(vc, "No Method in request defined!");

  if (strcmp(method, "eth_call") == 0)
    // for txReceipt, we need the txhash
    return vc_err(vc, "Not implemented yet");
  else
    return in3_verify_eth_basic(vc);
}

void in3_register_eth_full() {
  in3_verifier_t* v = _calloc(1, sizeof(in3_verification_t));
  v->type           = CHAIN_ETH;
  v->verify         = in3_verify_eth_full;
  in3_register_verifier(v);
}
