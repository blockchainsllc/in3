#ifndef IN3_NODE_SELECT_DEF_H
#define IN3_NODE_SELECT_DEF_H

#include "../core/client/context.h"
#include "nodelist.h"

typedef struct {
  bool               dirty;           /**< indicates whether the nodelist has been modified after last read from cache */
  uint16_t           avg_block_time;  /**< average block time (seconds) for this chain (calculated internally) */
  unsigned int       nodelist_length; /**< number of nodes in the nodeList */
  uint64_t           last_block;      /**< last blocknumber the nodeList was updated, which is used to detect changed in the nodelist*/
  in3_node_t*        nodelist;        /**< array of nodes */
  in3_node_weight_t* weights;         /**< stats and weights recorded for each node */
  bytes_t**          init_addresses;  /**< array of addresses of nodes that should always part of the nodeList */
  in3_whitelist_t*   whitelist;       /**< if set the whitelist of the addresses. */
  struct {
    uint64_t  exp_last_block; /**< the last_block when the nodelist last changed reported by this node */
    uint64_t  timestamp;      /**< approx. time when nodelist must be updated (i.e. when reported last_block will be considered final) */
    address_t node;           /**< node that reported the last_block which necessitated a nodeList update */
  } * nodelist_upd8_params;
} in3_nodeselect_def_t;

in3_ret_t in3_register_nodeselect_def(in3_t* c);

#endif //IN3_NODE_SELECT_DEF_H
