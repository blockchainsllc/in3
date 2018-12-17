/** @file 
 * serialization of ETH-Objects.
 * */ 

#include <util/bytes.h>
#include <client/context.h>
#include <string.h>
#include <client/verifier.h>

bytes_t* serialize_tx_receipt(in3_vctx_t* vc, jsmntok_t* receipt);
bytes_t *serialize_tx(in3_vctx_t *vc, jsmntok_t *tx);
