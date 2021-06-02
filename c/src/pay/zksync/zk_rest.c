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

#define CHECK_REST_API(ctx,conf) if (!conf->rest_api)   return req_set_error(ctx->req,"No zksync Rest-Api set in config",IN3_ECONFIG);


in3_ret_t zksync_tx_data(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
    CHECK_REST_API(ctx,conf)
    CHECK_PARAMS_LEN(ctx->req, ctx->params, 1)
    CHECK_PARAM_TYPE(ctx->req, ctx->params, 0, T_BYTES)
    CHECK_PARAM_LEN(ctx->req, ctx->params, 0, 32)

    d_token_t* res = NULL;
    in3_req_t* req = NULL;
    sb_t sb = {0};
    sb_add_chars(&sb, "\"GET\",\"");
    sb_add_escaped_chars(&sb, conf->rest_api);
    sb_add_rawbytes(&sb,"/transactions_all/0x",d_to_bytes(ctx->params+1),32);
    sb_add_chars(&sb, "\"");

    TRY_FINAL(req_send_sub_request(ctx->req, "in3_http", sb.data, NULL, &res, &req), _free(sb.data))

    char* resp = d_create_json(req->response_context,res);
    in3_rpc_handle_with_string(ctx,resp);
    _free(resp);
    return IN3_OK;
}


in3_ret_t zksync_account_history(zksync_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
    CHECK_REST_API(ctx,conf)
    CHECK_PARAMS_LEN(ctx->req, ctx->params, 1)
    CHECK_PARAM_ADDRESS(ctx->req, ctx->params, 0)

    d_token_t* ref_tx = d_get_at(ctx->params,1);
    d_token_t* limit = d_get_at(ctx->params,2);
    if (!limit && d_type(ref_tx)==T_INTEGER) {
      limit = ref_tx;
      ref_tx = NULL;
    }
    if (d_type(ref_tx)==T_NULL) ref_tx = NULL;
    if (ref_tx && d_type(ref_tx)!=T_STRING) return req_set_error(ctx->req,"The 2nd argument in account History (base tx) must be a string starting with < or > and the transactionId",IN3_ECONFIG);
    if (limit && d_type(limit)!=T_INTEGER) return req_set_error(ctx->req,"The 3rd argument in account History (limit) must be a integer!",IN3_ECONFIG);

    d_token_t* res = NULL;
    in3_req_t* req = NULL;
    sb_t sb = {0};
    sb_add_chars(&sb, "\"GET\",\"");
    sb_add_escaped_chars(&sb, conf->rest_api);
    sb_add_rawbytes(&sb,"/account/0x",d_to_bytes(ctx->params+1),20);
    sb_add_chars(&sb, "/history/");
    if (!ref_tx) {
      sb_add_chars(&sb, "0/");
      sb_add_int(&sb, limit ?  (int64_t)d_long(limit) : 100);
    }
    else if (strcmp(d_string(ref_tx),"pending")==0)  
       sb_add_chars(&sb, "newer_than");
    else if (ref_tx->data[0]=='<' || ref_tx->data[0]=='>') {
       sb_add_chars(&sb, ref_tx->data[0]=='<' ? "older_than?tx_id=" : "newer_than?tx_id=");
       sb_add_chars(&sb, d_string(ref_tx));
       sb_add_chars(&sb, "&limit=");
       sb_add_int(&sb, limit ? (int64_t)d_long(limit) : 100);
    }
    else {
      _free(sb.data);
      return req_set_error(ctx->req,"Invalid base_tx it must a tx_id with <,> or pending",IN3_ECONFIG);
    }
    sb_add_chars(&sb, "\"");

    TRY_FINAL(req_send_sub_request(ctx->req, "in3_http", sb.data, NULL, &res, &req), _free(sb.data))

    char* resp = d_create_json(req->response_context,res);
    in3_rpc_handle_with_string(ctx,resp);
    _free(resp);
    return IN3_OK;
}
