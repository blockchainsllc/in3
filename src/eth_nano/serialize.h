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
