#include "eth_nano.h"
#include <util/utils.h>
#include <client/context.h>
#include <string.h>
#include "rlp.h"
#include <crypto/secp256k1.h>
#include <crypto/ecdsa.h>


static int verify_signature(in3_vctx_t* vc, bytes_t* msg_hash, jsmntok_t* sig) {
    jsmntok_t* t;
    int res=0,i;

    // check messagehash
    if (!(t=res_get(vc,sig,"msgHash"))) return 0;
    bytes_t* sig_msg_hash =  res_to_bytes(vc,t);
    if (!b_cmp(sig_msg_hash,msg_hash)) {
        b_free(sig_msg_hash);
        return 0;
    }
    b_free(sig_msg_hash);


    uint8_t pubkey[65]; 
    bytes_t pubkey_bytes = { .len=64, .data=((uint8_t*) &pubkey)+1 };

    uint8_t sdata[64]; 
    
    bytes_t* r = res_to_bytes(vc, res_get(vc,sig,"r"));
    bytes_t* s = res_to_bytes(vc, res_get(vc,sig,"s"));
    int v      = res_to_long(vc, res_get(vc,sig,"v"),0);
    if ( v>= 27) v -= 27;
    if (r==NULL || s==NULL || r->len+s->len!=64) {
        b_free(r);
        b_free(s);
        vc_err(vc,"wrong signatuire length");
        return 0;
    }
    memcpy(sdata,r->data,r->len);
    memcpy(sdata+r->len, s->data,s->len);
    b_free(r);
    b_free(s);

	// verify signature
	ecdsa_recover_pub_from_sig(&secp256k1, pubkey, sdata, msg_hash->data, v);

    // now create the address
    bytes_t* hash = sha3(&pubkey_bytes);
	bytes_t* addr = b_new( (void*) hash->data+12, 20);

    for (i=0;i<vc->config->signaturesCount;i++) {
        if (b_cmp( vc->config->signatures +i, addr)) {
            res = i+1;
            break;
        }
    }

    b_free(hash);
    b_free(addr);

    return res;
}

int eth_verify_blockheader( in3_vctx_t* vc, bytes_t* header, jsmntok_t* expected_blockhash) {
    int res=0,i;
    uint64_t header_number=0;
    jsmntok_t* t,*sig,*t2;
    bytes_t* block_hash = sha3(header);
    bytes_t temp;

    if (!header) res = vc_err(vc,"no header found");

    if (!res && rlp_decode(header,0,&temp) && rlp_decode(&temp,8,&temp)) 
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
        // prepare the message to be sigfned
        bytes_t msg;
        uint8_t msg_data[64];
        msg.data= (uint8_t*) &msg_data;
        msg.len=64;
        // first the blockhash + blocknumber
        memcpy(msg_data,block_hash->data,32);
        memset(msg_data +32,0,32);
        long_to_bytes(header_number,msg_data+56);
        bytes_t* msg_hash =  sha3(&msg);


        int confirmed=0;
        for (i=0;i<t->size;i++) {
            sig = ctx_get_array_token(t,i);
            if ((t= res_get(vc,sig,"block"))) {
                uint64_t block_number = res_to_long(vc,t,0);
                //TODO make sure we are not recovering the same siognature twice, since this copuld be an attack veector!
                if (block_number==header_number && verify_signature(vc, msg_hash, sig)) 
                   confirmed++;
            }
        }

        b_free(msg_hash);
        if (confirmed< vc->config->signaturesCount) 
           res = vc_err(vc,"missing signatures");
    }

    b_free(block_hash);

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
