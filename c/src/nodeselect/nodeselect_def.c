#include "nodeselect_def.h"
#include "../core/client/context_internal.h"
#include "../core/client/keys.h"
#include "../core/util/bitset.h"
#include "../core/util/debug.h"
#include "../core/util/log.h"
#include "cache.h"
#include "nodeselect_def_cfg.h"

#define BLACKLISTTIME (24 * 3600)
#define WAIT_TIME_CAP 3600

static uint16_t avg_block_time_for_chain_id(chain_id_t id) {
  switch (id) {
    case CHAIN_ID_MAINNET:
    case CHAIN_ID_GOERLI: return 15;
    case CHAIN_ID_KOVAN: return 6;
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

static in3_ret_t remove_node(in3_nodeselect_def_t* data, address_t address) {
  assert(data);
  assert(address);

  int node_index = -1;
  for (unsigned int i = 0; i < data->nodelist_length; i++) {
    if (memcmp(data->nodelist[i].address, address, 20) == 0) {
      node_index = i;
      break;
    }
  }
  if (node_index == -1) return IN3_EFIND;
  if (data->nodelist[node_index].url)
    _free(data->nodelist[node_index].url);

  if (node_index < ((signed) data->nodelist_length) - 1) {
    memmove(data->nodelist + node_index, data->nodelist + node_index + 1, sizeof(in3_node_t) * (data->nodelist_length - 1 - node_index));
    memmove(data->weights + node_index, data->weights + node_index + 1, sizeof(in3_node_weight_t) * (data->nodelist_length - 1 - node_index));
  }
  data->nodelist_length--;
  if (!data->nodelist_length) {
    _free(data->nodelist);
    _free(data->weights);
    data->nodelist = NULL;
    data->weights  = NULL;
  }
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

  if (token->key == key("servers") || token->key == key("nodes")) {
    for (d_iterator_t ct = d_iter(token); ct.left; d_iter_next(&ct)) {
      bytes_t* wl_contract = d_get_byteskl(ct.token, key("whiteListContract"), 20);

      if (wl_contract && wl_contract->len == 20) {
        data->whitelist                 = _malloc(sizeof(in3_whitelist_t));
        data->whitelist->addresses.data = NULL;
        data->whitelist->addresses.len  = 0;
        data->whitelist->needs_update   = true;
        data->whitelist->last_block     = 0;
        memcpy(data->whitelist->contract, wl_contract->data, 20);
      }

      bool has_wlc = false, has_man_wl = false;
      for (d_iterator_t cp = d_iter(ct.token); cp.left; d_iter_next(&cp)) {
        if (cp.token->key == key("whiteListContract")) {
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
        else {
          EXPECT_TOK(cp.token, false, "unsupported config option!");
        }
      }
      in3_client_run_chain_whitelisting(data);
    }
  }
  else if (token->key == key("rpc")) {
    in3_t* c = ctx->client;
    EXPECT_TOK_STR(token);
    c->proof          = PROOF_NONE;
    c->chain.chain_id = CHAIN_ID_LOCAL;
    c->request_count  = 1;
    in3_node_t* n     = &data->nodelist[0];
    if (n->url) _free(n->url);
    n->url = _malloc(d_len(token) + 1);
    strcpy(n->url, d_string(token));
    _free(data->nodelist_upd8_params);
    data->nodelist_upd8_params = NULL;
  }
cleanup:
  ctx->error_msg = res;
  return IN3_OK;
}

static in3_ret_t config_get(in3_nodeselect_def_t* data, in3_get_config_ctx_t* ctx) {
  sb_t*  sb = ctx->sb;
  in3_t* c  = ctx->client;

  if (c->chain.chain_id == CHAIN_ID_LOCAL)
    add_string(sb, ',', "rpc", data->nodelist->url);

  sb_add_chars(sb, ",\"nodes\":{");
  sb_add_char(sb, '"');
  sb_add_hexuint(sb, c->chain.chain_id);
  sb_add_chars(sb, "\":");
  add_hex(sb, '{', "contract", *c->chain.contract);

  if (data->whitelist)
    add_hex(sb, ',', "whiteListContract", bytes(data->whitelist->contract, 20));

  add_hex(sb, ',', "registryId", bytes(c->chain.registry_id, 32));
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

  sb_add_chars(sb, "}");
  return IN3_OK;
}

static in3_ret_t pick_data(in3_nodeselect_def_t* data, void* ctx_) {
  in3_ctx_t*        ctx    = ctx_;
  in3_node_filter_t filter = NODE_FILTER_INIT;
  filter.nodes             = d_get(d_get(ctx->requests[0], K_IN3), K_DATA_NODES);
  filter.props             = (ctx->client->node_props & 0xFFFFFFFF) | NODE_PROP_DATA | ((ctx->client->flags & FLAGS_HTTP) ? NODE_PROP_HTTP : 0) | (in3_ctx_get_proof(ctx, 0) != PROOF_NONE ? NODE_PROP_PROOF : 0);
  return in3_node_list_pick_nodes(ctx, data, &ctx->nodes, ctx->client->request_count, filter);
}

NONULL static bool auto_ask_sig(const in3_ctx_t* ctx) {
  return (ctx_is_method(ctx, "in3_nodeList") && !(ctx->client->flags & FLAGS_NODE_LIST_NO_SIG) && ctx->client->chain.chain_id != CHAIN_ID_BTC);
}

NONULL static in3_node_t* get_node(const in3_nodeselect_def_t* data, const node_match_t* node) {
  return node->index < data->nodelist_length ? data->nodelist + node->index : NULL;
}

NONULL static in3_node_weight_t* get_node_weight(const in3_nodeselect_def_t* data, const node_match_t* node) {
  return node->index < data->nodelist_length ? data->weights + node->index : NULL;
}

static in3_ret_t pick_signer(in3_nodeselect_def_t* data, void* ctx_) {
  in3_ctx_t*   ctx = ctx_;
  const in3_t* c   = ctx->client;

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
      n = get_node(&c->chain, w);
      if (n) memcpy(ctx->signers + i * 20, n->address, 20);
      w = w->next;
    }
    if (signer_nodes) in3_ctx_free_nodes(signer_nodes);
  }

  return IN3_OK;
}

static in3_ret_t pick_followup(in3_nodeselect_def_t* data, void* ctx) {
  return IN3_OK;
}

static inline bool is_blacklisted(const node_match_t* node) { return node && node->blocked; }

static in3_ret_t blacklist_node(in3_nodeselect_def_t* data, void* ctx) {
  node_match_t* node = ctx;

  if (is_blacklisted(node)) return IN3_ERPC; // already handled

  if (node && !node->blocked) {
    in3_node_weight_t* w = get_node_weight(data, node);
    if (!w) {
      in3_log_debug("failed to blacklist node: %s\n", get_node(data, node)->url);
      return IN3_EFIND;
    }

    // blacklist the node
    uint64_t blacklisted_until_ = in3_time(NULL) + BLACKLISTTIME;
    if (w->blacklisted_until != blacklisted_until_)
      data->dirty = true;
    w->blacklisted_until = blacklisted_until_;
    node->blocked        = true;
    in3_log_debug("Blacklisting node for unverifiable response: %s\n", get_node(data, node)->url);
  }
  return IN3_OK;
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

  if (data->whitelist && d_get_longk(response_in3, K_LAST_WHITE_LIST) > data->whitelist->last_block)
    data->whitelist->needs_update = true;
}

static in3_ret_t chain_change(in3_nodeselect_def_t* data, void* ctx) {
  in3_t*      c    = ((in3_chain_change_ctx_t*) ctx)->client;
  json_ctx_t* json = nodeselect_def_cfg(c->chain.chain_id);
  if (json == NULL)
    return IN3_ECONFIG;

  in3_configure_ctx_t cctx = {.client = c, .json = json, .token = json->result, .error_msg = NULL};
  in3_ret_t           ret  = config_set(data, &cctx);
  json_free(json);
  if (IN3_OK != ret) {
    in3_log_error("nodeselect config error: %s\n", cctx.error_msg);
    return IN3_ECONFIG;
  }

  return IN3_OK;
}

static in3_ret_t nodeselect(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  in3_nodeselect_def_t* data = plugin_data;
  switch (action) {
    case PLGN_ACT_INIT:
      data->avg_block_time = avg_block_time_for_chain_id(((in3_configure_ctx_t*) plugin_ctx)->client->chain.chain_id);
      return IN3_OK;
    case PLGN_ACT_TERM:
      in3_whitelist_clear(data->whitelist);
      in3_nodelist_clear(data);
      _free(data->nodelist_upd8_params);
      _free(data);
      return IN3_OK;
    case PLGN_ACT_CONFIG_SET:
      return config_set(data, (in3_configure_ctx_t*) plugin_ctx);
    case PLGN_ACT_CONFIG_GET:
      return config_get(data, (in3_get_config_ctx_t*) plugin_ctx);
    case PLGN_ACT_NL_PICK_DATA:
      return pick_data(data, plugin_ctx);
    case PLGN_ACT_NL_PICK_SIGNER:
      return pick_signer(data, plugin_ctx);
    case PLGN_ACT_NL_PICK_FOLLOWUP:
      return pick_followup(data, plugin_ctx);
    case PLGN_ACT_NL_BLACKLIST:
      return blacklist_node(data, plugin_ctx);
    case PLGN_ACT_CHAIN_CHANGE:
      return chain_change(data, plugin_ctx);
    default: break;
  }
  return IN3_EIGNORE;
}

in3_ret_t in3_register_nodeselect_def(in3_t* c) {
  in3_ret_t             ret  = IN3_OK;
  in3_nodeselect_def_t* data = _calloc(1, sizeof(*data));

  json_ctx_t* json = nodeselect_def_cfg(c->chain.chain_id);
  if (json == NULL) {
    ret = IN3_ECONFIG;
    goto FREE_DATA;
  }

  in3_configure_ctx_t cctx = {.client = c, .json = json, .token = json->result, .error_msg = NULL};
  ret                      = config_set(data, &cctx);
  if (IN3_OK != ret) {
    in3_log_error("nodeselect config error: %s\n", cctx.error_msg);
    goto FREE_JSON;
  }

  ret = plugin_register(c, PLGN_ACT_LIFECYCLE | PLGN_ACT_NODELIST | PLGN_ACT_CONFIG | PLGN_ACT_CHAIN_CHANGE, nodeselect, data, false);

FREE_JSON:
  json_free(json);

FREE_DATA:
  _free(data);
  return ret;
}
