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

in3_ret_t zksync_deposit(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, d_token_t* params) {
  // check params
  if (!(d_len(params) == 1 && d_type(params + 1) == T_OBJECT)) {
    CHECK_PARAMS_LEN(ctx->ctx, params, 2)
    CHECK_PARAM_NUMBER(ctx->ctx, params, 0)
    CHECK_PARAM_TOKEN(ctx->ctx, params, 1)
  }

  //  amount
  d_token_t* tmp           = NULL;
  bytes_t    amount        = d_to_bytes(params_get(params, key("amount"), 0));
  d_token_t* token         = params_get(params, key("token"), 1);
  bool       approve       = d_int(params_get(params, key("approveDepositAmountForERC20"), 2));
  uint8_t*   main_contract = conf->main_contract;

  // make sure we have an account
  uint8_t* account = conf->account;
  if ((tmp = params_get(params, key("depositTo"), 3))) {
    if (tmp->len != 20) return ctx_set_error(ctx->ctx, "invalid depositTo", IN3_ERPC);
    account = tmp->data;
  }
  else if (!account)
    TRY(zksync_get_account(conf, ctx->ctx, &account))

  // check main_contract
  if (!main_contract) TRY(zksync_get_contracts(conf, ctx->ctx, &main_contract))

  d_token_t*      tx_receipt = NULL;
  zksync_token_t* token_conf = NULL;
  TRY(resolve_tokens(conf, ctx->ctx, token, &token_conf))
  if (!token_conf) return IN3_EUNKNOWN;

  if (memiszero(token_conf->address, 20)) { // is eth
    sb_t sb = {0};
    sb_add_rawbytes(&sb, "{\"to\":\"0x", bytes(main_contract, 20), 0);
    sb_add_rawbytes(&sb, "\",\"data\":\"0x2d2da806", bytes(account, 20), 32);
    sb_add_rawbytes(&sb, "\",\"value\":\"0x", amount, 0);
    sb_add_chars(&sb, "\",\"gas\":\"0x30d40\"}");
    TRY_FINAL(send_provider_request(ctx->ctx, NULL, "eth_sendTransactionAndWait", sb.data, &tx_receipt), _free(sb.data))
  }
  else {

    if (approve) {
      sb_t sb = {0};
      sb_add_rawbytes(&sb, "{\"to\":\"0x", bytes(token_conf->address, 20), 20);
      sb_add_rawbytes(&sb, "\",\"data\":\"0x095ea7b3", bytes(main_contract, 20), 32);
      sb_add_rawbytes(&sb, NULL, amount, 32);
      sb_add_chars(&sb, "\",\"gas\":\"0x30d40\"}");

      TRY_FINAL(send_provider_request(ctx->ctx, NULL, "eth_sendTransactionAndWait", sb.data, &tx_receipt), _free(sb.data))
    }

    sb_t sb = {0};
    sb_add_rawbytes(&sb, "{\"to\":\"0x", bytes(main_contract, 20), 20);
    sb_add_rawbytes(&sb, "\",\"data\":\"0xe17376b5", bytes(token_conf->address, 20), 32);
    sb_add_rawbytes(&sb, NULL, amount, 32);
    sb_add_rawbytes(&sb, NULL, bytes(account, 20), 32);
    sb_add_chars(&sb, "\",\"gas\":\"0xffd40\"}");

    TRY_FINAL(send_provider_request(ctx->ctx, NULL, "eth_sendTransactionAndWait", sb.data, &tx_receipt), _free(sb.data))
  }

  // now that we have the receipt, we need to find the opId in the log
  bytes32_t event_hash;
  hex_to_bytes("d0943372c08b438a88d4b39d77216901079eda9ca59d45349841c099083b6830", -1, event_hash, 32);
  for (d_iterator_t iter = d_iter(d_get(tx_receipt, K_LOGS)); iter.left; d_iter_next(&iter)) {
    bytes_t* ev = d_get_bytes_at(d_get(iter.token, K_TOPICS), 0);
    if (ev && ev->len == 32 && memcmp(event_hash, ev->data, 32) == 0) {
      bytes_t* data = d_get_bytesk(iter.token, K_DATA);
      if (data && data->len > 64) {
        str_range_t r  = d_to_json(tx_receipt);
        sb_t*       sb = in3_rpc_handle_start(ctx);
        sb_add_chars(sb, "{\"receipt\":");
        sb_add_range(sb, r.data, 0, r.len);
        sb_add_chars(sb, ",\"priorityOpId\":");
        sb_add_int(sb, bytes_to_long(data->data + 64 - 8, 8));
        sb_add_chars(sb, "}");
        ctx_remove_required(ctx->ctx, ctx_find_required(ctx->ctx, "eth_sendTransactionAndWait"), true);
        return in3_rpc_handle_finish(ctx);
      }
    }
  }

  return ctx_set_error(ctx->ctx, "Could not find the serial in the receipt", IN3_EFIND);
}

in3_ret_t zksync_emergency_withdraw(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx, d_token_t* params) {
  uint8_t         aid[4];
  zksync_token_t* token_conf    = NULL;
  uint8_t*        main_contract = conf->main_contract;
  uint8_t*        account       = conf->account;
  uint32_t        account_id    = 0;
  d_token_t*      tx_receipt    = NULL;
  sb_t            sb            = {0};

  CHECK_PARAM_TOKEN(ctx->ctx, params, 0)

  // check main_contract
  TRY(zksync_get_contracts(conf, ctx->ctx, &main_contract))
  TRY(resolve_tokens(conf, ctx->ctx, params_get(params, key("token"), 0), &token_conf))
  TRY(zksync_get_account_id(conf, ctx->ctx, &account_id))
  TRY(zksync_get_account(conf, ctx->ctx, &account))

  int_to_bytes(account_id, aid);
  sb_add_rawbytes(&sb, "{\"to\":\"0x", bytes(main_contract, 20), 0);
  sb_add_rawbytes(&sb, "\",\"data\":\"0x000000e2", bytes(aid, 4), 32);
  sb_add_rawbytes(&sb, "", bytes(token_conf->address, 20), 32);
  sb_add_rawbytes(&sb, "\",\"from\":\"0x", bytes(account, 20), 20);
  sb_add_chars(&sb, "\",\"gas\":\"0x7a120\"}");
  TRY_FINAL(send_provider_request(ctx->ctx, NULL, "eth_sendTransactionAndWait", sb.data, &tx_receipt), _free(sb.data))
  if (d_type(tx_receipt) != T_OBJECT) return ctx_set_error(ctx->ctx, "no txreceipt found, which means the transaction was not succesful", IN3_EFIND);
  str_range_t r = d_to_json(tx_receipt);
  r.data[r.len] = 0;
  return in3_rpc_handle_with_string(ctx, r.data);
}
