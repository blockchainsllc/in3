
#include <util/mem.h>
#include "../eth_nano/eth_nano.h"
#include "eth_basic.h"
#include <util/utils.h>
#include <client/context.h>
#include <string.h>
#include "../eth_nano/rlp.h"
#include "../eth_nano/merkle.h"
#include "../eth_nano/serialize.h"



int eth_verify_eth_getTransaction(in3_vctx_t *vc, jsmntok_t *tx_hash) {

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
        bytes_t raw_transaction = { .len=0, .data=NULL };
        bytes_t *txHash = req_to_bytes(vc, tx_hash);
        bytes_t **proof = res_prop_to_bytes_a(vc, vc->proof, "merkleProof");
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
        b_free(path);
    }
    b_free(blockHeader);

    return res;

}
