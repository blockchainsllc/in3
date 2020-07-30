#include "provider.h"
#include "../../core/client/context_internal.h"
#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "zksync.h"

in3_ret_t send_provider_request(in3_ctx_t* parent, zksync_config_t* conf, char* method, char* params, d_token_t** result) {
  if (params == NULL) params = "";

  in3_ctx_t* ctx = parent;
  for (; ctx; ctx = ctx->required) {
    if (strcmp(d_get_stringk(ctx->requests[0], K_METHOD), method)) continue;
    d_token_t* t = d_get(ctx->requests[0], K_PARAMS);
    if (!t) continue;
    str_range_t p = d_to_json(t);
    if (strncmp(params, p.data + 1, p.len - 2) == 0) break;
  }

  if (ctx)
    switch (in3_ctx_state(ctx)) {
      case CTX_ERROR:
        return ctx_set_error(parent, ctx->error, ctx->verification_state ? ctx->verification_state : IN3_ERPC);
      case CTX_SUCCESS:
        *result = d_get(ctx->responses[0], K_RESULT);
        if (!*result) {
          char* s = d_get_stringk(d_get(ctx->responses[0], K_ERROR), K_MESSAGE);
          return ctx_set_error(parent, s ? s : "error executing provider call", IN3_ERPC);
        }
        return IN3_OK;
      case CTX_WAITING_TO_SEND:
      case CTX_WAITING_FOR_RESPONSE:
        return IN3_WAITING;
    }

  // create the call
  char* req = _malloc(strlen(params) + strlen(method) + 150);
  sprintf(req, "{\"method\":\"%s\",\"params\":[%s],\"in3\":{\"rpc\":\"%s\"}}", method, params, conf->provider_url);
  return ctx_add_required(parent, ctx_new(parent->client, req));
}
