#include "../../core/client/plugin.h"
#include "zksync.h"

#ifndef ZKSYNC_PROVIDER_H
#define ZKSYNC_PROVIDER_H

in3_ret_t send_provider_request(in3_ctx_t* parent, zksync_config_t* conf, char* method, char* params, d_token_t** result);

#endif