/**
 *
 */
// @PUBLIC_HEADER
#ifndef IN3_NODE_SELECT_DEF_H
#define IN3_NODE_SELECT_DEF_H

#include "plugin.h"
#include "request.h"
#include "nodelist.h"

#ifdef NODESELECT_DEF

/**
 * default nodeselect implementation
 */
in3_ret_t in3_nodeselect_handle_action(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx);

/**
 * get access to internal plugin data if registered
 */
static inline in3_nodeselect_def_t* in3_nodeselect_def_data(in3_t* c) {
  for (in3_plugin_t* p = c->plugins; p; p = p->next) {
    if (p->action_fn == in3_nodeselect_handle_action) return in3_get_nodelist_data(p->data, c->chain.id);
  }
  return NULL;
}

/**
 * registers the default nodeselect implementation
 */
in3_ret_t in3_register_nodeselect_def(in3_t* c);

#endif // NODESELECT_DEF

#endif // IN3_NODE_SELECT_DEF_H
