
#include "swift.h"
#include "../../core/client/client.h"
#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/client/request.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../third-party/crypto/sha3.h"

static in3_ret_t handle(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  swift_cb_t* conf = plugin_data;
  switch (action) {
    case PLGN_ACT_TERM:
      _free(plugin_data);
      return IN3_OK;
    case PLGN_ACT_CACHE_GET: {
      in3_cache_ctx_t* _Nonnull ctx = plugin_ctx;
      return conf->cache_get(ctx);
    }
    case PLGN_ACT_CACHE_SET: {
      in3_cache_ctx_t* _Nonnull ctx = plugin_ctx;
      return conf->cache_set(ctx);
    }
    case PLGN_ACT_SIGN_ACCOUNT: {
      in3_sign_account_ctx_t* _Nonnull ctx = plugin_ctx;
      char* accounts                       = conf->sign_accounts(ctx);
      if (accounts) {
        size_t l = strlen(accounts);
        if (l % 40 == 0) {
          ctx->accounts_len = l / 40;
          ctx->accounts     = _malloc(l / 2);
          hex_to_bytes(accounts, l, ctx->accounts, l / 2);
        }
        _free(accounts);
        return l > 2 ? IN3_OK : IN3_EIGNORE;
      }
      return IN3_EIGNORE;
    }
    case PLGN_ACT_SIGN: {
      in3_sign_ctx_t* _Nonnull ctx = plugin_ctx;

      in3_sign_account_ctx_t sctx = {0};
      sctx.req                    = ctx->req;
      char* accounts              = conf->sign_accounts(&sctx);
      if (!accounts) return IN3_EIGNORE;
      int l = strlen(accounts);
      if (l < 3) {
        _free(accounts);
        return IN3_EIGNORE;
      }

      if (ctx->account.len == 20) {
        char adr[41];
        bytes_to_hex_string(adr, "", ctx->account, "");
        bool found = false;
        for (int i = 2; i < l; i += 40) {
          if (strncmp(adr, accounts + i, 40) == 0) {
            found = true;
            break;
          }
        }
        _free(accounts);
        if (!found) return IN3_EIGNORE;
      }
      else
        _free(accounts);

      bytes_t   signature = NULL_BYTES;
      bytes32_t msghash;

      struct SHA3_CTX kctx;
      sha3_256_Init(&kctx);
      sha3_Update(&kctx, ctx->message.data, ctx->message.len);
      if (ctx->account.data) sha3_Update(&kctx, ctx->account.data, ctx->account.len);
      keccak_Final(&kctx, msghash);

      TRY(req_send_sign_request(ctx->req, ctx->digest_type, ctx->payload_type, &signature, ctx->message, ctx->account, ctx->meta, bytes(msghash, 32)))
      ctx->signature = bytes_dup(signature);
      return IN3_OK;
    }
    case PLGN_ACT_CACHE_CLEAR:
      return conf->cache_clear();

    default:
      return IN3_ENOTSUP;
  }
  return IN3_EIGNORE;
}

in3_ret_t in3_register_swift(in3_t* c, swift_cb_t* cbs) {
  swift_cb_t* ptr = _malloc(sizeof(swift_cb_t));
  memcpy(ptr, cbs, sizeof(swift_cb_t));
  return in3_plugin_register(c, PLGN_ACT_CACHE_GET | PLGN_ACT_CACHE_SET | PLGN_ACT_CACHE_CLEAR | PLGN_ACT_TERM | PLGN_ACT_SIGN_ACCOUNT | PLGN_ACT_SIGN, handle, ptr, true);
}

char* sign_get_method(in3_req_t* r) {
  return d_get_string(r->requests[0], K_METHOD);
}

bytes_t sign_get_message(in3_req_t* r) {
  d_token_t* params = d_get(r->requests[0], K_PARAMS);
  return d_to_bytes(d_get_at(params, 0));
}
uint8_t* sign_get_from(in3_req_t* r) {
  d_token_t* params = d_get(r->requests[0], K_PARAMS);
  return d_to_bytes(d_get_at(params, 1)).data;
}

int sign_get_payload_type(in3_req_t* r) {
  d_token_t* params = d_get(r->requests[0], K_PARAMS);
  return d_get_int_at(params, 2);
}

char* sign_get_metadata(in3_req_t* r) {
  d_token_t* params = d_get(r->requests[0], K_PARAMS);
  d_token_t* meta   = d_get_at(params, 3);
  return meta ? d_create_json(r->request_context, meta) : NULL;
}
