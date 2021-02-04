#ifndef IN3_NODE_SELECT_DEF_H
#define IN3_NODE_SELECT_DEF_H

#include "../core/client/context.h"
#include "../core/client/plugin.h"
#include "nodelist.h"

#ifdef NODESELECT_DEF

/**
 * default nodeselect implementation
 */
in3_ret_t in3_nodeselect_def(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx);

/**
 * get access to internal plugin data if registered
 */
static inline in3_nodeselect_def_t* in3_nodeselect_def_data(in3_t* c) {
  in3_nodeselect_wrapper_t* w = in3_plugin_get_data(c, in3_nodeselect_def);
  return w ? w->data : NULL;
}

/**
 * registers the default nodeselect implementation
 */
in3_ret_t in3_register_nodeselect_def(in3_t* c);

#endif //NODESELECT_DEF

#endif //IN3_NODE_SELECT_DEF_H
