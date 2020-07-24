#include "nodeselect_def.h"

static in3_ret_t pick_data(void* plugin_data, void* plugin_ctx) {
  return IN3_OK;
}

static in3_ret_t pick_signer(void* plugin_data, void* plugin_ctx) {
  return IN3_OK;
}

static in3_ret_t pick_followup(void* plugin_data, void* plugin_ctx) {
  return IN3_OK;
}

static in3_ret_t nodeselect(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  switch (action) {
    case PLGN_ACT_NL_PICK_DATA:
      return pick_data(plugin_data, plugin_ctx);
    case PLGN_ACT_NL_PICK_SIGNER:
      return pick_signer(plugin_data, plugin_ctx);
    case PLGN_ACT_NL_PICK_FOLLOWUP:
      return pick_followup(plugin_data, plugin_ctx);
    default: break;
  }
  return IN3_EIGNORE;
}

in3_ret_t in3_register_nodeselect_def(in3_t* c) {
  return in3_plugin_register(c, PLGN_ACT_NODELIST, nodeselect, NULL, false);
}
