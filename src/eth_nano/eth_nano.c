#include "eth_nano.h"
#include <util/utils.h>
#include <client/context.h>
#include <client/verifier.h>
#include <string.h>
#include "rlp.h"
#include "merkle.h"
#include "serialize.h"
#include <crypto/secp256k1.h>
#include <crypto/ecdsa.h>
#include <util/mem.h>
#include <client/keys.h>

// list of methods allowed withoput proof
#define MAX_METHODS  23
char *ALLOWED_METHODS[MAX_METHODS] = {"eth_blockNumber", "web3_clientVersion", "web3_sha3", "net_version", "net_peerCount", "net_listening", "eth_protocolVersion", "eth_syncing", "eth_coinbase", "eth_mining", "eth_hashrate", "eth_gasPrice", "eth_accounts", "eth_sign", "eth_sendRawTransaction", "eth_estimateGas", "eth_getCompilers", "eth_compileLLL", "eth_compileSolidity", "eth_compileSerpent", "eth_getWork", "eth_submitWork", "eth_submitHashrate"};






int in3_verify_eth_nano(in3_vctx_t *vc)
{
    char* method;
    d_token_t* params = d_get(vc->request, K_PARAMS);
    int i;

    if (vc->config->verification == VERIFICATION_NEVER)
        return 0;

    // do we have a result? if not it is a vaslid error-response
    if (!vc->result)
        return 0;

    // do we support this request?
    if (!(method = d_get_stringk(vc->request, K_METHOD)))
        return vc_err(vc, "No Method in request defined!");

    // check if this call is part of the not verifieable calls
    for (i=0;i<MAX_METHODS;i++) {
       if (strcmp(ALLOWED_METHODS[i],method)==0) 
          return 0;
    }

    if (strcmp(method, "eth_getTransactionReceipt")==0)
        // for txReceipt, we need the txhash
        return eth_verify_eth_getTransactionReceipt(vc, d_get_bytes_at(params,0));
    else
        return vc_err(vc, "The Method cannot be verified with eth_nano!");

}

void in3_register_eth_nano()
{
    in3_verifier_t *v = _calloc(1, sizeof(in3_verification_t));
    v->type = CHAIN_ETH;
    v->verify = in3_verify_eth_nano;
    in3_register_verifier(v);
}
