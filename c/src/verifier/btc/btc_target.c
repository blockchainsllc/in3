#include "btc_target.h"
#include "../../core/client/cache.h"
#include "../../core/client/context_internal.h"
#include "../../core/client/keys.h"
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
static void set_cachekey(chain_id_t id, char* buffer) {
  sprintf(buffer, "btc_target_%d", (uint32_t) id);
}

// format:  <2 bytes big endias HEX DAP NR> <4 bytes bits>
// TODO add targets for all daps every 6 months
#define BTC_TARGETS "00fa8c577e17" \
                    "0138397a1117"

btc_target_conf_t* btc_get_conf(in3_t* c, in3_chain_t* chain) {
  btc_target_conf_t* tc = (btc_target_conf_t*) chain->conf;
  if (tc == NULL) {
    char cache_key[50];
    set_cachekey(c->chain_id, cache_key);

    tc              = _malloc(sizeof(btc_target_conf_t));
    chain->conf     = tc;
    tc->max_daps    = 20;
    tc->max_diff    = 5;
    tc->dap_limit   = 20;
    bytes_t* cached = c->cache ? c->cache->get_item(c->cache->cptr, cache_key) : NULL;

    if (cached) {
      tc->data = *cached;
      _free(cached);
    } else {
      const char*        btc_targets = BTC_TARGETS;
      const unsigned int len         = strlen(btc_targets);
      tc->data                       = bytes(_malloc(len >> 1), len >> 1);
      hex_to_bytes(btc_targets, len, tc->data.data, tc->data.len);
    }
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
  UNUSED_VAR(c);
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

static void mul_target(uint32_t percent, bytes32_t target) {
  uint8_t* p = target + 28;
  for (int i = 31; i >= 0; i--) {
    if (target[i]) {
      p = target + i - 3;
      break;
    }
  }
  uint32_t val = bytes_to_int(p, 4);
  val += (percent * val) / 100;
  int_to_bytes(val, p);
}

static uint8_t* get_difficulty(bytes_t header) {
  return btc_block_get(header, BTC_B_BITS).data;
}

in3_ret_t btc_check_target(in3_vctx_t* vc, uint32_t block_number, bytes32_t block_target, bytes_t final, bytes_t header) {

  // is there a required ctx, which we need to clean up?
  in3_ctx_t* ctx = ctx_find_required(vc->ctx, "in3_proofTarget");                                                  // do we have an existing required proofTarget-request?
  if (ctx)                                                                                                         // yes, we do!
    switch (in3_ctx_state(ctx)) {                                                                                  // but what is the state?
      case CTX_ERROR:                                                                                              // there was an error,
        return ctx_set_error(vc->ctx, "Error verifying the target", ctx_set_error(vc->ctx, ctx->error, IN3_ERPC)); // so we report it!
      case CTX_WAITING_FOR_REQUIRED_CTX:                                                                           // if we are waiting
      case CTX_WAITING_FOR_RESPONSE:                                                                               // for an response
        return IN3_WAITING;                                                                                        // we keep on waiting.
      case CTX_SUCCESS:                                                                                            // if it was successful,
        if (ctx_remove_required(vc->ctx, ctx)) return vc_err(vc, "could not clean up proofTarget-request!");       //  we remove it,
        break;                                                                                                     // since gthe verification already added the verified targets.
    }

  // let's see if we can find a verifiied target.
  bytes32_t verified_target;
  memset(verified_target, 0, 32);
  btc_target_conf_t* conf        = btc_get_config(vc);
  uint32_t           current_dap = btc_get_dap(block_number);
  uint32_t           found_dap   = btc_get_closest_target(vc, current_dap, verified_target);
  uint32_t           dist        = current_dap > found_dap ? current_dap - found_dap : found_dap - current_dap;

  if (!found_dap) return vc_err(vc, "could not find any verified target!");
  if (dist == 0) // found it, all is fine.
    return (memcmp(verified_target, block_target, 32) == 0) ? IN3_OK : vc_err(vc, "header target does not match the verified target");

  else if (dist <= conf->max_daps) {
    bytes32_t tmp;                                                                                   // the distance is within the allowed range
    memcpy(tmp, verified_target, 32);                                                                // so we take the verified target
    mul_target(conf->max_diff, tmp);                                                                 // and add the allowed percent
    if (memcmp(tmp, block_target, 32) > 0) {                                                         // and check if claimed target is below that limit, because lower means more work put it.
      btc_set_target(vc, current_dap, get_difficulty(header));                                       // ok, we can accept the current block target as the target for this dap.
      if (btc_get_dap(block_number + final.len / 80) == current_dap + 1)                             // the finality header crossed the dap-limit,
        btc_set_target(vc, current_dap + 1, get_difficulty(bytes(final.data + final.len - 80, 80))); // so we can also accept the new target from the finality headers
      return IN3_OK;                                                                                 // all is fine ...
    }
  }

  // we need more proof, so we create a request
  char* req = _malloc(300);
  sprintf(req, "{\"method\":\"in3_proofTarget\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":[\"%d,%d,%d,%d,%d\"]}", current_dap, found_dap, conf->max_diff, conf->max_daps, conf->dap_limit);
  return ctx_add_required(vc->ctx, ctx_new(vc->ctx->client, req));
}
