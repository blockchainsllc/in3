

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

in3_ret_t zksync_transfer(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, d_token_t* params, zk_msg_type_t type) {
  bytes32_t sync_key;
  TRY(zksync_get_sync_key(conf, ctx->ctx, sync_key));
  // prepare tx data
  zksync_tx_data_t tx_data = {0};
  bytes_t          to      = d_to_bytes(params_get(params, type == ZK_WITHDRAW ? key("ethAddress") : K_TO, 0));
  if (!to.data || to.len != 20) return ctx_set_error(ctx->ctx, "invalid to address", IN3_EINVAL);
  memcpy(tx_data.to, to.data, 20);
  tx_data.type = type;

#ifdef ZKSYNC_256
  bytes_t amount = d_to_bytes(params_get(params, key("amount"), 1));
  if (amount.len > 33) return ctx_set_error(ctx->ctx, "invalid to amount", IN3_EINVAL);
  memcpy(tx_data.amount + 32 - amount.len, amount.data, amount.len);
#else
  tx_data.amount = d_long(params_get(params, key("amount"), 1));
#endif

  // prepare tx_data
  TRY(zksync_get_account_id(conf, ctx->ctx, &tx_data.account_id))
  TRY(resolve_tokens(conf, ctx->ctx, params_get(params, key("token"), 2), &tx_data.token))
  TRY(zksync_get_nonce(conf, ctx->ctx, params_get(params, K_NONCE, 4), &tx_data.nonce))
  TRY(zksync_get_fee(conf, ctx->ctx, params_get(params, key("fee"), 3), to, params_get(params, key("token"), 2), type == ZK_WITHDRAW ? "Withdraw" : "Transfer",
#ifdef ZKSYNC_256
                     tx_data.fee
#else
                     &tx_data.fee
#endif
                     ))
  memcpy(tx_data.from, conf->account, 20);

  // create payload
  cache_entry_t* cached = ctx->ctx->cache;
  while (cached) {
    if (cached->props & 0x10) break;
    cached = cached->next;
  }
  if (!cached) {
    sb_t      sb  = {0};
    in3_ret_t ret = zksync_sign_transfer(&sb, &tx_data, ctx->ctx, sync_key);
    if (ret && sb.data) _free(sb.data);
    if (!sb.data) return IN3_EUNKNOWN;
    TRY(ret)
    cached        = in3_cache_add_entry(&ctx->ctx->cache, bytes(NULL, 0), bytes((void*) sb.data, strlen(sb.data)));
    cached->props = CACHE_PROP_MUST_FREE | 0x10;
  }

  d_token_t* result = NULL;
  in3_ret_t  ret    = send_provider_request(ctx->ctx, conf, "tx_submit", (void*) cached->value.data, &result);
  if (ret == IN3_OK && cached && cached->value.data) {
    sb_t* sb = in3_rpc_handle_start(ctx);
    sb_add_range(sb, (void*) cached->value.data, 0, strlen((void*) cached->value.data) - 177);
    sb_add_chars(sb, ",\"txHash\":\"");
    sb_add_chars(sb, d_string(result));
    sb_add_chars(sb, "\"}");
    return in3_rpc_handle_finish(ctx);
  }
  return ret;
}