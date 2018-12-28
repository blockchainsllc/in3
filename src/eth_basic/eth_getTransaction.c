
#include <util/mem.h>
#include "../eth_nano/eth_nano.h"
#include "eth_basic.h"
#include <util/utils.h>
#include <client/context.h>
#include <string.h>
#include "../eth_nano/rlp.h"
#include "../eth_nano/merkle.h"
#include "../eth_nano/serialize.h"
#include <util/data.h>
#include <client/keys.h>




int eth_verify_eth_getTransaction(in3_vctx_t *vc, bytes_t *tx_hash) {

    int res = 0;

    if (!tx_hash)
        return vc_err(vc, "No Transaction Hash found");
    if (tx_hash->len!=32)
        return vc_err(vc, "The transactionHash has the wrong length!");

    // this means result: null, which is ok, since we can not verify a transaction that does not exists
    if (!vc->proof)
        return vc_err(vc, "Proof is missing!");

    bytes_t *blockHeader = d_get_bytesk(vc->proof,K_BLOCK);
    if (!blockHeader)
        return vc_err(vc, "No Block-Proof!");
    res = eth_verify_blockheader(vc, blockHeader, d_get_bytesk(vc->result, K_BLOCK_HASH ) );
    if (res == 0)
    {
        bytes_t* path = create_tx_path(  d_get_intk(vc->proof, K_TX_INDEX));
        bytes_t root, raw_transaction = { .len=0, .data=NULL };
        bytes_t **proof      = d_create_bytes_vec( d_get(vc->proof,K_MERKLE_PROOF));
        if (rlp_decode_in_list(blockHeader,4,&root)!=1) 
            res=vc_err(vc,"no tx root");
        else {
            if (!proof || !verifyMerkleProof(&root,path,proof,&raw_transaction) || raw_transaction.data==NULL)
                res=vc_err(vc,"Could not verify the tx proof");
            else {
                bytes_t* proofed_hash = sha3(&raw_transaction);
                if (!b_cmp(proofed_hash, tx_hash))
                    res = vc_err(vc,"The TransactionHash is not the same as expected");
                b_free(proofed_hash);
            }
        }
        if (proof) _free(proof);
        b_free(path);

        bytes_t* tx_data = serialize_tx(vc->result);
        if (res==0 && !b_cmp(tx_data,&raw_transaction)) 
           res=vc_err(vc,"Could not verify the transaction data");
        b_free(tx_data);
    }
    return res;

}
