#ifndef _BTC_TYPES_H
#define _BTC_TYPES_H

#include "../../core/client/request.h"
#include "../../core/util/bytes.h"
#include "../../core/util/data.h"
#include "../../core/util/error.h"
#include "btc_script.h"
#include <stdint.h>

// General size values
#define BTC_UNCOMP_PUB_KEY_SIZE_BYTES 65
#define BTC_COMP_PUB_KEY_SIZE_BYTES   33

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
} btc_utxo_t;

/* Bitcoin transaction context */
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

btc_stype_t btc_get_script_type(const bytes_t* script);
bool        script_is_standard(btc_stype_t script_type);
bool        pub_key_is_valid(const bytes_t* pub_key);

in3_ret_t btc_parse_tx(bytes_t tx, btc_tx_t* dst);
uint32_t  btc_get_raw_tx_size(const btc_tx_t* tx);
in3_ret_t btc_serialize_tx(in3_req_t* req, const btc_tx_t* tx, bytes_t* dst);
in3_ret_t btc_tx_id(btc_tx_t* tx, bytes32_t dst);

uint8_t*  btc_parse_tx_in(uint8_t* data, btc_tx_in_t* dst, uint8_t* limit);
in3_ret_t btc_serialize_tx_in(in3_req_t* req, btc_tx_in_t* tx_in, bytes_t* dst);

uint8_t*  btc_parse_tx_out(uint8_t* data, btc_tx_out_t* dst);
in3_ret_t btc_serialize_tx_out(in3_req_t* req, btc_tx_out_t* tx_out, bytes_t* dst);

uint32_t btc_vsize(btc_tx_t* tx);
uint32_t btc_weight(btc_tx_t* tx);

in3_ret_t btc_add_input_to_tx(in3_req_t* req, btc_tx_ctx_t* tx_ctx, btc_tx_in_t* tx_in);
in3_ret_t btc_add_output_to_tx(in3_req_t* req, btc_tx_ctx_t* tx_ctx, btc_tx_out_t* tx_out);
in3_ret_t btc_add_witness_to_tx(in3_req_t* req, btc_tx_ctx_t* tx_ctx, bytes_t* witness);

in3_ret_t btc_prepare_utxos(in3_req_t* req, btc_tx_ctx_t* tx_ctx, btc_account_pub_key_t* default_acc_pk, d_token_t* utxo_inputs, d_token_t* args);
in3_ret_t btc_set_segwit(btc_tx_ctx_t* tx_ctx);

in3_ret_t btc_verify_public_key(in3_req_t* req, const bytes_t* public_key);

static inline bool btc_is_witness(bytes_t tx) {
  return tx.data[4] == 0 && tx.data[5] == 1;
}

in3_ret_t add_outputs_to_tx(in3_req_t* req, d_token_t* outputs, btc_tx_ctx_t* tx_ctx);

#endif