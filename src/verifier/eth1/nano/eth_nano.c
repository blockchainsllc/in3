#include "eth_nano.h"
#include "../../../core/client/context.h"
#include "../../../core/client/keys.h"
#include "../../../core/client/verifier.h"
#include "../../../core/util/mem.h"
#include "../../../third-party/crypto/ecdsa.h"
#include "merkle.h"
#include "serialize.h"
#include <string.h>

// list of methods allowed withoput proof
#define MAX_METHODS 24
char* ALLOWED_METHODS[MAX_METHODS] = {"in3_stats", "eth_blockNumber", "web3_clientVersion", "web3_sha3", "net_version", "net_peerCount", "net_listening", "eth_protocolVersion", "eth_syncing", "eth_coinbase", "eth_mining", "eth_hashrate", "eth_gasPrice", "eth_accounts", "eth_sign", "eth_sendRawTransaction", "eth_estimateGas", "eth_getCompilers", "eth_compileLLL", "eth_compileSolidity", "eth_compileSerpent", "eth_getWork", "eth_submitWork", "eth_submitHashrate"};

in3_ret_t in3_verify_eth_nano(in3_vctx_t* vc) {
  char*      method = NULL;
  d_token_t* params = d_get(vc->request, K_PARAMS);
  int        i;

  if (vc->config->verification == VERIFICATION_NEVER)
    return IN3_OK;

  // do we have a result? if not it is a vaslid error-response
  if (!vc->result)
    return IN3_OK;

  // do we support this request?
  if (!(method = d_get_stringk(vc->request, K_METHOD)))
    return vc_err(vc, "No Method in request defined!");

  // check if this call is part of the not verifieable calls
  for (i = 0; i < MAX_METHODS; i++) {
    if (strcmp(ALLOWED_METHODS[i], method) == 0)
      return IN3_OK;
  }

  if (strcmp(method, "eth_getTransactionReceipt") == 0)
    // for txReceipt, we need the txhash
    return eth_verify_eth_getTransactionReceipt(vc, d_get_bytes_at(params, 0));
  else if (strcmp(method, "in3_nodeList") == 0)
    return eth_verify_in3_nodelist(vc, d_get_int_at(params, 0), d_get_bytes_at(params, 1), d_get_at(params, 2));
  else
    return vc_err(vc, "The Method cannot be verified with eth_nano!");
}

void in3_register_eth_nano() {
  in3_verifier_t* v = _calloc(1, sizeof(in3_verifier_t));
  v->type           = CHAIN_ETH;
  v->verify         = in3_verify_eth_nano;
  in3_register_verifier(v);
}
