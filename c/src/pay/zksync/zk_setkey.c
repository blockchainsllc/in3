#include "../../core/client/context_internal.h"
#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/util/debug.h"
#include "../../core/util/mem.h"
#include "../../third-party/zkcrypto/lib.h"
#include "zk_helper.h"
#include "zksync.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static in3_ret_t auth_pub_key(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, uint32_t nonce, address_t pub_hash) {
  d_token_t* result        = NULL;
  sb_t       sb            = {0};
  uint8_t*   main_contract = NULL;
  uint8_t    data[128];                 // the abi-ebcoded data
  memset(data, 0, 128);                 // clear the data
  memcpy(data + 12, conf->account, 20); // account to check
  int_to_bytes(nonce, data + 60);       // nonce
  TRY(zksync_get_contracts(conf, ctx->ctx, &main_contract))

  // check if the key is already authorized by calling
  // authFacts(address account,uint32 nonce) == keccak256(pubkey_hash)
  sb_add_rawbytes(&sb, "{\"to\":\"0x", bytes(main_contract, 20), 0);
  sb_add_rawbytes(&sb, "\",\"data\":\"0x8ae20dc9", bytes(data, 64), 0);
  sb_add_chars(&sb, "\"},\"latest\"");

  // send request
  TRY_FINAL(send_provider_request(ctx->ctx, NULL, "eth_call", sb.data, &result), _free(sb.data))

  // check result
  bytes32_t pub_hash_hash;
  keccak(bytes(pub_hash, 20), pub_hash_hash);
  if (d_type(result) == T_BYTES && d_len(result) == 32 && memcmp(pub_hash_hash, result->data, 32) == 0) return IN3_OK;

  // not approved yet, so we need to approve in layer 1
  // the abi-ebcoded data for calling setAuthPubkeyHash(bytes calldata _pubkey_hash, uint32 _nonce)
  memset(&sb, 0, sizeof(sb_t));    // clear stringbuilder
  memset(data, 0, 128);            // clear the data
  data[31] = 64;                   // offset for bytes
  data[95] = 20;                   // length of the pubKeyHash
  memcpy(data + 96, pub_hash, 20); // copy new pubKeyHash
  int_to_bytes(nonce, data + 60);  // nonce
  sb_add_rawbytes(&sb, "{\"to\":\"0x", bytes(main_contract, 20), 0);
  sb_add_rawbytes(&sb, "\",\"data\":\"0x595a5ebc", bytes(data, 128), 0);
  sb_add_chars(&sb, "\",\"gas\":\"0x30d40\"}");

  TRY_FINAL(send_provider_request(ctx->ctx, NULL, "eth_sendTransactionAndWait", sb.data, &result), _free(sb.data))

  // was it successfull?
  if (result == NULL || d_type(result) != T_OBJECT || d_get_intk(result, K_STATUS) == 0)
    return ctx_set_error(ctx->ctx, "setAuthPubkeyHash-Transaction failed", IN3_EINVAL);

  return IN3_OK;
}

in3_ret_t zksync_set_key(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, d_token_t* params) {
  bytes32_t       pk;
  address_t       pub_hash;
  uint32_t        nonce;
  d_token_t*      token      = d_len(params) == 1 ? params + 1 : NULL;
  zksync_token_t* token_data = NULL;
  if (!token) return ctx_set_error(ctx->ctx, "Missing fee token as first token", IN3_EINVAL);
#ifdef ZKSYNC_256
  bytes32_t fee;
#else
  uint64_t fee;
#endif
  TRY(zksync_get_nonce(conf, ctx->ctx, NULL, &nonce))
  TRY(resolve_tokens(conf, ctx->ctx, token, &token_data))
  TRY(zksync_get_sync_key(conf, ctx->ctx, pk))

  zkcrypto_pk_to_pubkey(pk, pub_hash);                                                                                        // calculate the pubKey_hash
  if (memcmp(pub_hash, conf->pub_key_hash, 20) == 0) return ctx_set_error(ctx->ctx, "Signer key is already set", IN3_EINVAL); // and check if it is already set
  if (!conf->account_id) return ctx_set_error(ctx->ctx, "No Account set yet", IN3_EINVAL);

  // for contracts we need to pre authorized on layer 1
  if (conf->sign_type == ZK_SIGN_CONTRACT) TRY(auth_pub_key(conf, ctx, nonce, pub_hash))

  // get fees
  TRY(zksync_get_fee(conf, ctx->ctx, NULL, bytes(conf->account, 20), token, conf->sign_type == ZK_SIGN_CONTRACT ? "{\"ChangePubKey\":{\"onchainPubkeyAuth\":true}}" : "{\"ChangePubKey\":{\"onchainPubkeyAuth\":false}}",
#ifdef ZKSYNC_256
                     fee
#else
                     &fee
#endif
                     ))

  // create payload for change key tx
  cache_entry_t* cached = ctx->ctx->cache;
  while (cached) {
    if (cached->props & 0x10) break;
    cached = cached->next;
  }
  if (!cached) {
    sb_t      sb  = {0};
    in3_ret_t ret = zksync_sign_change_pub_key(&sb, ctx->ctx, pub_hash, pk, nonce, conf, fee, token_data);
    if (ret && sb.data) _free(sb.data);
    if (!sb.data) return IN3_EUNKNOWN;
    TRY(ret)
    cached        = in3_cache_add_entry(&ctx->ctx->cache, bytes(NULL, 0), bytes((void*) sb.data, strlen(sb.data)));
    cached->props = CACHE_PROP_MUST_FREE | 0x10;
  }

  d_token_t* result = NULL;
  in3_ret_t  ret    = send_provider_request(ctx->ctx, conf, "tx_submit", (void*) cached->value.data, &result);
  if (ret == IN3_OK) {
    // return only the pubkeyhash as result
    sb_t* sb = in3_rpc_handle_start(ctx);
    sb_add_rawbytes(sb, "\"sync:", bytes(pub_hash, 20), 20);
    sb_add_char(sb, '\"');
    return in3_rpc_handle_finish(ctx);
  }
  return ret;
}
