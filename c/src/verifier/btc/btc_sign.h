#ifndef _BTC_SIGN_H
#define _BTC_SIGN_H

#include "btc_types.h"

#define BTC_SIGHASH_ALL          0x1
#define BTC_SIGHASH_NONE         0x2
#define BTC_SIGHASH_SINGLE       0x3
#define BTC_SIGHASH_ANYONECANPAY 0x80

void create_raw_tx(btc_tx_in_t* tx_in, uint32_t tx_in_len, btc_tx_out_t* tx_out, uint32_t tx_out_len, uint32_t lock_time, bytes_t* dst_raw_tx);
void btc_sign_tx_in(const btc_tx_t* tx, const btc_utxo_t* utxo_list, const uint32_t utxo_index, const bytes_t* priv_key, btc_tx_in_t* tx_in, uint8_t sighash);
void btc_sign_tx(btc_tx_t* tx, const btc_utxo_t* selected_utxo_list, uint32_t utxo_list_len, bytes_t* priv_key);

#endif