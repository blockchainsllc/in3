// @PUBLIC_HEADER
/** @file
 * Bitcoin verification.
 * */

#ifndef in3_btc_h__
#define in3_btc_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "../../core/client/plugin.h"
#include "btc_target.h"

/**
 * this function should only be called once and will register the bitcoin verifier.
 */
in3_ret_t in3_register_btc(in3_t* c);

/**
 * prepares a transaction and writes the data to the dst-bytes. In case of success, you MUST free only the data-pointer of the dst.
 */
in3_ret_t btc_prepare_unsigned_tx(in3_req_t* req, /**< the current context */
                                  bytes_t*   dst, /**< the bytes to write the result to. */
                                  d_token_t* outputs, d_token_t* utxos, bytes_t* account, bytes_t* pub_key, bool is_testnet,
                                  sb_t* meta /**< a stringbuilder in order write the wallet_state and metadata depending on the tx. */
);

in3_ret_t btc_sign_raw_tx(in3_req_t* req, bytes_t* raw_tx, address_t signer_id, bytes_t* signer_pub_key, bytes_t* dst);

btc_target_conf_t* btc_get_config(in3_t* c);

#ifdef __cplusplus
}
#endif

#endif // in3_btc_h__