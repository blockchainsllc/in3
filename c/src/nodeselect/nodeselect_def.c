#include "nodeselect_def.h"

// function decls
static in3_ret_t nodeselect_def_pick(in3_nodeselect_t* nodeselect, in3_ctx_t* ctx, int count, in3_node_filter_t filter);
static void      nodeselect_def_blacklist(in3_nodeselect_t* nodeselect, in3_node_weight_t* node);
static char*     nodeselect_def_configure(in3_plugin_t* plugin, const char* config);

// variable defs
static in3_nodeselect_t nodeselect_def        = {.blacklist = nodeselect_def_blacklist, .pick = nodeselect_def_pick};
const in3_plugin_t      nodeselect_def_plugin = {.internal = &nodeselect_def, .type = PLUGIN_NODESELECT, .configure = nodeselect_def_configure};

// function defs
static in3_ret_t nodeselect_def_pick(in3_nodeselect_t* nodeselect, in3_ctx_t* ctx, int count, in3_node_filter_t filter) {
  return IN3_OK;
}

static void nodeselect_def_blacklist(in3_nodeselect_t* nodeselect, in3_node_weight_t* node) {
}

static char* nodeselect_def_configure(in3_plugin_t* plugin, const char* config) {
  return NULL;
}