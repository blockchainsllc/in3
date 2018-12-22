#include "eth_nano.h"
#include <util/utils.h>
#include <client/context.h>
#include <string.h>
#include "rlp.h"
#include "merkle.h"
#include "serialize.h"
#include <crypto/secp256k1.h>
#include <crypto/ecdsa.h>
#include <util/mem.h>


bytes_t* create_tx_path(uint32_t index) {

   uint8_t data[4];
   int i;
   bytes_t b={ .len=4, .data=data };
   if (index==0)
     b.len=0;
   else {
       int_to_bytes(index,data);
       for (i=3;i>=0;i--) {
           if (data[i]==0) {
               b.data+=i+1;
               b.len-=i+1;
               break;
           }
       }
   }

   bytes_builder_t* bb = bb_new();
   rlp_encode_item(bb, &b);
   return bb_move_to_bytes(bb);
}

int eth_verify_eth_getTransactionReceipt(in3_vctx_t *vc, jsmntok_t *tx_hash)
{

    int res = 0;
    jsmntok_t* t;

    if (!tx_hash)
        return vc_err(vc, "No Transaction Hash found");
    if (tx_hash->end - tx_hash->start != 66)
        return vc_err(vc, "The transactionHash has the wrong length!");

    // this means result: null, which is ok, since we can not verify a transaction that does not exists
    if (vc->result->type == JSMN_PRIMITIVE)
        return 0;

    if (!vc->proof)
        return vc_err(vc, "Proof is missing!");
    if (!(t = res_get(vc, vc->proof, "block")))
        return vc_err(vc, "No Block-Proof!");

    bytes_t *blockHeader = res_to_bytes(vc, t);
    res = eth_verify_blockheader(vc, blockHeader, res_get(vc, vc->result, "blockHash"));
    if (res == 0)
    {
        bytes_t* path = create_tx_path(res_get_int(vc,vc->proof,"txIndex",0));
        bytes_t root;
        if (rlp_decode_in_list(blockHeader,5,&root)!=1) 
           res=vc_err(vc,"no receipt_root");
        else {
            bytes_t* receipt_raw = serialize_tx_receipt(vc, vc->result);
            bytes_t **proof = res_prop_to_bytes_a(vc, vc->proof, "merkleProof");

            if (!proof || !verifyMerkleProof(&root,path,proof,receipt_raw))
                res=vc_err(vc,"Could not verify the merkle proof");

            b_free(receipt_raw);
            if (proof) free_proof(proof);
        }

        if (res==0) {
            // now we need to verify the transactionIndex
            bytes_t raw_transaction = { .len=0, .data=NULL };
            bytes_t *txHash = req_to_bytes(vc, tx_hash);
            bytes_t **proof = res_prop_to_bytes_a(vc, vc->proof, "txProof");
            if (rlp_decode_in_list(blockHeader,4,&root)!=1) 
               res=vc_err(vc,"no tx root");
            else {
                 if (!proof || !verifyMerkleProof(&root,path,proof,&raw_transaction))
                    res=vc_err(vc,"Could not verify the tx proof");
                else if (raw_transaction.data==NULL)
                   res=vc_err(vc,"No value returned after verification");
                else {
                    bytes_t* proofed_hash = sha3(&raw_transaction);
                    if (!b_cmp(proofed_hash, txHash))
                      res = vc_err(vc,"The TransactionHash is not the same as expected");
                    b_free(proofed_hash);
                }
            }
            if (proof) free_proof(proof);
            b_free(txHash);
        }
        b_free(path);
    }
    b_free(blockHeader);

    if (res==0) {
        // check rest iof the values
        if (!ctx_equals_path(vc->ctx->response_data, vc->proof,1,vc->ctx->response_data, vc->result,1,EQ_MODE_CASE_NUMBER,"txIndex","transactionIndex"))
           return vc_err(vc,"wrong transactionIndex");
        if (!ctx_equals_path(vc->ctx->request_data, tx_hash,0,vc->ctx->response_data, vc->result,1,EQ_MODE_CASE_INSENSITIVE,"transactionHash"))
           return vc_err(vc,"wrong transactionHash");
    }

    return res;
}
