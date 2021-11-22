#ifndef _BTC_SIGN_H
#define _BTC_SIGN_H

#include "../../core/client/request.h"
#include "btc_types.h"

#define BTC_SIGHASH_ALL          0x1
#define BTC_SIGHASH_NONE         0x2
#define BTC_SIGHASH_SINGLE       0x3
#define BTC_SIGHASH_ANYONECANPAY 0x80

in3_ret_t btc_sign_tx_in(in3_req_t* req, btc_tx_t* tx, const btc_utxo_t* utxo_list, const uint32_t utxo_list_len, const uint32_t utxo_index, const bool is_segwit, const bool is_multisig, const bytes_t* account, const bytes_t* pub_key, btc_tx_in_t* tx_in, uint8_t sighash);
in3_ret_t btc_sign_tx(in3_req_t* ctx, btc_tx_t* tx, const btc_utxo_t* selected_utxo_list, uint32_t utxo_list_len, bytes_t* account, bytes_t* pub_key);

#endif