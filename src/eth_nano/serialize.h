/** @file 
 * serialization of ETH-Objects.
 * */ 

#include <util/bytes.h>
#include <client/context.h>
#include <string.h>
#include <client/verifier.h>
#include <util/data.h>

bytes_t* serialize_tx_receipt(d_token_t* receipt);
bytes_t *serialize_tx(d_token_t *tx);
bytes_t *serialize_block_header(d_token_t *block);

int rlp_add(bytes_builder_t *rlp, d_token_t* t, int ml);
