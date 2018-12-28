
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
    d_token_t* transactions, *t, *tx_hashs, *txh;
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

    bool include_full_tx = d_get_int_at( d_get(vc->request,K_PARAMS ),1);

    if (!include_full_tx) {
      tx_hashs =  d_get( vc->result, K_TRANSACTIONS);
      txh = tx_hashs+1;
    }
    // if we have transaction, we need to verify them as well
    if ((transactions = d_get( include_full_tx ? vc->result : vc->proof, K_TRANSACTIONS))) {

        if (!include_full_tx && (!tx_hashs || d_len(transactions)!=d_len(tx_hashs)))
          return vc_err(vc,"no transactionhashes found!" );

        trie_t* trie = trie_new();
        for (i=0, t=transactions+1; i<d_len(transactions);i++,t=d_next(t)) {
            bytes_t* key = create_tx_path(i);
            bytes_t* tx  = serialize_tx(t);
            bytes_t* h   = include_full_tx ? NULL : sha3(tx);
            if (h) {
                if (!b_cmp(d_bytes(txh),h))
                    res= vc_err(vc,"Wrong Transactionhash");
                txh = d_next(txh);
            }
            trie_set_value(trie, key, tx);
            b_free(key);
            b_free(tx);
            if (h) b_free(h);
        }

        if (!b_cmp(&trie->root, d_get_bytesk(vc->result,K_TRANSACTIONS_ROOT)) ) 
          res= vc_err(vc,"Wrong Transaction root");

        trie_free(trie);
    }
    else 
        res= vc_err(vc,"Missing transaction-properties");
    
    return res;

}
