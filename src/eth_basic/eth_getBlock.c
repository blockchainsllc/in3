
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



int eth_verify_eth_getBlock(in3_vctx_t *vc, bytes_t *block_hash, uint64_t blockNumber) {


    int res = 0;
    d_token_t* transactions;
    if (block_hash && !b_cmp(block_hash,d_get_bytesk(vc->result,K_HASH ))) 
        return vc_err(vc, "The transactionHash does not match the required");

    if (blockNumber && blockNumber!= d_get_longk(vc->result,K_NUMBER )) 
        return vc_err(vc, "The blockNumber  does not match the required");

    // this means result: null, which is ok, since we can not verify a transaction that does not exists
    if (!vc->proof)
        return vc_err(vc, "Proof is missing!");

    // verify the blockheader and hash
    bytes_t *blockHeader = d_get_bytesk(vc->proof,K_BLOCK);
    if (eth_verify_blockheader(vc, blockHeader, d_get_bytesk(vc->result,K_HASH ) )) 
        return vc_err(vc, "invalid blockheader!");

    // verify the blockdata
    bytes_t* header_from_data = serialize_block(vc->result);
    if (!b_cmp(blockHeader, header_from_data)) {
        b_free(header_from_data);
        return vc_err(vc,"The blockdata do not match the header");
    }
    b_free(header_from_data);

    // if we have transaction, we need to verify them as well
    if ((transactions = d_get(vc->result, K_TRANSACTIONS))) {
//TODO




    }
    



    
    return res;

}
