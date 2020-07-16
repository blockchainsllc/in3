#ifndef IN3_NODE_SELECT_DEF_H
#define IN3_NODE_SELECT_DEF_H

#include "../core/client/context.h"
#include "../core/client/nodelist.h"

typedef struct in3_nodeselect_ in3_nodeselect_t;
struct in3_nodeselect_ {
  in3_ret_t (*pick)(in3_nodeselect_t* nodeselect, in3_ctx_t* ctx, int count, in3_node_filter_t filter); /**< picks and fills ctx nodes based on filter (if specified) */
  void (*blacklist)(in3_nodeselect_t* nodeselect, in3_node_weight_t* node);                             /**< blacklists specified node */
};

extern const in3_plugin_t nodeselect_def_plugin;

#endif //IN3_NODE_SELECT_DEF_H
