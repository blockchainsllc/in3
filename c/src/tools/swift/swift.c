
#include "swift.h"

static in3_ret_t handle(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  swift_cb_t* conf = plugin_data;
  switch (action) {
    case PLGN_ACT_TERM:
      _free(plugin_data);
      return IN3_OK;
    case PLGN_ACT_CACHE_GET: {
      in3_cache_ctx_t* _Nonnull ctx = plugin_ctx;
      return conf->cache_get(ctx);
    }
    case PLGN_ACT_CACHE_SET: {
      in3_cache_ctx_t* _Nonnull ctx = plugin_ctx;
      return conf->cache_set(ctx);
    }
    case PLGN_ACT_CACHE_CLEAR:
      return conf->cache_clear();

    default:
      return IN3_ENOTSUP;
  }
  return IN3_EIGNORE;
}

in3_ret_t in3_register_swift(in3_t* c, swift_cb_t* cbs) {
  swift_cb_t* ptr = _malloc(sizeof(swift_cb_t));
  memcpy(ptr, cbs, sizeof(swift_cb_t));
  return in3_plugin_register(c, PLGN_ACT_CACHE_GET | PLGN_ACT_CACHE_SET | PLGN_ACT_CACHE_CLEAR | PLGN_ACT_TERM, handle, ptr, true);
}