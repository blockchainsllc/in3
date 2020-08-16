#include "btc_target.h"
#include "../../core/client/cache.h"
#include "../../core/client/context_internal.h"
#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
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
#define BTC_TARGETS "000affff001d" \
                    "000fffff001d" \
                    "00145746651c" \
                    "0019a7bc201c" \
                    "001e64ba0e1c" \
                    "00235a0c011c" \
                    "0028ed66471b" \
                    "002d56720e1b" \
                    "0032cb04041b" \
                    "0037cd2d011b" \
                    "003cfa98001b" \
                    "00418521131a" \
                    "0046864a091a" \
                    "004bcaf00d1a" \
                    "0050d7690d1a" \
                    "005587320b1a" \
                    "005a5f8b0a1a" \
                    "005fc93c081a" \
                    "0064087e051a" \
                    "006962fa041a" \
                    "006e5c98041a" \
                    "007394de011a" \
                    "007815de001a" \
                    "007d32875419" \
                    "0082cab01619" \
                    "008742120619" \
                    "008c2cf50119" \
                    "009199db0019" \
                    "009642286918" \
                    "009ba2ae3a18" \
                    "00a093b81f18" \
                    "00a5747b1b18" \
                    "00aa87bb1818" \
                    "00aff0171718" \
                    "00b48e411618" \
                    "00b9c14d1318" \
                    "00be89b21018" \
                    "00c31bb30918" \
                    "00c8c3a40618" \
                    "00cd36840518" \
                    "00d228720518" \
                    "00d7c4400418" \
                    "00dc858b0318" \
                    "00e1937e0218" \
                    "00e63e1b0218" \
                    "00eb308d0118" \
                    "00f00b310118" \
                    "00f54bce0018" \
                    "00fa8c577e17" \
                    "00ff494a5117" \
                    "0104495a4117" \
                    "01097b4f2f17" \
                    "010e91c12517" \
                    "0113f41e3717" \
                    "0118505b2e17" \
                    "011d38ff2917" \
                    "01229b0d1f17" \
                    "0127f5ab1717" \
                    "012c3eb21517" \
                    "0131ff321217" \
                    "0136bc201317"

in3_ret_t btc_check_conf(in3_t* c, btc_target_conf_t* conf) {
  // did the chain_id change?
  if (c->chain_id != conf->chain_id) {
    if (conf->data.data) _free(conf->data.data);
    conf->data     = bytes(NULL, 0);
    conf->chain_id = c->chain_id;
  }

  if (!conf->data.data) {
    char cache_key[50];
    set_cachekey(conf->chain_id, cache_key);
    in3_cache_ctx_t cctx = {.ctx = NULL, .content = NULL, .key = cache_key};
    in3_plugin_execute_all(c, PLGN_ACT_CACHE_GET, &cctx);

    if (cctx.content) {
      conf->data = *cctx.content;
      _free(cctx.content);
    }
    else {
      const char*        btc_targets = BTC_TARGETS;
      const unsigned int len         = strlen(btc_targets);
      conf->data                     = bytes(_malloc(len >> 1), len >> 1);
      hex_to_bytes(btc_targets, len, conf->data.data, conf->data.len);
    }
  }
  return IN3_OK;
}

void btc_set_target(btc_target_conf_t* tc, in3_vctx_t* vc, uint32_t dap, uint8_t* difficulty) {
  // add internally
  if (!tc->data.data)
    tc->data = bytes(_malloc(6), 6);
  else
    tc->data = bytes(_realloc(tc->data.data, tc->data.len + 6, tc->data.len), tc->data.len + 6);
  uint8_t* p = tc->data.data + tc->data.len - 6;
  p[0]       = dap >> 8 & 0xFF;
  p[1]       = dap & 0xFF;
  memcpy(p + 2, difficulty, 4);

  // add to cache
  char cache_key[50];
  set_cachekey(vc->chain->chain_id, cache_key);
  in3_cache_ctx_t cctx = {.ctx = NULL, .content = &tc->data, .key = cache_key};
  in3_plugin_execute_first_or_none(vc->ctx, PLGN_ACT_CACHE_SET, &cctx);
}

uint32_t btc_get_closest_target(btc_target_conf_t* tc, uint32_t dap, uint8_t* target) {
  bytes32_t tmp;
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

in3_ret_t btc_check_target(btc_target_conf_t* tc, in3_vctx_t* vc, uint32_t block_number, bytes32_t block_target, bytes_t final, bytes_t header) {

  // is there a required ctx, which we need to clean up?
  in3_ctx_t* ctx = ctx_find_required(vc->ctx, "in3_proofTarget");                                                  // do we have an existing required proofTarget-request?
  if (ctx)                                                                                                         // yes, we do!
    switch (in3_ctx_state(ctx)) {                                                                                  // but what is the state?
      case CTX_ERROR:                                                                                              // there was an error,
        return ctx_set_error(vc->ctx, "Error verifying the target", ctx_set_error(vc->ctx, ctx->error, IN3_ERPC)); // so we report it!
      case CTX_WAITING_FOR_RESPONSE:                                                                               // for an response
      case CTX_WAITING_TO_SEND:
        return IN3_WAITING;                                                                                         // we keep on waiting.
      case CTX_SUCCESS:                                                                                             // if it was successful,
        if (ctx_remove_required(vc->ctx, ctx, false)) return vc_err(vc, "could not clean up proofTarget-request!"); //  we remove it,
        break;                                                                                                      // since gthe verification already added the verified targets.
    }

  // let's see if we can find a verifiied target.
  bytes32_t verified_target;
  memset(verified_target, 0, 32);
  uint32_t current_dap = btc_get_dap(block_number);
  uint32_t found_dap   = btc_get_closest_target(tc, current_dap, verified_target);
  uint32_t dist        = current_dap > found_dap ? current_dap - found_dap : found_dap - current_dap;

  if (!found_dap) return vc_err(vc, "could not find any verified target!");
  if (dist == 0) // found it, all is fine.
    return (memcmp(verified_target, block_target, 32) == 0) ? IN3_OK : vc_err(vc, "header target does not match the verified target");

  else if (dist <= tc->max_daps) {
    bytes32_t tmp;                                                                                       // the distance is within the allowed range
    memcpy(tmp, verified_target, 32);                                                                    // so we take the verified target
    mul_target(tc->max_diff, tmp);                                                                       // and add the allowed percent
    if (memcmp(tmp, block_target, 32) > 0) {                                                             // and check if claimed target is below that limit, because lower means more work put it.
      btc_set_target(tc, vc, current_dap, get_difficulty(header));                                       // ok, we can accept the current block target as the target for this dap.
      if (btc_get_dap(block_number + final.len / 80) == current_dap + 1)                                 // the finality header crossed the dap-limit,
        btc_set_target(tc, vc, current_dap + 1, get_difficulty(bytes(final.data + final.len - 80, 80))); // so we can also accept the new target from the finality headers
      return IN3_OK;                                                                                     // all is fine ...
    }
  }

  // we need more proof, so we create a request
  char* req = _malloc(300);
  sprintf(req, "{\"method\":\"in3_proofTarget\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":[\"%d,%d,%d,%d,%d\"]}", current_dap, found_dap, (int) tc->max_diff, (int) tc->max_daps, (int) tc->dap_limit);
  return ctx_add_required(vc->ctx, ctx_new(vc->ctx->client, req));
}
