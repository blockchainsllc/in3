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
 * registers the default nodeselect implementation
 */
in3_ret_t in3_register_nodeselect_def(in3_t* c);

#endif //NODESELECT_DEF

#endif //IN3_NODE_SELECT_DEF_H
