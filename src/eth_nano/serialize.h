/** @file 
 * serialization of ETH-Objects.
 * 
 * This incoming tokens will represent their values as properties based on [JSON-RPC](https://github.com/ethereum/wiki/wiki/JSON-RPC).
 * 
 * */

#include "../core/util/data.h"

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

bytes_t* serialize_tx(d_token_t* tx);

/**
 * creates rlp-encoded raw bytes for a account.
 * 
 * The bytes must be freed with b_free after use!
 * 
 * 
 */

bytes_t* serialize_account(d_token_t* a);

/**
 * creates rlp-encoded raw bytes for a blockheader.
 * 
 * The bytes must be freed with b_free after use!
 * 
 * \param receipt the json-onject as descibed in [eth_getBlockByHash](https://github.com/ethereum/wiki/wiki/JSON-RPC#eth_getblockbyhash)
 */
bytes_t* serialize_block_header(d_token_t* block);

/**
 * adds the value represented by the token rlp-encoded to the byte_builder.
 * \param ml the minimum number of bytes. if the length is 0, a one byte-string with 0x00 will be written as 0x.
 * \param rlp the builder to add to.
 * \param t the token representing the value. must be a integer, bytes or NULL.
 * 
 * \return 0 if added -1 if the value could not be handled.
 */
int rlp_add(bytes_builder_t* rlp, d_token_t* t, int ml);

// clang-format off

#define BLOCKHEADER_PARENT_HASH       0
#define BLOCKHEADER_SHA3_UNCLES       1
#define BLOCKHEADER_MINER             2
#define BLOCKHEADER_STATE_ROOT        3
#define BLOCKHEADER_TRANSACTIONS_ROOT 4
#define BLOCKHEADER_RECEIPT_ROOT      5
#define BLOCKHEADER_LOGS_BLOOM        6
#define BLOCKHEADER_DIFFICULTY        7
#define BLOCKHEADER_NUMBER            8
#define BLOCKHEADER_GAS_LIMIT         9
#define BLOCKHEADER_GAS_USED         10
#define BLOCKHEADER_TIMESTAMP        11
#define BLOCKHEADER_EXTRA_DATA       12
#define BLOCKHEADER_SEALED_FIELD1    13
#define BLOCKHEADER_SEALED_FIELD2    14
#define BLOCKHEADER_SEALED_FIELD3    15

