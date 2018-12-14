#include "eth_nano.h"
#include <util/utils.h>
#include <client/context.h>
#include <string.h>
#include "rlp.h"

int eth_verify_blockheader(  in3_ctx_t* ctx, in3_chain_t* chain,in3_request_config_t* request_config, bytes_t* header, jsmntok_t* expected_blockhash, jsmntok_t* proof) {
    int res=0,i;
    uint64_t header_number=0;
    jsmntok_t* t,*sig,*t2;
    bytes_t* block_hash = sha3(header);
    bytes_t temp;

    if (rlp_decode_item(header,0,&temp) && rlp_decode_item(&temp,8,&temp)) {
        // this should be the list for the blockheader
       memcpy(((uint8_t*)&header_number)+8-temp.len,temp.data,temp.len);
    }
    else
      res = ctx_set_error(ctx,"Could not rlpdecode the blocknumber",-1);

    if (res==0 && expected_blockhash) {
        bytes_t* expected = ctx_to_bytes(ctx->response_data,expected_blockhash,32);
        if (!b_cmp(block_hash, expected)) res = ctx_set_error(ctx,"wrong blockhash",-1);
        b_free(expected);
    }

    if (res==0 && request_config->signaturesCount==0) {
      // for proof of authorities we can verify the signatures
      // TODO
//        if (ctx && ctx.chainSpec && ctx.chainSpec.engine==='authorityRound') {
//        const finality = await checkBlockSignatures([b, ...(proof.proof && proof.proof.finalityBlocks || [])],_=>getChainSpec(_,ctx))
//        if  (proof.finality &&  proof.finality > finality)
//            throw new Error('we have only a finality of '+finality+' but expected was '+proof.finality)
//        }

    }
    else if (res==0 && (!(t=ctx_get_token(ctx->response_data,proof,"signatures")) || t->size<request_config->signaturesCount))
      res = ctx_set_error(ctx,"missing signatures",-1);
    else if (res==0){
        int confirmed=0;
        for (i=0;i<t->size;i++) {
            sig = ctx_get_array_token(ctx->response_data,t,i);
            if ((t=ctx_get_token(ctx->response_data,sig,"block"))) {
                uint64_t block_number = ctx_to_long(ctx->response_data,t,0);
                if (block_number==header_number) {
                    // TODO check sig

                    confirmed++;
                }
            }
        }
        if (confirmed<request_config->signaturesCount) 
           res = ctx_set_error(ctx,"missing signatures",-1);
    }


    return res;
}


int in3_verify_eth_getTransactionReceipt(in3_ctx_t* ctx , in3_chain_t* chain, jsmntok_t* tx_hash, 
    in3_request_config_t* request_config, jsmntok_t* result, jsmntok_t* proof ) {

    int res=0;
    jsmntok_t* in3,*t;

    if (!tx_hash) return ctx_set_error(ctx,"No Transaction Hash found",-1);
    if (tx_hash->end- tx_hash->start!=66) return ctx_set_error(ctx,"The transactionHash has the wrong length!",-1);

    // this means result: null, which is ok, since we can not verify a transaction that does not exists
    if (result->type==JSMN_PRIMITIVE) return 0;

    if (!proof)  return ctx_set_error(ctx,"Proof is missing!",-1);
    if (!(t=ctx_get_token(ctx->response_data,proof,"block"))) 
       return ctx_set_error(ctx,"No Block-Proof!",-1);


    bytes_t* blockHeader = ctx_to_bytes(ctx->response_data, t,32);
    res = eth_verify_blockheader(ctx, chain, request_config, blockHeader, ctx_get_token(ctx->response_data,result,"blockHash"), proof);
    b_free(blockHeader);

    if (res==0) {
        bytes_t* txHash = ctx_to_bytes(ctx->response_data, tx_hash,32);

        // TODO check merkle tree
        b_free(txHash);
    }


    return res;
} 



int in3_verify_eth_nano( in3_ctx_t* ctx , in3_chain_t* chain, jsmntok_t* request, in3_request_config_t* request_config, jsmntok_t* response) {
    jsmntok_t* t, *result, *proof;

    if (request_config->verification==VERIFICATION_NEVER) return 0;
    // do we support this request?
    if (!(t=ctx_get_token(ctx->request_data,request,"method"))) 
       return ctx_set_error(ctx,"No Method in request defined!",-1);
    if (!ctx_equals(ctx->request_data,t,"eth_getTransactionReceipt")) 
       return ctx_set_error(ctx,"The Method cannot be verified with eth_nano!",-1);

    // do we have a result? if not it is a vaslid error-response
    if (!(result=ctx_get_token(ctx->response_data,response,"result"))) 
      return 0;

    if (!(t=ctx_get_token(ctx->request_data,request,"params")) || t->size<1) 
       return ctx_set_error(ctx,"No params in request defined!",-1);

    if ((proof=ctx_get_token(ctx->response_data,response,"in3"))) 
       proof = ctx_get_token(ctx->response_data,proof,"proof");
    
    
    // for txReceipt, we need the txhash
    return in3_verify_eth_getTransactionReceipt(ctx, chain,  ctx_get_array_token(ctx->request_data,t,0),request_config ,result, proof);
}









void in3_register_eth_nano() {
    in3_verifier_t* v = calloc(1,sizeof(in3_verification_t));
    v->type = CHAIN_ETH;
    v->verify = in3_verify_eth_nano;
    in3_register_verifier(v);
}
