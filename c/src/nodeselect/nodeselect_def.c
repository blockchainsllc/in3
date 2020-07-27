#include "nodeselect_def.h"
#include "../core/client/plugin.h"
#include "../core/util/bitset.h"
#include "../core/util/debug.h"
#include "cache.h"

static in3_ret_t nl_config_set(in3_nodeselect_def_t* data, in3_configure_ctx_t* ctx) {
  char*      res   = NULL;
  d_token_t* token = ctx->token;
  in3_t*     c     = ctx->client;

  if (token->key == key("servers") || token->key == key("nodes")) {
    for (d_iterator_t ct = d_iter(token); ct.left; d_iter_next(&ct)) {
      chain_id_t chain_id    = char_to_long(d_get_keystr(ct.token->key), -1);
      bytes_t*   wl_contract = d_get_byteskl(ct.token, key("whiteListContract"), 20);

      if (wl_contract && wl_contract->len == 20) {
        data->whitelist                 = _malloc(sizeof(in3_whitelist_t));
        data->whitelist->addresses.data = NULL;
        data->whitelist->addresses.len  = 0;
        data->whitelist->needs_update   = true;
        data->whitelist->last_block     = 0;
        memcpy(data->whitelist->contract, wl_contract->data, 20);
      }

      // chain_props
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
          if (in3_client_clear_nodes(c, chain_id) < 0) goto cleanup;
          int i = 0;
          for (d_iterator_t n = d_iter(cp.token); n.left; d_iter_next(&n), i++) {
            EXPECT_CFG(d_get(n.token, key("url")) && d_get(n.token, key("address")), "expected URL & address");
            EXPECT_TOK_STR(d_get(n.token, key("url")));
            EXPECT_TOK_ADDR(d_get(n.token, key("address")));
            EXPECT_CFG(in3_client_add_node(c, chain_id, d_get_string(n.token, "url"),
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
cleanup:
  ctx->err = res;
  return IN3_OK;
}

static in3_ret_t nl_cache_clear(in3_nodeselect_def_t* data, in3_configure_ctx_t* ctx) {
  return IN3_OK;
}

static in3_ret_t nl_pick_data(in3_nodeselect_def_t* data, in3_configure_ctx_t* ctx) {
  return IN3_OK;
}

static in3_ret_t nl_pick_signer(in3_nodeselect_def_t* data, in3_configure_ctx_t* ctx) {
  return IN3_OK;
}

static in3_ret_t nl_pick_followup(in3_nodeselect_def_t* data, in3_configure_ctx_t* ctx) {
  return IN3_OK;
}

static in3_ret_t nodeselect(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  in3_nodeselect_def_t* data = plugin_data;
  in3_configure_ctx_t*  ctx  = plugin_ctx;
  switch (action) {
    case PLGN_ACT_INIT:
      return IN3_OK;
    case PLGN_ACT_TERM:
      in3_whitelist_clear(data->whitelist);
      in3_nodelist_clear(data);
      _free(data);
      return IN3_OK;
    case PLGN_ACT_CONFIG_SET:
      return nl_config_set(data, ctx);
    case PLGN_ACT_CACHE_SET:
      in3_cache_store_whitelist(ctx->client, data);
      return in3_cache_store_nodelist(ctx->client, data);
    case PLGN_ACT_CACHE_GET:
      in3_cache_update_whitelist(ctx->client, data);
      return in3_cache_update_nodelist(ctx->client, data);
    case PLGN_ACT_CACHE_CLEAR:
      return nl_cache_clear(data, ctx);
    case PLGN_ACT_NL_PICK_DATA:
      return nl_pick_data(data, ctx);
    case PLGN_ACT_NL_PICK_SIGNER:
      return nl_pick_signer(data, ctx);
    case PLGN_ACT_NL_PICK_FOLLOWUP:
      return nl_pick_followup(data, ctx);
    default: break;
  }
  return IN3_EIGNORE;
}

in3_ret_t in3_register_nodeselect_def(in3_t* c) {
  in3_nodeselect_def_t* data = _calloc(1, sizeof(*data));
  return in3_plugin_register(c, PLGN_ACT_LIFECYCLE | PLGN_ACT_NODELIST | PLGN_ACT_CACHE | PLGN_ACT_CONFIG_SET, nodeselect, data, false);
}
