#include "nodeselect_def.h"

static in3_ret_t nl_pick_data(void* plugin_data, void* plugin_ctx) {
  return IN3_OK;
}

static in3_ret_t nl_pick_signer(void* plugin_data, void* plugin_ctx) {
  return IN3_OK;
}

static in3_ret_t nl_pick_followup(void* plugin_data, void* plugin_ctx) {
  return IN3_OK;
}

static in3_ret_t nl_cache_set(void* plugin_data, void* plugin_ctx) {
  return IN3_OK;
}

static in3_ret_t nl_cache_get(void* plugin_data, void* plugin_ctx) {
  return IN3_OK;
}

static in3_ret_t nl_cache_clear(void* plugin_data, void* plugin_ctx) {
  return IN3_OK;
}

static in3_ret_t nodeselect(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  switch (action) {
    case PLGN_ACT_CACHE_SET:
      return nl_cache_set(plugin_data, plugin_ctx);
    case PLGN_ACT_CACHE_GET:
      return nl_cache_get(plugin_data, plugin_ctx);
    case PLGN_ACT_CACHE_CLEAR:
      return nl_cache_clear(plugin_data, plugin_ctx);
    case PLGN_ACT_NL_PICK_DATA:
      return nl_pick_data(plugin_data, plugin_ctx);
    case PLGN_ACT_NL_PICK_SIGNER:
      return nl_pick_signer(plugin_data, plugin_ctx);
    case PLGN_ACT_NL_PICK_FOLLOWUP:
      return nl_pick_followup(plugin_data, plugin_ctx);
    default: break;
  }
  return IN3_EIGNORE;
}

in3_ret_t in3_register_nodeselect_def(in3_t* c) {
  return in3_plugin_register(c, PLGN_ACT_NODELIST | PLGN_ACT_CACHE, nodeselect, NULL, false);
}
