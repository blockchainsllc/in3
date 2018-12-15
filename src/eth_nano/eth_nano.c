#include "eth_nano.h"
#include <util/utils.h>
#include <client/context.h>
#include <string.h>
#include "rlp.h"

int eth_verify_blockheader( in3_vctx_t* vc, bytes_t* header, jsmntok_t* expected_blockhash) {
    int res=0,i;
    uint64_t header_number=0;
    jsmntok_t* t,*sig,*t2;
    bytes_t* block_hash = sha3(header);
    bytes_t temp;

    if (!header) return vc_err(vc,"no header found");

    if (rlp_decode(header,0,&temp) && rlp_decode(&temp,8,&temp)) 
       header_number = bytes_to_long(temp.data,temp.len);
    else
      res = vc_err(vc,"Could not rlpdecode the blocknumber");

    if (res==0 && expected_blockhash) {
        bytes_t* expected = res_to_bytes(vc,expected_blockhash);
        if (!b_cmp(block_hash, expected)) res = vc_err(vc,"wrong blockhash");
        b_free(expected);
    }

    if (res==0 && vc->config->signaturesCount==0) {
      // for proof of authorities we can verify the signatures
      // TODO
//        if (ctx && ctx.chainSpec && ctx.chainSpec.engine==='authorityRound') {
//        const finality = await checkBlockSignatures([b, ...(proof.proof && proof.proof.finalityBlocks || [])],_=>getChainSpec(_,ctx))
//        if  (proof.finality &&  proof.finality > finality)
//            throw new Error('we have only a finality of '+finality+' but expected was '+proof.finality)
//        }

    }
    else if (res==0 && (!(t= res_get(vc,vc->proof,"signatures")) || t->size<vc->config->signaturesCount))
      res = vc_err(vc,"missing signatures");
    else if (res==0){
        int confirmed=0;
        for (i=0;i<t->size;i++) {
            sig = ctx_get_array_token(t,i);
            if ((t= res_get(vc,sig,"block"))) {
                uint64_t block_number = res_to_long(vc,t,0);
                if (block_number==header_number) {
                    // TODO check sig
                    

                    confirmed++;
                }
            }
        }
        if (confirmed< vc->config->signaturesCount) 
           res = vc_err(vc,"missing signatures");
    }


    return res;
}


int in3_verify_eth_getTransactionReceipt(in3_vctx_t* vc, jsmntok_t* tx_hash ) {

    int res=0;
    jsmntok_t* in3,*t;

    if (!tx_hash) return vc_err(vc,"No Transaction Hash found");
    if (tx_hash->end- tx_hash->start!=66) return vc_err(vc,"The transactionHash has the wrong length!");

    // this means result: null, which is ok, since we can not verify a transaction that does not exists
    if (vc->result->type==JSMN_PRIMITIVE) return 0;

    if (!vc->proof)  return vc_err(vc,"Proof is missing!");
    if (!(t= res_get(vc,vc->proof,"block"))) 
       return vc_err(vc,"No Block-Proof!");


    bytes_t* blockHeader = res_to_bytes(vc, t);
    res = eth_verify_blockheader(vc, blockHeader, res_get(vc,vc->result,"blockHash"));
    b_free(blockHeader);

    if (res==0) {
        bytes_t* txHash = res_to_bytes(vc,tx_hash);

        // TODO check merkle tree
        b_free(txHash);
    }


    return res;
} 



int in3_verify_eth_nano( in3_vctx_t* vc) {
    jsmntok_t* t;

    if (vc->config->verification==VERIFICATION_NEVER) return 0;

    // do we have a result? if not it is a vaslid error-response
    if (!vc->result) 
      return 0;

    // do we support this request?
    if (!(t=req_get(vc,vc->request,"method")))
       return vc_err(vc,"No Method in request defined!");
    if (!req_eq(vc,t,"eth_getTransactionReceipt")) 
       return vc_err(vc,"The Method cannot be verified with eth_nano!");


    // for txReceipt, we need the txhash
    return in3_verify_eth_getTransactionReceipt(vc, req_get_param(vc,0));
}









void in3_register_eth_nano() {
    in3_verifier_t* v = calloc(1,sizeof(in3_verification_t));
    v->type = CHAIN_ETH;
    v->verify = in3_verify_eth_nano;
    in3_register_verifier(v);
}
