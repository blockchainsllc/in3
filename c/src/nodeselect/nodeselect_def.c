#include "nodeselect_def.h"
#include "../core/client/context_internal.h"
#include "../core/client/keys.h"
#include "../core/util/bitset.h"
#include "../core/util/debug.h"
#include "../core/util/log.h"
#include "cache.h"
#include "nodeselect_def_cfg.h"
#include "registry.h"

#define BLACKLISTTIME (24 * 3600)
#define WAIT_TIME_CAP 3600

static in3_ret_t rpc_verify(in3_nodeselect_def_t* data, in3_vctx_t* vc) {
  char*      method = NULL;
  d_token_t* params = d_get(vc->request, K_PARAMS);

  // do we support this request?
  if (!(method = d_get_stringk(vc->request, K_METHOD)))
    return vc_err(vc, "No Method in request defined!");
  if (vc->chain->type != CHAIN_ETH && strcmp(method, "in3_nodeList")) return IN3_EIGNORE;
  if (in3_ctx_get_proof(vc->ctx, vc->index) == PROOF_NONE) return IN3_OK;

  // do we have a result? if not it is a valid error-response
  if (!vc->result)
    return IN3_OK;

  if (strcmp(method, "in3_nodeList") == 0)
    return eth_verify_in3_nodelist(data, vc, d_get_int_at(params, 0), d_get_bytes_at(params, 1), d_get_at(params, 2));
#ifdef NODESELECT_DEF_WL
  else if (strcmp(method, "in3_whiteList") == 0)
    return eth_verify_in3_whitelist(data, vc);
#endif
  else
    return IN3_EIGNORE;
}

static uint16_t avg_block_time_for_chain_id(chain_id_t id) {
  switch (id) {
    case CHAIN_ID_MAINNET:
    case CHAIN_ID_GOERLI: return 15;
    default: return 5;
  }
}

static in3_ret_t add_node(in3_nodeselect_def_t* data, char* url, in3_node_props_t props, address_t address) {
  assert(data);
  assert(url);
  assert(address);

  in3_node_t*  node       = NULL;
  unsigned int node_index = data->nodelist_length;
  for (unsigned int i = 0; i < data->nodelist_length; i++) {
    if (memcmp(data->nodelist[i].address, address, 20) == 0) {
      node       = data->nodelist + i;
      node_index = i;
      break;
    }
  }
  if (!node) {
    // init or change the size ofthe nodelist
    data->nodelist = data->nodelist
                         ? _realloc(data->nodelist, sizeof(in3_node_t) * (data->nodelist_length + 1), sizeof(in3_node_t) * data->nodelist_length)
                         : _calloc(data->nodelist_length + 1, sizeof(in3_node_t));
    // the weights always have to have the same size
    data->weights = data->weights
                        ? _realloc(data->weights, sizeof(in3_node_weight_t) * (data->nodelist_length + 1), sizeof(in3_node_weight_t) * data->nodelist_length)
                        : _calloc(data->nodelist_length + 1, sizeof(in3_node_weight_t));
    if (!data->nodelist || !data->weights) return IN3_ENOMEM;
    node = data->nodelist + data->nodelist_length;
    memcpy(node->address, address, 20);
    node->index    = data->nodelist_length;
    node->capacity = 1;
    node->deposit  = 0;
    BIT_CLEAR(node->attrs, ATTR_WHITELISTED);
    data->nodelist_length++;
  }
  else
    _free(node->url);

  node->props = props;
  node->url   = _malloc(strlen(url) + 1);
  memcpy(node->url, url, strlen(url) + 1);

  in3_node_weight_t* weight   = data->weights + node_index;
  weight->blacklisted_until   = 0;
  weight->response_count      = 0;
  weight->total_response_time = 0;
  return IN3_OK;
}

static in3_ret_t clear_nodes(in3_nodeselect_def_t* data) {
  assert(data);

  in3_nodelist_clear(data);
  data->nodelist        = NULL;
  data->weights         = NULL;
  data->nodelist_length = 0;
  return IN3_OK;
}

static in3_ret_t config_set(in3_nodeselect_def_t* data, in3_configure_ctx_t* ctx) {
  char*       res   = NULL;
  json_ctx_t* json  = ctx->json;
  d_token_t*  token = ctx->token;

  if (token->key == key("nodeRegistry")) {
    EXPECT_TOK_OBJ(token);

#ifdef NODESELECT_DEF_WL
    bool has_wlc = false, has_man_wl = false;
#endif
    for (d_iterator_t cp = d_iter(token); cp.left; d_iter_next(&cp)) {
      if (cp.token->key == key("contract")) {
        EXPECT_TOK_ADDR(cp.token);
        memcpy(data->contract, cp.token->data, cp.token->len);
      }
      else if (cp.token->key == key("registryId")) {
        EXPECT_TOK_B256(cp.token);
        bytes_t reg_id = d_to_bytes(cp.token);
        memcpy(data->registry_id, reg_id.data, 32);
      }
#ifdef NODESELECT_DEF_WL
      else if (cp.token->key == key("whiteListContract")) {
        EXPECT_TOK_ADDR(cp.token);
        EXPECT_CFG(!has_man_wl, "cannot specify manual whiteList and whiteListContract together!");
        has_wlc = true;
        in3_whitelist_clear(data->whitelist);
        data->whitelist               = _calloc(1, sizeof(in3_whitelist_t));
        data->whitelist->needs_update = true;
        memcpy(data->whitelist->contract, cp.token->data, 20);
      }
      else if (cp.token->key == key("whiteList")) {
        EXPECT_TOK_ARR(cp.token);
        EXPECT_CFG(!has_wlc, "cannot specify manual whiteList and whiteListContract together!");
        has_man_wl = true;
        int len = d_len(cp.token), i = 0;
        in3_whitelist_clear(data->whitelist);
        data->whitelist            = _calloc(1, sizeof(in3_whitelist_t));
        data->whitelist->addresses = bytes(_calloc(1, len * 20), len * 20);
        for (d_iterator_t n = d_iter(cp.token); n.left; d_iter_next(&n), i += 20) {
          EXPECT_TOK_ADDR(n.token);
          const uint8_t* whitelist_address = d_bytes(n.token)->data;
          for (uint32_t j = 0; j < data->whitelist->addresses.len; j += 20) {
            if (!memcmp(whitelist_address, data->whitelist->addresses.data + j, 20)) {
              in3_whitelist_clear(data->whitelist);
              data->whitelist = NULL;
              EXPECT_TOK(cp.token, false, "duplicate address!");
            }
          }
          d_bytes_to(n.token, data->whitelist->addresses.data + i, 20);
        }
      }
#endif
      else if (cp.token->key == key("needsUpdate")) {
        EXPECT_TOK_BOOL(cp.token);
        if (!d_int(cp.token)) {
          if (data->nodelist_upd8_params) {
            _free(data->nodelist_upd8_params);
            data->nodelist_upd8_params = NULL;
          }
        }
        else if (!data->nodelist_upd8_params)
          data->nodelist_upd8_params = _calloc(1, sizeof(*(data->nodelist_upd8_params)));
      }
      else if (cp.token->key == key("avgBlockTime")) {
        EXPECT_TOK_U16(cp.token);
        data->avg_block_time = (uint16_t) d_int(cp.token);
      }
      else if (cp.token->key == key("nodeList")) {
        EXPECT_TOK_ARR(cp.token);
        if (clear_nodes(data) < 0) goto cleanup;
        int i = 0;
        for (d_iterator_t n = d_iter(cp.token); n.left; d_iter_next(&n), i++) {
          EXPECT_CFG(d_get(n.token, key("url")) && d_get(n.token, key("address")), "expected URL & address");
          EXPECT_TOK_STR(d_get(n.token, key("url")));
          EXPECT_TOK_ADDR(d_get(n.token, key("address")));
          EXPECT_CFG(add_node(data, d_get_string(n.token, "url"),
                              d_get_longkd(n.token, key("props"), 65535),
                              d_get_byteskl(n.token, key("address"), 20)->data) == IN3_OK,
                     "add node failed");
#ifndef __clang_analyzer__
          BIT_SET(data->nodelist[i].attrs, ATTR_BOOT_NODE);
#endif
        }
      }
    }
#ifdef NODESELECT_DEF_WL
    in3_client_run_chain_whitelisting(data);
#endif
  }
  else if (token->key == key("rpc")) {
    EXPECT_TOK_STR(token);
    in3_t* c          = ctx->client;
    c->proof          = PROOF_NONE;
    c->chain.chain_id = CHAIN_ID_LOCAL;
    c->request_count  = 1;

    clear_nodes(data);
    data->nodelist = _calloc(1, sizeof(in3_node_t));
    data->nodelist_length++;
    in3_node_t* n = &data->nodelist[0];
    if (n->url) _free(n->url);
    n->url = _strdupn(d_string(token), -1);
    _free(data->nodelist_upd8_params);
    data->nodelist_upd8_params = NULL;
  }
  else if (token->key == key("replaceLatestBlock")) {
    EXPECT_TOK_U8(token);
    in3_node_props_set(&ctx->client->node_props, NODE_PROP_MIN_BLOCK_HEIGHT, d_int(token));
  }
  else {
    return IN3_EIGNORE;
  }

  // for supported chains we will get the registryId & contract from def cfg lazily
  if (!nodeselect_def_cfg_data(ctx->client->chain.chain_id).data)
    EXPECT_CFG(!memiszero(data->registry_id, 32) && !memiszero(data->contract, 20), "missing registryId/contract!");

cleanup:
  ctx->error_msg = res;
  return ctx->error_msg ? IN3_ECONFIG : IN3_OK;
}

static in3_ret_t config_get(in3_nodeselect_def_t* data, in3_get_config_ctx_t* ctx) {
  sb_t*  sb = ctx->sb;
  in3_t* c  = ctx->client;

  if (c->chain.chain_id == CHAIN_ID_LOCAL)
    add_string(sb, ',', "rpc", data->nodelist->url);

  sb_add_chars(sb, ",\"nodeRegistry\":");
  add_hex(sb, '{', "contract", bytes(data->contract, 20));
#ifdef NODESELECT_DEF_WL
  if (data->whitelist)
    add_hex(sb, ',', "whiteListContract", bytes(data->whitelist->contract, 20));
#endif
  add_hex(sb, ',', "registryId", bytes(data->registry_id, 32));
  add_bool(sb, ',', "needsUpdate", data->nodelist_upd8_params != NULL);
  add_uint(sb, ',', "avgBlockTime", data->avg_block_time);
  sb_add_chars(sb, ",\"nodeList\":[");

  for (unsigned int j = 0; j < data->nodelist_length; j++) {
    if ((data->nodelist[j].attrs & ATTR_BOOT_NODE) == 0) continue;
    if (sb->data[sb->len - 1] != '[') sb_add_char(sb, ',');
    add_string(sb, '{', "url", data->nodelist[j].url);
    add_uint(sb, ',', "props", data->nodelist[j].props);
    add_hex(sb, ',', "address", bytes(data->nodelist[j].address, 20));
    sb_add_char(sb, '}');
  }

  if (sb->data[sb->len - 1] == '[') {
    sb->len -= 13;
    sb_add_char(sb, '}');
  }
  else
    sb_add_chars(sb, "]}");
  return IN3_OK;
}

static in3_ret_t init_boot_nodes(in3_nodeselect_def_t* data, in3_t* c) {
  json_ctx_t* json = nodeselect_def_cfg(c->chain.chain_id);
  if (json == NULL)
    return IN3_ECONFIG;

  in3_configure_ctx_t cctx = {.client = c, .json = json, .token = json->result + 1, .error_msg = NULL};
  in3_ret_t           ret  = config_set(data, &cctx);
  json_free(json);
  if (IN3_OK != ret) {
    in3_log_error("nodeselect config error: %s\n", cctx.error_msg);
    return IN3_ECONFIG;
  }

  for (unsigned int i = 0; i < data->nodelist_length; ++i)
    BIT_SET(data->nodelist[i].attrs, ATTR_BOOT_NODE);

  return in3_cache_init(c, data);
}

static in3_ret_t pick_data(in3_nodeselect_def_t* data, in3_ctx_t* ctx) {
  // init cache lazily,
  // this also means we can be sure that all other related plugins are registered by now
  if (data->nodelist == NULL && IN3_ECONFIG == init_boot_nodes(data, ctx->client))
    return IN3_ECONFIG;

  in3_node_filter_t filter = NODE_FILTER_INIT;
  filter.nodes             = d_get(d_get(ctx->requests[0], K_IN3), K_DATA_NODES);
  filter.props             = (ctx->client->node_props & 0xFFFFFFFF) | NODE_PROP_DATA | ((ctx->client->flags & FLAGS_HTTP) ? NODE_PROP_HTTP : 0) | (in3_ctx_get_proof(ctx, 0) != PROOF_NONE ? NODE_PROP_PROOF : 0);
  return in3_node_list_pick_nodes(ctx, data, &ctx->nodes, ctx->client->request_count, filter);
}

NONULL static bool auto_ask_sig(const in3_ctx_t* ctx) {
  return (ctx_is_method(ctx, "in3_nodeList") && !(ctx->client->flags & FLAGS_NODE_LIST_NO_SIG) && ctx->client->chain.chain_id != CHAIN_ID_BTC);
}

static in3_ret_t pick_signer(in3_nodeselect_def_t* data, in3_ctx_t* ctx) {
  const in3_t* c = ctx->client;

  if (in3_ctx_get_proof(ctx, 0) == PROOF_NONE && !auto_ask_sig(ctx))
    return IN3_OK;

  // For nodeList request, we always ask for proof & atleast one signature
  uint8_t total_sig_cnt = c->signature_count
                              ? c->signature_count
                              : (auto_ask_sig(ctx) ? 1 : 0);

  if (total_sig_cnt) {
    node_match_t*     signer_nodes = NULL;
    in3_node_filter_t filter       = NODE_FILTER_INIT;
    filter.nodes                   = d_get(d_get(ctx->requests[0], K_IN3), K_SIGNER_NODES);
    filter.props                   = c->node_props | NODE_PROP_SIGNER;
    const in3_ret_t res            = in3_node_list_pick_nodes(ctx, data, &signer_nodes, total_sig_cnt, filter);
    if (res < 0)
      return ctx_set_error(ctx, "Could not find any nodes for requesting signatures", res);
    if (ctx->signers) _free(ctx->signers);
    const int node_count  = ctx_nodes_len(signer_nodes);
    ctx->signers_length   = node_count;
    ctx->signers          = _malloc(20 * node_count); // 20 bytes per address
    const node_match_t* w = signer_nodes;
    in3_node_t*         n = NULL;
    for (int i = 0; i < node_count; i++) {
      n = get_node(data, w);
      if (n) memcpy(ctx->signers + i * 20, n->address, 20);
      w = w->next;
    }
    if (signer_nodes) in3_ctx_free_nodes(signer_nodes);
  }

  return IN3_OK;
}

NONULL in3_ret_t handle_failable(in3_nodeselect_def_t* data, in3_ctx_t* ctx) {
  in3_ret_t res = IN3_OK;

  // blacklist node that gave us an error response for nodelist (if not first update)
  // and clear nodelist params
  if (nodelist_not_first_upd8(data))
    blacklist_node_addr(data, data->nodelist_upd8_params->node, BLACKLISTTIME);
  _free(data->nodelist_upd8_params);
  data->nodelist_upd8_params = NULL;

  if (ctx->required) {
    // if first update return error otherwise return IN3_OK, this is because first update is
    // always from a boot node which is presumed to be trusted
    if (nodelist_first_upd8(data))
      res = ctx_set_error(ctx, ctx->required->error ? ctx->required->error : "error handling subrequest", IN3_ERPC);

    if (res == IN3_OK) res = ctx_remove_required(ctx, ctx->required, true);
  }

  return res;
}

static uint16_t update_waittime(uint64_t nodelist_block, uint64_t current_blk, uint8_t repl_latest, uint16_t avg_blktime) {
  if (nodelist_block > current_blk)
    // misbehaving node, so allow to update right away and it'll get blacklisted due to the exp_last_block mechanism
    return 0;

  uint64_t diff = current_blk - nodelist_block;
  if (diff >= repl_latest)
    return 0;
  // we need to cap wait time as we might end up waiting for too long for chains with higher block time
  return min((repl_latest - diff) * avg_blktime, WAIT_TIME_CAP);
}

static void check_autoupdate(const in3_ctx_t* ctx, in3_nodeselect_def_t* data, d_token_t* response_in3, node_match_t* node) {
  assert_in3_ctx(ctx);
  assert(data);
  if ((ctx->client->flags & FLAGS_AUTO_UPDATE_LIST) == 0) return;

  if (d_get_longk(response_in3, K_LAST_NODE_LIST) > d_get_longk(response_in3, K_CURRENT_BLOCK)) {
    // this shouldn't be possible, so we ignore this lastNodeList and do NOT try to update the nodeList
    return;
  }

  if (d_get_longk(response_in3, K_LAST_NODE_LIST) > data->last_block) {
    if (data->nodelist_upd8_params == NULL)
      data->nodelist_upd8_params = _malloc(sizeof(*(data->nodelist_upd8_params)));
    in3_node_t* n = get_node(data, node);
    if (n) {
      // overwrite old params since we have a newer nodelist update now
      memcpy(data->nodelist_upd8_params->node, n->address, 20);
      data->nodelist_upd8_params->exp_last_block = d_get_longk(response_in3, K_LAST_NODE_LIST);
      data->nodelist_upd8_params->timestamp      = in3_time(NULL) + update_waittime(d_get_longk(response_in3, K_LAST_NODE_LIST),
                                                                               d_get_longk(response_in3, K_CURRENT_BLOCK),
                                                                               ctx->client->replace_latest_block,
                                                                               data->avg_block_time);
    }
  }

#ifdef NODESELECT_DEF_WL
  if (data->whitelist && d_get_longk(response_in3, K_LAST_WHITE_LIST) > data->whitelist->last_block)
    data->whitelist->needs_update = true;
#endif
}

static void handle_times(in3_nodeselect_def_t* data, node_match_t* node, in3_response_t* response) {
  if (!node || get_node(data, node)->blocked || !response || !response->time) return;
  in3_node_weight_t* w = get_node_weight(data, node);
  if (!w) return;
  w->response_count++;
  w->total_response_time += response->time;
  response->time = 0; // make sure we count the time only once
}

static in3_ret_t pick_followup(in3_nodeselect_def_t* data, in3_nl_followop_type_t* fctx) {
  in3_ctx_t*    ctx         = fctx->ctx;
  node_match_t* vnode       = fctx->node;
  node_match_t* node        = ctx->nodes;
  int           nodes_count = ctx->nodes == NULL ? 1 : ctx_nodes_len(ctx->nodes);

  for (int n = 0; n < nodes_count; n++, node = node ? node->next : NULL)
    handle_times(data, node, ctx->raw_response + n);

  // check auto update opts only if this node wasn't blacklisted (due to wrong result/proof)
  if (!is_blacklisted(get_node(data, node)) && ctx->responses && d_get(ctx->responses[0], K_IN3) && !d_get(ctx->responses[0], K_ERROR))
    check_autoupdate(ctx, data, d_get(ctx->responses[0], K_IN3), vnode);

  // update weights in the cache
  return in3_cache_store_nodelist(ctx->client, data);
}

static in3_ret_t chain_change(in3_nodeselect_def_t* data, in3_t* c) {
  data->avg_block_time = avg_block_time_for_chain_id(c->chain.chain_id);
  return init_boot_nodes(data, c);
}

in3_ret_t in3_nodeselect_def(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  in3_nodeselect_def_t* data = plugin_data;
  switch (action) {
    case PLGN_ACT_INIT:
      return IN3_OK;
    case PLGN_ACT_TERM:
      in3_nodelist_clear(data);
#ifdef NODESELECT_DEF_WL
      in3_whitelist_clear(data->whitelist);
#endif
      _free(data->nodelist_upd8_params);
      _free(data);
      return IN3_OK;
    case PLGN_ACT_RPC_VERIFY:
      return rpc_verify(data, (in3_vctx_t*) plugin_ctx);
    case PLGN_ACT_CONFIG_SET:
      return config_set(data, (in3_configure_ctx_t*) plugin_ctx);
    case PLGN_ACT_CONFIG_GET:
      return config_get(data, (in3_get_config_ctx_t*) plugin_ctx);
    case PLGN_ACT_NL_PICK: {
      in3_nl_pick_ctx_t* pctx = plugin_ctx;
      return pctx->type == NL_DATA ? pick_data(data, pctx->ctx) : pick_signer(data, pctx->ctx);
    }
    case PLGN_ACT_NL_PICK_FOLLOWUP:
      return pick_followup(data, plugin_ctx);
    case PLGN_ACT_NL_BLACKLIST:
      return blacklist_node(data, ((node_match_t*) plugin_ctx)->index, BLACKLISTTIME);
    case PLGN_ACT_NL_FAILABLE:
      return handle_failable(data, plugin_ctx);
    case PLGN_ACT_CHAIN_CHANGE:
      return chain_change(data, plugin_ctx);
    case PLGN_ACT_GET_DATA: {
      in3_get_data_ctx_t* pctx = plugin_ctx;
      if (pctx->type == GET_DATA_REGISTRY_ID) {
        pctx->data    = data->registry_id;
        pctx->cleanup = NULL;
        return IN3_OK;
      }
      return IN3_EIGNORE;
    }
    case PLGN_ACT_ADD_PAYLOAD: {
#ifdef NODESELECT_DEF_WL
      sb_t* payload = plugin_ctx;
      if (data->whitelist) {
        const bytes_t adr = bytes(data->whitelist->contract, 20);
        sb_add_bytes(payload, ",\"whiteListContract\":", &adr, 1, false);
      }
#endif
      return IN3_OK;
    }
    default: break;
  }
  return IN3_EIGNORE;
}

in3_ret_t in3_register_nodeselect_def(in3_t* c) {
  in3_nodeselect_def_t* data = _calloc(1, sizeof(*data));
  data->avg_block_time       = avg_block_time_for_chain_id(c->chain.chain_id);
  data->nodelist_upd8_params = _calloc(1, sizeof(*(data->nodelist_upd8_params)));
  return plugin_register(c, PLGN_ACT_LIFECYCLE | PLGN_ACT_RPC_VERIFY | PLGN_ACT_NODELIST | PLGN_ACT_CONFIG | PLGN_ACT_CHAIN_CHANGE | PLGN_ACT_GET_DATA | PLGN_ACT_ADD_PAYLOAD, in3_nodeselect_def, data, false);
}
