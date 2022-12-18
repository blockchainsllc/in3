#ifndef _BTC_SIGN_H
#define _BTC_SIGN_H

#include "../../core/client/request.h"
#include "btc_types.h"

#define BTC_SIGHASH_ALL          0x1
#define BTC_SIGHASH_NONE         0x2
#define BTC_SIGHASH_SINGLE       0x3
#define BTC_SIGHASH_ANYONECANPAY 0x80

// btc_sign_tx_in : Creates a single signature to a given bitcoin transaction input
// params:
//    - req: IN3 request context
//    - der_sig: signature which will be created given a transaction input and a key
//    - tx_ctx: transaction context. Strores all information about a transaction (inputs, outputs, utxos, witnesses, etc)
//    - utxo_index: index of the utxo inside tx_ctx to which the transaction input we need to sign is related
//    - account_index: index of the account inside the utxo which stores the private key used to sign our transaction input
//    - tx_in: the input we want to sign
//    - sighash: how do we want to sign our transaction input
// Warning: der_sig.data needs to be freed after calling this function
in3_ret_t btc_sign_tx_in(in3_req_t* req, bytes_t* der_sig, const btc_tx_ctx_t* tx_ctx, const uint32_t utxo_index, const uint32_t account_index, const btc_tx_in_t* tx_in, uint8_t sighash, sb_t* sb);

// btc_sign_tx: Signs all inputs inside our transaction context and includes them into the transaction
// params:
//      - req: IN3 request context
//      - tx_ctx: Transaction context, including all inputs, outputs and utxos we need for signing
in3_ret_t btc_sign_tx(in3_req_t* req, btc_tx_ctx_t* tx_ctx, sb_t* sb);

#endif