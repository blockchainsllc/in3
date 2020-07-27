#include "nodeselect_def.h"
#include "../core/client/plugin.h"
#include "../core/util/bitset.h"
#include "../core/util/debug.h"

static void whitelist_free(in3_whitelist_t* wl) {
  if (!wl) return;
  if (wl->addresses.data) _free(wl->addresses.data);
  _free(wl);
}

static in3_ret_t nl_config_set(void* plugin_data, void* plugin_ctx) {
  char*                res   = NULL;
  in3_configure_ctx_t* ctx   = plugin_ctx;
  d_token_t*           token = ctx->token;
  in3_t*               c     = ctx->client;

  if (token->key == key("servers") || token->key == key("nodes")) {
    for (d_iterator_t ct = d_iter(token); ct.left; d_iter_next(&ct)) {
      chain_id_t   chain_id    = char_to_long(d_get_keystr(ct.token->key), -1);
      bytes_t*     wl_contract = d_get_byteskl(ct.token, key("whiteListContract"), 20);
      in3_chain_t* chain       = in3_find_chain(c, chain_id);

      EXPECT_CFG(chain != NULL, "invalid chain id!");
      // todo: use wl_contract

      // chain_props
      bool has_wlc = false, has_man_wl = false;
      for (d_iterator_t cp = d_iter(ct.token); cp.left; d_iter_next(&cp)) {
        if (cp.token->key == key("whiteListContract")) {
          EXPECT_TOK_ADDR(cp.token);
          EXPECT_CFG(!has_man_wl, "cannot specify manual whiteList and whiteListContract together!");
          has_wlc = true;
          whitelist_free(chain->whitelist);
          chain->whitelist               = _calloc(1, sizeof(in3_whitelist_t));
          chain->whitelist->needs_update = true;
          memcpy(chain->whitelist->contract, cp.token->data, 20);
        }
        else if (cp.token->key == key("whiteList")) {
          EXPECT_TOK_ARR(cp.token);
          EXPECT_CFG(!has_wlc, "cannot specify manual whiteList and whiteListContract together!");
          has_man_wl = true;
          int len = d_len(cp.token), i = 0;
          whitelist_free(chain->whitelist);
          chain->whitelist            = _calloc(1, sizeof(in3_whitelist_t));
          chain->whitelist->addresses = bytes(_calloc(1, len * 20), len * 20);
          for (d_iterator_t n = d_iter(cp.token); n.left; d_iter_next(&n), i += 20) {
            EXPECT_TOK_ADDR(n.token);
            const uint8_t* whitelist_address = d_bytes(n.token)->data;
            for (uint32_t j = 0; j < chain->whitelist->addresses.len; j += 20) {
              if (!memcmp(whitelist_address, chain->whitelist->addresses.data + j, 20)) {
                whitelist_free(chain->whitelist);
                chain->whitelist = NULL;
                EXPECT_TOK(cp.token, false, "duplicate address!");
              }
            }
            d_bytes_to(n.token, chain->whitelist->addresses.data + i, 20);
          }
        }
        else if (cp.token->key == key("needsUpdate")) {
          EXPECT_TOK_BOOL(cp.token);
          if (!d_int(cp.token)) {
            if (chain->nodelist_upd8_params) {
              _free(chain->nodelist_upd8_params);
              chain->nodelist_upd8_params = NULL;
            }
          }
          else if (!chain->nodelist_upd8_params)
            chain->nodelist_upd8_params = _calloc(1, sizeof(*(chain->nodelist_upd8_params)));
        }
        else if (cp.token->key == key("avgBlockTime")) {
          EXPECT_TOK_U16(cp.token);
          chain->avg_block_time = (uint16_t) d_int(cp.token);
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
            BIT_SET(chain->nodelist[i].attrs, ATTR_BOOT_NODE);
#endif
          }
        }
        else {
          EXPECT_TOK(cp.token, false, "unsupported config option!");
        }
      }
      in3_client_run_chain_whitelisting(chain);
    }
  }
cleanup:
  ctx->err = res;
  return IN3_OK;
}

static in3_ret_t nl_cache_set(void* plugin_data, void* plugin_ctx) {
  return IN3_OK;
}

static in3_ret_t nl_cache_get(void* plugin_data, void* plugin_ctx) {
  return IN3_OK;
}

static in3_ret_t nl_cache_clear(void* plugin_data, void* plugin_ctx) {
  return IN3_OK;
}

static in3_ret_t nl_pick_data(void* plugin_data, void* plugin_ctx) {
  return IN3_OK;
}

static in3_ret_t nl_pick_signer(void* plugin_data, void* plugin_ctx) {
  return IN3_OK;
}

static in3_ret_t nl_pick_followup(void* plugin_data, void* plugin_ctx) {
  return IN3_OK;
}

static in3_ret_t nodeselect(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  switch (action) {
    case PLGN_ACT_CONFIG_SET:
      return nl_config_set(plugin_data, plugin_ctx);
    case PLGN_ACT_CACHE_SET:
      return nl_cache_set(plugin_data, plugin_ctx);
    case PLGN_ACT_CACHE_GET:
      return nl_cache_get(plugin_data, plugin_ctx);
    case PLGN_ACT_CACHE_CLEAR:
      return nl_cache_clear(plugin_data, plugin_ctx);
    case PLGN_ACT_NL_PICK_DATA:
      return nl_pick_data(plugin_data, plugin_ctx);
    case PLGN_ACT_NL_PICK_SIGNER:
      return nl_pick_signer(plugin_data, plugin_ctx);
    case PLGN_ACT_NL_PICK_FOLLOWUP:
      return nl_pick_followup(plugin_data, plugin_ctx);
    default: break;
  }
  return IN3_EIGNORE;
}

in3_ret_t in3_register_nodeselect_def(in3_t* c) {
  return in3_plugin_register(c, PLGN_ACT_NODELIST | PLGN_ACT_CACHE | PLGN_ACT_CONFIG_SET, nodeselect, NULL, false);
}
