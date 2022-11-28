#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/debug.h"
#include "../../core/util/mem.h"
#include "../../third-party/zkcrypto/lib.h"
#include "../../verifier/eth1/nano/rpcs.h"
#include "zk_helper.h"
#include "zksync.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

in3_ret_t zksync_deposit(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  // check ctx->params
  if (!(d_len(ctx->params) == 1 && d_type(d_get_at(ctx->params, 0)) == T_OBJECT)) {
    CHECK_PARAMS_LEN(ctx->req, ctx->params, 2)
    CHECK_PARAM_NUMBER(ctx->req, ctx->params, 0)
    CHECK_PARAM_TOKEN(ctx->req, ctx->params, 1)
  }

  //  amount
  d_token_t*      tmp           = NULL;
  d_token_t*      tx_receipt    = NULL;
  zksync_token_t* token_conf    = NULL;
  bytes_t         amount        = d_bytes(params_get(ctx->params, key("amount"), 0));
  d_token_t*      token         = params_get(ctx->params, key("token"), 1);
  bool            approve       = d_int(params_get(ctx->params, key("approveDepositAmountForERC20"), 2));
  uint8_t*        main_contract = conf->main_contract;

  // make sure we have an account
  uint8_t* account = conf->account;
  if ((tmp = params_get(ctx->params, key("depositTo"), 3)) && d_is_bytes(tmp)) {
    bytes_t b = d_bytes(tmp);
    if (b.len != 20) return req_set_error(ctx->req, "invalid depositTo", IN3_ERPC);
    account = b.data;
  }
  else if (!account)
    TRY(zksync_get_account(conf, ctx->req, &account))

  // check main_contract
  if (!main_contract) TRY(zksync_get_contracts(conf, ctx->req, &main_contract))

  // get token from the tokenlist
  TRY(resolve_tokens(conf, ctx->req, token, &token_conf))

  if (memiszero(token_conf->address, 20)) { // is eth
    sb_t sb = {0};
    sb_add_rawbytes(&sb, "{\"to\":\"0x", bytes(main_contract, 20), 0);
    sb_add_rawbytes(&sb, "\",\"data\":\"0x2d2da806", bytes(account, 20), 32);
    sb_add_rawbytes(&sb, "\",\"value\":\"0x", amount, 0);
    sb_add_chars(&sb, "\",\"gas\":\"0x30d40\"}");
    TRY_FINAL(send_provider_request(ctx->req, NULL, FN_ETH_SENDTRANSACTIONANDWAIT, sb.data, &tx_receipt), _free(sb.data))
  }
  else {

    if (approve) {
      sb_t sb = {0};
      sb_add_rawbytes(&sb, "{\"to\":\"0x", bytes(token_conf->address, 20), 20);
      sb_add_rawbytes(&sb, "\",\"data\":\"0x095ea7b3", bytes(main_contract, 20), 32);
      sb_add_rawbytes(&sb, NULL, amount, 32);
      sb_add_chars(&sb, "\",\"gas\":\"0x30d40\"}");

      TRY_FINAL(send_provider_request(ctx->req, NULL, FN_ETH_SENDTRANSACTIONANDWAIT, sb.data, &tx_receipt), _free(sb.data))
    }

    sb_t sb = {0};
    sb_add_rawbytes(&sb, "{\"to\":\"0x", bytes(main_contract, 20), 20);
    sb_add_rawbytes(&sb, "\",\"data\":\"0xe17376b5", bytes(token_conf->address, 20), 32);
    sb_add_rawbytes(&sb, NULL, amount, 32);
    sb_add_rawbytes(&sb, NULL, bytes(account, 20), 32);
    sb_add_chars(&sb, "\",\"gas\":\"0xffd40\"}");

    TRY_FINAL(send_provider_request(ctx->req, NULL, FN_ETH_SENDTRANSACTIONANDWAIT, sb.data, &tx_receipt), _free(sb.data))
  }

  // now that we have the receipt, we need to find the opId in the log
  const uint8_t event_hash[] = {0xd0, 0x94, 0x33, 0x72, 0xc0, 0x8b, 0x43, 0x8a, 0x88, 0xd4, 0xb3, 0x9d, 0x77, 0x21, 0x69, 0x01, 0x07, 0x9e, 0xda, 0x9c, 0xa5, 0x9d, 0x45, 0x34, 0x98, 0x41, 0xc0, 0x99, 0x08, 0x3b, 0x68, 0x30};
  for_children_of(iter, d_get(tx_receipt, K_LOGS)) {
    bytes_t ev = d_get_bytes_at(d_get(iter.token, K_TOPICS), 0);
    if (ev.data && ev.len == 32 && memcmp(event_hash, ev.data, 32) == 0) {
      bytes_t data = d_get_bytes(iter.token, K_DATA);
      if (data.data && data.len > 64) {
        str_range_t r  = d_to_json(tx_receipt);
        sb_t*       sb = in3_rpc_handle_start(ctx);
        sb_add_chars(sb, "{\"receipt\":");
        sb_add_range(sb, r.data, 0, r.len);
        sb_add_chars(sb, ",\"priorityOpId\":");
        sb_add_int(sb, bytes_to_long(data.data + 64 - 8, 8));
        sb_add_chars(sb, "}");
        req_remove_required(ctx->req, req_find_required(ctx->req, FN_ETH_SENDTRANSACTIONANDWAIT, NULL), true);
        return in3_rpc_handle_finish(ctx);
      }
    }
  }

  return req_set_error(ctx->req, "Could not find the serial in the receipt", IN3_EFIND);
}

in3_ret_t zksync_emergency_withdraw(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  uint8_t         aid[4];
  zksync_token_t* token_conf    = NULL;
  uint8_t*        main_contract = conf->main_contract;
  uint8_t*        account       = conf->account;
  uint32_t        account_id    = 0;
  d_token_t*      tx_receipt    = NULL;
  sb_t            sb            = {0};

  CHECK_PARAM_TOKEN(ctx->req, ctx->params, 0)

  // check main_contract
  TRY(zksync_get_contracts(conf, ctx->req, &main_contract))
  TRY(resolve_tokens(conf, ctx->req, params_get(ctx->params, key("token"), 0), &token_conf))
  TRY(zksync_get_account_id(conf, ctx->req, &account_id))
  TRY(zksync_get_account(conf, ctx->req, &account))

  int_to_bytes(account_id, aid);
  sb_add_rawbytes(&sb, "{\"to\":\"0x", bytes(main_contract, 20), 0);
  sb_add_rawbytes(&sb, "\",\"data\":\"0x000000e2", bytes(aid, 4), 32);
  sb_add_rawbytes(&sb, "", bytes(token_conf->address, 20), 32);
  sb_add_rawbytes(&sb, "\",\"from\":\"0x", bytes(account, 20), 20);
  sb_add_chars(&sb, "\",\"gas\":\"0x7a120\"}");
  TRY_FINAL(send_provider_request(ctx->req, NULL, FN_ETH_SENDTRANSACTIONANDWAIT, sb.data, &tx_receipt), _free(sb.data))
  if (d_type(tx_receipt) != T_OBJECT) return req_set_error(ctx->req, "no txreceipt found, which means the transaction was not succesful", IN3_EFIND);
  str_range_t r = d_to_json(tx_receipt);
  r.data[r.len] = 0;
  return in3_rpc_handle_with_string(ctx, r.data);
}
