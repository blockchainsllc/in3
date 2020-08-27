#include "provider.h"
#include "../../core/client/context_internal.h"
#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "zksync.h"
in3_ret_t send_provider_request(in3_ctx_t* parent, zksync_config_t* conf, char* method, char* params, d_token_t** result) {
  if (params == NULL) params = "";
  char* in3 = conf ? alloca(strlen(conf->provider_url) + 26) : NULL;
  if (in3) sprintf(in3, "{\"rpc\":\"%s\"}", conf->provider_url);
  return ctx_send_sub_request(parent, method, params, in3, result);
}
