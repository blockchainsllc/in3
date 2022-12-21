#include "client_data.h"

/**
 * get direct access to plugin data (if registered) based on action function
 */
static inline void* in3_plugin_get_data(in3_t* c, in3_plugin_act_fn fn) {
  for (in3_plugin_t* p = c->plugins; p; p = p->next) {
    if (p->action_fn == fn) return p->data;
  }
  return NULL;
}

in3_ret_t in3_plugin_client_data(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  if (action == PLGN_ACT_GET_DATA) {
    in3_get_data_ctx_t* pctx = plugin_ctx;
    if (pctx->type == GET_DATA_CLIENT_DATA) {
      pctx->data    = plugin_data;
      pctx->cleanup = NULL;
      return IN3_OK;
    }
  }
  return IN3_EIGNORE;
}

void* in3_plugin_get_client_data(in3_t* c) {
  return in3_plugin_get_data(c, in3_plugin_client_data);
}

in3_ret_t in3_register_client_data(in3_t* c, void* data) {
  return in3_plugin_register(c, PLGN_ACT_GET_DATA, in3_plugin_client_data, data, true);
}
