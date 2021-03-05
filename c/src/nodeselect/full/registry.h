#ifndef IN3_NODE_SELECT_REGISTRY_H
#define IN3_NODE_SELECT_REGISTRY_H

#include "../../core/client/plugin.h"
#include "../../core/util/data.h"
#include "../../core/util/error.h"
#include "nodeselect_def.h"

NONULL_FOR((1))
in3_ret_t eth_verify_in3_nodelist(in3_nodeselect_def_t* data, in3_vctx_t* vc, uint32_t node_limit, bytes_t* seed, d_token_t* required_addresses);

#ifdef NODESELECT_DEF_WL
NONULL in3_ret_t eth_verify_in3_whitelist(in3_nodeselect_def_t* data, in3_vctx_t* vc);
#endif

#endif //IN3_NODE_SELECT_REGISTRY_H
