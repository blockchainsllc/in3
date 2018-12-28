
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
#include "trie.h"



int eth_verify_eth_getBlock(in3_vctx_t *vc, bytes_t *block_hash, uint64_t blockNumber) {


    int res = 0,i;
    d_token_t* transactions, *t;
    if (block_hash && !b_cmp(block_hash,d_get_bytesk(vc->result,K_HASH ))) 
        return vc_err(vc, "The transactionHash does not match the required");

    if (blockNumber && blockNumber!= d_get_longk(vc->result,K_NUMBER )) 
        return vc_err(vc, "The blockNumber  does not match the required");

    // this means result: null, which is ok, since we can not verify a transaction that does not exists
    if (!vc->proof)
        return vc_err(vc, "Proof is missing!");

    // verify the blockdata
    bytes_t* header_from_data = serialize_block(vc->result);
    if (eth_verify_blockheader(vc, header_from_data, d_get_bytesk(vc->result,K_HASH ) )) {
        b_free(header_from_data);
        return vc_err(vc, "invalid blockheader!");
    }
    b_free(header_from_data);

    // if we have transaction, we need to verify them as well
    if ((transactions = d_get(vc->result, K_TRANSACTIONS))) {

        trie_t* trie = trie_new();
        for (i=0, t=transactions+1; i<d_len(transactions);i++,t=d_next(t)) {
            d_token_t* tx2=t;
            bytes_t* key = create_tx_path(i);
            bytes_t* tx  = serialize_tx(t);
            trie_set_value(trie, key, tx);
            b_free(key);
            b_free(tx);
        }

        if (!b_cmp(&trie->root, d_get_bytesk(vc->result,K_TRANSACTIONS_ROOT)) ) {
            printf("\nExpoected  ");
            b_print(d_get_bytesk(vc->result,K_TRANSACTIONS_ROOT));
            printf("\ncalculated ");
            b_print(&trie->root);



          res= vc_err(vc,"Wrong Transaction root");
        }
        trie_free(trie);
    }
    



    
    return res;

}
