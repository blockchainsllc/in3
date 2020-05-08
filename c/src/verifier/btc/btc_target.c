#include "btc_target.h"
#include "../../core/client/cache.h"
#include "../../core/util/mem.h"
#include "btc_serialize.h"

in3_ret_t btc_new_target_check(in3_vctx_t* vc, bytes32_t old_target, bytes32_t new_target) {
  bytes32_t tmp;
  memcpy(tmp, old_target, 32);
  for (int i = 0; i < 31; i++) tmp[i] = (tmp[i] << 2) | (tmp[i + 1] >> 6); // multiply by 4
  if (memcmp(tmp, new_target, 32) < 0) return vc_err(vc, "new target is more than 4 times the old target");
  memcpy(tmp, old_target, 32);
  for (int i = 1; i < 32; i++) tmp[i] = (tmp[i] >> 2) | (tmp[i - 1] << 6); // divide by 4
  if (memcmp(tmp, new_target, 32) > 0) return vc_err(vc, "new target is less than one 4th of the old target");
  return IN3_OK;
}

// format:  <2 bytes big endias HEX DAP NR> <4 bytes bits>
#define BTC_TARGETS "0135413b1417" \
                    "0138397a1117"

btc_target_conf_t* btc_get_conf(in3_t* c, in3_chain_t* chain) {
  btc_target_conf_t* tc = (btc_target_conf_t*) chain->conf;
  if (tc == NULL) {
    tc                             = _malloc(sizeof(btc_target_conf_t));
    chain->conf                    = tc;
    tc->max_daps                   = 5;
    tc->max_diff                   = 5;
    const char*        btc_targets = BTC_TARGETS;
    const unsigned int len         = strlen(btc_targets);
    tc->data                       = bytes(_malloc(len >> 1), len >> 1);
    hex_to_bytes(btc_targets, len, tc->data.data, tc->data.len);
  }
  return tc;
}

in3_ret_t btc_vc_set_config(in3_t* c, d_token_t* conf, in3_chain_t* chain) {
  btc_target_conf_t* tc = btc_get_conf(c, chain);
  if (!tc) return IN3_ENOMEM;
  if (conf->key == key("maxDAP"))
    tc->max_daps = d_int(conf);
  else if (conf->key == key("maxDiff"))
    tc->max_diff = d_int(conf);
  else
    return IN3_EINVAL;
  return IN3_OK;
}

void btc_vc_free(in3_t* c, in3_chain_t* chain) {
  if (chain->conf) {
    btc_target_conf_t* tc = (btc_target_conf_t*) chain->conf;
    if (tc->data.data) _free(tc->data.data);
    _free(tc);
    chain->conf = NULL;
  }
}

btc_target_conf_t* btc_get_config(in3_vctx_t* vc) {
  return btc_get_conf(vc->ctx->client, vc->chain);
}

static void set_cachekey(chain_id_t id, char* buffer) {
  sprintf(buffer, "btc_target_%d", (uint32_t) id);
}

void btc_set_target(in3_vctx_t* vc, uint32_t dap, uint8_t* difficulty) {
  // add internally
  btc_target_conf_t* tc = btc_get_config(vc);
  if (!tc->data.data)
    tc->data = bytes(_malloc(6), 6);
  else
    tc->data = bytes(_realloc(tc->data.data, tc->data.len + 6, tc->data.len), tc->data.len + 6);
  uint8_t* p = tc->data.data + tc->data.len - 6;
  p[0]       = dap >> 8 & 0xFF;
  p[1]       = dap & 0xFF;
  memcpy(p + 2, difficulty, 4);

  // add to cache
  if (vc->ctx->client->cache) {
    char cache_key[50];
    set_cachekey(vc->chain->chain_id, cache_key);
    vc->ctx->client->cache->set_item(vc->ctx->client->cache->cptr, cache_key, &tc->data);
  }
}

uint32_t btc_get_closest_target(in3_vctx_t* vc, uint32_t dap, uint8_t* target) {
  btc_target_conf_t* tc = btc_get_config(vc);
  bytes32_t          tmp;
  if (!tc->data.len) return 0;

  uint32_t found = 0, dist = 0xFFFFFFFF;
  uint8_t* end = tc->data.len + tc->data.data;

  for (uint8_t* p = tc->data.data; p != end; p += 6) {
    uint32_t c = ((uint32_t) p[0]) << 8 | p[1];
    uint32_t d = c < dap ? dap - c : c - dap;
    if (d < dist) {
      memset(tmp, 0, 32);
      memcpy(tmp + p[5] - 3, p + 2, 3);
      rev_copy(target, tmp);
      found = c;
      dist  = d;
      if (d == 0) return c;
    }
  }

  return found;
}

in3_ret_t btc_get_verified_target(in3_vctx_t* vc, uint32_t block_number, uint8_t* target) {
  bytes32_t verified_target;
  memset(verified_target, 0, 32);
  btc_target_conf_t* conf        = btc_get_config(vc);
  uint32_t           current_dap = btc_get_dap(block_number);
  uint32_t           found_dap   = btc_get_closest_target(vc, current_dap, verified_target);
  uint32_t           dist        = current_dap > found_dap ? current_dap - found_dap : found_dap - current_dap;

  if (!found_dap) return vc_err(vc, "could not find any verified target!");
  if (dist == 0) {
    memcpy(target, verified_target, 32);
    return IN3_OK;
  }

  if (conf->max_daps <= dist) {
    // the distance is within the allowed range
    }
  return IN3_EFIND;
}