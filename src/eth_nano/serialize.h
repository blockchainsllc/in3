/** @file 
 * serialization of ETH-Objects.
 * 
 * This incoming tokens will represent their values as properties based on [JSON-RPC](https://github.com/ethereum/wiki/wiki/JSON-RPC).
 * 
 * */ 

#include <util/data.h>

/**
 * creates rlp-encoded raw bytes for a receipt.
 * 
 * The bytes must be freed with b_free after use!
 * 
 * \param receipt the json-onject as descibed in [eth_getTransactionReceipt](https://github.com/ethereum/wiki/wiki/JSON-RPC#eth_gettransactionreceipt)
 */
bytes_t* serialize_tx_receipt(d_token_t* receipt);
/**
 * creates rlp-encoded raw bytes for a transaction.
 * 
 * The bytes must be freed with b_free after use!
 * 
 * \param receipt the json-onject as descibed in [eth_getTransactionByHash](https://github.com/ethereum/wiki/wiki/JSON-RPC#eth_gettransactionbyhash)
 * 
 */
bytes_t *serialize_tx(d_token_t *tx);
/**
 * creates rlp-encoded raw bytes for a blockheader.
 * 
 * The bytes must be freed with b_free after use!
 * 
 * \param receipt the json-onject as descibed in [eth_getBlockByHash](https://github.com/ethereum/wiki/wiki/JSON-RPC#eth_getblockbyhash)
 */
bytes_t *serialize_block_header(d_token_t *block);

/**
 * adds the value represented by the token rlp-encoded to the byte_builder.
 * \param ml the minimum number of bytes. if the length is 0, a one byte-string with 0x00 will be written as 0x.
 * \param rlp the builder to add to.
 * \param t the token representing the value. must be a integer, bytes or NULL.
 * 
 * \return 0 if added -1 if the value could not be handled.
 */
int rlp_add(bytes_builder_t *rlp, d_token_t* t, int ml);
