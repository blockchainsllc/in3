#ifndef _BTC_TYPES_H
#define _BTC_TYPES_H

#include "../../core/client/request.h"
#include "../../core/util/bytes.h"
#include "../../core/util/data.h"
#include "../../core/util/error.h"
#include "btc_address.h"
#include "btc_script.h"
#include <stdint.h>

// General size values
#define BTC_UNCOMP_PUB_KEY_SIZE_BYTES 65
#define BTC_COMP_PUB_KEY_SIZE_BYTES   33
#define BTC_SHA256_SIZE_BYTES         32
#define BTC_HASH160_SIZE_BYTES        20

// Transaction fixed size values
#define BTC_TX_VERSION_SIZE_BYTES  4
#define BTC_TX_LOCKTIME_SIZE_BYTES 4
#define BTC_TX_HASH_SIZE_BYTES     32
#define BTX_TX_INDEX_SIZE_BYTES    4

// Input fixed size values
#define BTC_TX_IN_PREV_OUPUT_SIZE_BYTES (BTC_TX_HASH_SIZE_BYTES + BTX_TX_INDEX_SIZE_BYTES)
#define BTC_TX_IN_SEQUENCE_SIZE_BYTES   4
#define BTC_TX_IN_SIGHASH_SIZE_BYTES    4

// Output fixed size values
#define BTC_TX_OUT_VALUE_SIZE_BYTES 8

// Default transaction values
#define DEFAULT_TXIN_SEQUENCE_NUMBER 0xffffffff

// BIP 68 flags
#define SEQUENCE_LOCKTIME_TYPE_BLOCK 0
#define SEQUENCE_LOCKTIME_TYPE_TIME  1

// -- when this bit is set, sequence number is NOT interpreted as relative locktime
#define SEQUENCE_LOCKTIME_DISABLE_FLAG (1 << 31)

// -- When relative locktime is activated and this bit is:
// -- SET - relative locktime specifies units of 512 seconts
// -- NOT SET - relative locktime specifies blocks with granularity 1
#define SEQUENCE_LOCKTIME_TYPE_FLAG (1 << 22)

// -- mask to extract locktim value from nsequence field
#define SEQUENCE_LOCKTIME_MASK 0x0000ffff

// Ethereum account which stores our private key. Used by signer module
typedef struct btc_account_pub_key {
  bytes_t pub_key;
  bytes_t account;
} btc_account_pub_key_t;

typedef struct btc_tx {
  bytes_t  all;     // This transaction, serialized
  uint32_t version; // btc protocol version
  uint16_t flag;    // 1 if segwit, 0 otherwise
  uint32_t input_count;
  bytes_t  input; // serialized inputs data
  uint32_t output_count;
  bytes_t  output;    // serialized outputs data
  bytes_t  witnesses; // serialized witnesses data
  uint32_t lock_time;
} btc_tx_t;

typedef struct btc_tx_in {
  uint8_t*     prev_tx_hash;
  uint32_t     prev_tx_index;
  btc_script_t script;
  uint32_t     sequence;
} btc_tx_in_t;

typedef struct btc_tx_out {
  uint64_t     value;
  btc_script_t script;
} btc_tx_out_t;

typedef struct btc_utxo {
  uint8_t*               tx_hash;        // Hash of previous transaction
  uint32_t               tx_index;       // Putput index inside previous transaction
  btc_tx_out_t           tx_out;         // Previous output which the utxo represents
  btc_script_t           raw_script;     // Unhashed script used to redeem P2SH or P2WSH utxos
  uint32_t               req_sigs;       // Number of signatures we need to provide in order to unlock the utxo
  bytes_t*               signatures;     // Array of signatures used to redeem the utxo
  uint32_t               sig_count;      // Number of signatures we currently have in our array
  btc_account_pub_key_t* accounts;       // Array of ETH accounts used by in3 to sign BTC transactions
  uint32_t               accounts_count; // Number of accounts we currently have in our array
  uint32_t               sequence;       // Desired sequence number when utxo is converted to a transaction input
} btc_utxo_t;

// Bitcoin transaction context
typedef struct btc_tx_ctx {
  btc_tx_t      tx;
  btc_utxo_t*   utxos;
  uint32_t      utxo_count;
  btc_tx_in_t*  inputs;
  uint32_t      input_count;
  btc_tx_out_t* outputs;
  uint32_t      output_count;
} btc_tx_ctx_t;

void btc_init_tx(btc_tx_t* tx);
void btc_init_tx_ctx(btc_tx_ctx_t* tx_ctx);
void btc_init_tx_in(btc_tx_in_t* tx_in);
void btc_init_tx_out(btc_tx_out_t* tx_out);

void btc_free_tx(btc_tx_t* tx);
void btc_free_tx_in(btc_tx_in_t* tx_in);
void btc_free_tx_out(btc_tx_out_t* tx_out);
void btc_free_utxo(btc_utxo_t* utxo);
void btc_free_tx_ctx(btc_tx_ctx_t* tx_ctx);

bytes_t btc_build_locking_script(bytes_t* receiving_btc_addr, btc_stype_t type, const bytes_t* args, uint32_t args_len);
bool    pub_key_is_valid(const bytes_t* pub_key);

in3_ret_t btc_parse_tx(bytes_t tx, btc_tx_t* dst);
in3_ret_t btc_parse_tx_ctx(bytes_t raw_tx, btc_tx_ctx_t* dst);
uint32_t  btc_get_raw_tx_size(const btc_tx_t* tx);
in3_ret_t btc_serialize_tx(in3_req_t* req, const btc_tx_t* tx, bytes_t* dst);
in3_ret_t btc_tx_id(btc_tx_t* tx, bytes32_t dst);

uint8_t*  btc_parse_tx_in(uint8_t* data, btc_tx_in_t* dst, uint8_t* limit);
in3_ret_t btc_serialize_tx_in(in3_req_t* req, btc_tx_in_t* tx_in, bytes_t* dst);

uint8_t*  btc_parse_tx_out(uint8_t* data, btc_tx_out_t* dst);
in3_ret_t btc_serialize_tx_out(in3_req_t* req, btc_tx_out_t* tx_out, bytes_t* dst);

uint32_t btc_vsize(btc_tx_t* tx);
uint32_t btc_weight(btc_tx_t* tx);

/*
 * Parses output script to extract target btc address
 * parsed address can be founs on 'dst' after function execution
 * returns the type of scriptPubKey the adress was extracted from
 */
btc_stype_t extract_address_from_output(btc_tx_out_t* tx_out, btc_address_t* dst);

/*
 * Parses a p2ms script to extract a list of defined public keys
 * Public keys are stored on pub_key_list array
 * returns the length of pub_key_list
 */
uint32_t extract_public_keys_from_multisig(bytes_t multisig_script, bytes_t** pub_key_list);

in3_ret_t btc_add_input_to_tx(in3_req_t* req, btc_tx_ctx_t* tx_ctx, btc_tx_in_t* tx_in);
in3_ret_t btc_add_output_to_tx(in3_req_t* req, btc_tx_ctx_t* tx_ctx, btc_tx_out_t* tx_out);
in3_ret_t btc_add_witness_to_tx(in3_req_t* req, btc_tx_ctx_t* tx_ctx, bytes_t* witness);

uint32_t btc_build_nsequence_relative_locktime(uint8_t locktime_type_flag, uint16_t value);
uint16_t btc_nsequence_get_relative_locktime_value(uint32_t nsequence);
bool     btc_nsequence_is_relative_locktime(uint32_t nsequence);

in3_ret_t btc_prepare_outputs(in3_req_t* req, btc_tx_ctx_t* tx_ctx, d_token_t* output_data);
in3_ret_t btc_prepare_utxos(in3_req_t* req, btc_tx_ctx_t* tx_ctx, btc_account_pub_key_t* default_account, d_token_t* utxo_inputs);
in3_ret_t btc_prepare_inputs(in3_req_t* req, btc_tx_ctx_t* tx_ctx);
in3_ret_t btc_set_segwit(btc_tx_ctx_t* tx_ctx);

bool btc_public_key_is_valid(const bytes_t* public_key);

static inline bool btc_is_witness(bytes_t tx) {
  return tx.data[4] == 0 && tx.data[5] == 1;
}

in3_ret_t add_outputs_to_tx(in3_req_t* req, d_token_t* outputs, btc_tx_ctx_t* tx_ctx);

#endif