#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/client/request_internal.h"
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
  TRY(zksync_get_contracts(conf, ctx->req, &main_contract))

  // check if the key is already authorized by calling
  // authFacts(address account,uint32 nonce) == keccak256(pubkey_hash)
  sb_add_rawbytes(&sb, "{\"to\":\"0x", bytes(main_contract, 20), 0);
  sb_add_rawbytes(&sb, "\",\"data\":\"0x8ae20dc9", bytes(data, 64), 0);
  sb_add_chars(&sb, "\"},\"latest\"");

  // send request
  TRY_FINAL(send_provider_request(ctx->req, NULL, "eth_call", sb.data, &result), _free(sb.data))
  bytes_t call_res = d_bytes(result);

  // check result
  bytes32_t pub_hash_hash;
  keccak(bytes(pub_hash, 20), pub_hash_hash);
  if (call_res.data && call_res.len == 32 && memcmp(pub_hash_hash, call_res.data, 32) == 0) return IN3_OK;

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

  TRY_FINAL(send_provider_request(ctx->req, NULL, "eth_sendTransactionAndWait", sb.data, &result), _free(sb.data))

  // was it successfull?
  if (result == NULL || d_type(result) != T_OBJECT || d_get_int(result, K_STATUS) == 0)
    return req_set_error(ctx->req, "setAuthPubkeyHash-Transaction failed", IN3_EINVAL);

  return IN3_OK;
}

in3_ret_t zksync_set_key(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, bool only_update) {
  address_t          pub_hash;
  d_token_internal_t tmp;
  zksync_valid_t     valid;
  uint32_t           nonce;
  int                plen    = only_update ? 0 : d_len(ctx->params);
  d_token_t*         token   = plen == 1 ? d_get_at(ctx->params, 0) : NULL;
  bytes_t            new_key = d_get_bytes_at(ctx->params, 1);
  valid.from                 = plen > 2 ? d_get_long_at(ctx->params, 2) : 0;
  valid.to                   = plen > 3 ? d_get_long_at(ctx->params, 3) : 0;
  if (!valid.to) valid.to = 0xffffffffl;
  if (!token) {
    token = &tmp;
    tmp   = (d_token_internal_t){.data = (void*) "ETH", .len = T_STRING << 28 | 3, .key = 0, .state = 0};
  }

  zksync_token_t* token_data = NULL;
  if (!token) return req_set_error(ctx->req, "Missing fee token as first token", IN3_EINVAL);
  zk_fee_t fee;
  if (new_key.data && new_key.len == 32) memcpy(conf->sync_key, new_key.data, 32);
  TRY(zksync_get_nonce(conf, ctx->req, NULL, &nonce))
  TRY(resolve_tokens(conf, ctx->req, token, &token_data))
  TRY(zksync_get_pubkey_hash(conf, ctx->req, pub_hash))

  if (memcmp(pub_hash, conf->pub_key_hash_set, 20) == 0) return req_set_error(ctx->req, "Signer key is already set", IN3_EINVAL); // and check if it is already set
  if (!conf->account_id) return req_set_error(ctx->req, "No Account set yet", IN3_EINVAL);

  // for contracts we need to pre authorized on layer 1
  if (conf->sign_type == ZK_SIGN_CONTRACT) TRY(auth_pub_key(conf, ctx, nonce, pub_hash))

  // get fees
  // 'Onchain' | 'ECDSA' | 'CREATE2' | 'ECDSALegacyMessage';
  char* keytype = "{\"ChangePubKey\":\"ECDSA\"}";
  if (conf->sign_type == ZK_SIGN_CREATE2)
    keytype = "{\"ChangePubKey\":\"CREATE2\"}";
  else if (conf->sign_type == ZK_SIGN_CONTRACT)
    keytype = "{\"ChangePubKey\":\"Onchain\"}";
  TRY(zksync_get_fee(conf, ctx->req, NULL, bytes(conf->account, 20), token, keytype,
#ifdef ZKSYNC_256
                     fee
#else
                     &fee
#endif
                     ))

  // create payload for change key tx
  cache_props_t  ckey   = CACHE_PROP_MUST_FREE | 0xC100;
  cache_entry_t* cached = ctx->req->cache;
  while (cached) {
    if (cached->props == ckey) break;
    cached = cached->next;
  }
  if (!cached) {
    sb_t      sb  = {0};
    in3_ret_t ret = zksync_sign_change_pub_key(&sb, ctx->req, pub_hash, nonce, conf, fee, token_data, valid);
    if (ret && sb.data) _free(sb.data);
    TRY(ret)
    if (!sb.data) return IN3_EUNKNOWN;
    cached        = in3_cache_add_entry(&ctx->req->cache, NULL_BYTES, bytes((void*) sb.data, strlen(sb.data)));
    cached->props = ckey;
  }

  d_token_t* result = NULL;
  in3_ret_t  ret    = send_provider_request(ctx->req, conf, "tx_submit", (void*) cached->value.data, &result);
  if (ret == IN3_OK) {
    if (only_update) {
      if (d_type(result) == T_STRING) {
        conf->nonce++;
        memcpy(conf->pub_key_hash_set, pub_hash, 20);
        return IN3_OK;
      }
      return req_set_error(ctx->req, "Invalid response qwhen setting key", IN3_ERPC);
    }
    // return only the pubkeyhash as result
    sb_t* sb = in3_rpc_handle_start(ctx);
    sb_add_rawbytes(sb, "\"sync:", bytes(pub_hash, 20), 20);
    sb_add_char(sb, '\"');
    return in3_rpc_handle_finish(ctx);
  }
  return ret;
}
