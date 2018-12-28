#include <util/mem.h>
#include "../eth_nano/eth_nano.h"
#include "eth_basic.h"
#include <util/utils.h>
#include <client/context.h>
#include <string.h>
#include "../eth_nano/rlp.h"
#include "../eth_nano/merkle.h"
#include "../eth_nano/serialize.h"
#include <crypto/secp256k1.h>
#include <crypto/ecdsa.h>
#include <util/data.h>
#include <client/keys.h>


int in3_verify_eth_basic(in3_vctx_t *vc)
{
    char* method = d_get_stringk(vc->request, K_METHOD);

    if (vc->config->verification == VERIFICATION_NEVER)
        return 0;

    // do we have a result? if not it is a vaslid error-response
    if (!vc->result || d_type(vc->result)==T_NULL)
        return 0;

    // do we support this request?
    if (!method)
        return vc_err(vc, "No Method in request defined!");

    if (strcmp(method,"eth_getTransactionByHash")==0)
        // for txReceipt, we need the txhash
        return eth_verify_eth_getTransaction(vc, d_get_bytes_at( d_get(vc->request, K_PARAMS) ,0));
    else if (strcmp(method,"eth_getBlockByNumber")==0)
        // for txReceipt, we need the txhash
        return eth_verify_eth_getBlock(vc, NULL, d_get_long_at(  d_get(vc->request, K_PARAMS) ,0));
    else if (strcmp(method,"eth_getBlockByHash")==0)
        // for txReceipt, we need the txhash
        return eth_verify_eth_getBlock(vc, d_get_bytes_at(  d_get(vc->request, K_PARAMS) ,0),0);
    else
        return in3_verify_eth_nano(vc);
}

void in3_register_eth_basic()
{
    in3_verifier_t *v = _calloc(1, sizeof(in3_verification_t));
    v->type = CHAIN_ETH;
    v->verify = in3_verify_eth_basic;
    in3_register_verifier(v);
}

