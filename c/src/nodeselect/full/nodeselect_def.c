#define __USE_GNU
#define _XOPEN_SOURCE   600
#define _POSIX_C_SOURCE 200809L

#include "nodeselect_def.h"
#include "../../core/client/keys.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/bitset.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "cache.h"
#include "nodeselect_def_cfg.h"
#include "registry.h"
#include <limits.h>

#define BLACKLISTTIME (24 * 3600)
#define WAIT_TIME_CAP 3600

// global nodelist-data for all nodelist instances without any special configurations
in3_nodeselect_def_t* nodelist_registry = NULL;

// define function to lock the registry while reading from or even changing the registry
#ifdef THREADSAFE
#if defined(_MSC_VER) || defined(__MINGW32__)
static HANDLE lock_registry = NULL;
static void   _lock_registry() {
  if (!lock_registry) {
    HANDLE p = CreateMutex(NULL, FALSE, NULL);
    if (InterlockedCompareExchangePointer((PVOID*) &lock_registry, (PVOID) p, NULL)) CloseHandle(p);
  }
  MUTEX_LOCK(lock_registry);
}
#define LOCK_REGISTRY(code)     \
  {                             \
    _lock_registry();           \
    code                        \
    MUTEX_UNLOCK(lock_registry) \
  }
#else
static pthread_mutex_t lock_registry = PTHREAD_MUTEX_INITIALIZER;
#define LOCK_REGISTRY(code)     \
  {                             \
    MUTEX_LOCK(lock_registry);  \
    code                        \
    MUTEX_UNLOCK(lock_registry) \
  }
#endif
#else
#define LOCK_REGISTRY(code) \
  { code }
#endif

static in3_ret_t rpc_verify(in3_nodeselect_def_t* data, in3_vctx_t* vc) {

  // do we support this request?
  if (!vc->req) return IN3_EUNKNOWN;
  if (vc->chain->type != CHAIN_ETH && strcmp(vc->method, "in3_nodeList")) return IN3_EIGNORE;
  if (in3_req_get_proof(vc->req, vc->index) == PROOF_NONE) return IN3_OK;

  // do we have a result? if not it is a valid error-response
  if (!vc->result) return IN3_OK;
  UNUSED_VAR(data); // no waring in case RPC_ONLY is used

#if !defined(RPC_ONLY) || defined(RPC_IN3_NODELIST)
  if (VERIFY_RPC("in3_nodeList")) {
    d_token_t* params = d_get(vc->request, K_PARAMS);
    return eth_verify_in3_nodelist(data, vc, d_get_int_at(params, 0), d_get_bytes_at(params, 1), d_get_at(params, 2));
  }
#endif

#ifdef NODESELECT_DEF_WL
#if !defined(RPC_ONLY) || defined(RPC_IN3_WHITELIST)
  if (strcmp(vc->method, "in3_whiteList") == 0)
    return eth_verify_in3_whitelist(data, vc);
#endif
#endif
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

static inline bool is_shared_nodelist(in3_nodeselect_def_t* n) {
  for (in3_nodeselect_def_t* t = nodelist_registry; t; t = t->next) {
    if (t == n) return true;
  }
  return false;
}

static in3_ret_t nodelist_seperate_from_registry(in3_nodeselect_def_t** src, in3_nodeselect_config_t* w) {
  LOCK_REGISTRY(
      if (w && is_shared_nodelist(*src)) {
        // create a custom nodelist
        (*src)->ref_counter--;                                                          // detach from old
        in3_nodeselect_def_t* data = _calloc(1, sizeof(*data));                         // create new empty list
        data->avg_block_time       = avg_block_time_for_chain_id((*src)->chain_id);     // with default blocktime
        data->nodelist_upd8_params = _calloc(1, sizeof(*(data->nodelist_upd8_params))); // but marked as needed to be updated
        data->chain_id             = (*src)->chain_id;                                  // clone the chain_id
        data->ref_counter          = 1;                                                 // this will never changed, since it is not a shared list
        w->chains[0]               = data;                                              // change the pointer in the wrapper
        *src                       = data;                                              // and in the calling function
#ifdef THREADSAFE
        MUTEX_INIT(data->mutex) // mutex is still needed to be threadsafe
#endif
      })
  return IN3_OK;
}

static in3_ret_t config_set(in3_nodeselect_def_t* data, in3_configure_ctx_t* ctx, in3_nodeselect_config_t* w) {
  char*       res   = NULL;
  json_ctx_t* json  = ctx->json;
  d_token_t*  token = ctx->token;

  if (token->key == CONFIG_KEY("preselect_nodes")) {
    if (data->pre_address_filter) b_free(data->pre_address_filter);
    if (d_type(token) == T_BYTES && d_len(token) % 20 == 0)
      data->pre_address_filter = b_dup(d_bytes(token));
    else if (d_type(token) == T_NULL)
      data->pre_address_filter = NULL;
    else {
      EXPECT_CFG(d_type(token) == T_NULL, "invalid preselect_nodes ");
    }
  }
  else if (token->key == CONFIG_KEY("replaceLatestBlock")) {
    EXPECT_TOK_U8(token);
    ctx->client->replace_latest_block = (uint8_t) d_int(token);
    const uint64_t dp_                = ctx->client->replace_latest_block;
    w->node_props                     = (w->node_props & 0xFFFFFFFF) | (dp_ << 32U);
  }
  else if (token->key == CONFIG_KEY("requestCount")) {
    EXPECT_TOK_U8(token);
    EXPECT_CFG(d_int(token), "requestCount must be at least 1");
    w->request_count = (uint8_t) d_int(token);
  }
  else if (token->key == CONFIG_KEY("minDeposit")) {
    EXPECT_TOK_U64(token);
    w->min_deposit = d_long(token);
  }
  else if (token->key == CONFIG_KEY("nodeProps")) {
    EXPECT_TOK_U64(token);
    w->node_props = d_long(token);
  }
  else if (token->key == CONFIG_KEY("nodeLimit")) {
    EXPECT_TOK_U16(token);
    w->node_limit = (uint16_t) d_int(token);
  }
  else if (token->key == CONFIG_KEY("nodeRegistry") || token->key == CONFIG_KEY("servers") || token->key == CONFIG_KEY("nodes")) {
    EXPECT_TOK_OBJ(token);

    // this is legacy-support, if the object has a key with the chain_id, we simply use the value.
    char node_id[10];
    sprintf(node_id, "0x%x", ctx->client->chain.id);
    if (d_get(token, key(node_id))) token = d_get(token, key(node_id));

    // this is changing the nodelist config, so we need to make sure we have our own nodelist
    TRY(nodelist_seperate_from_registry(&data, w))

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
      else if (cp.token->key == CONFIG_KEY("whiteListContract")) {
        EXPECT_TOK_ADDR(cp.token);
        EXPECT_CFG(!has_man_wl, "cannot specify manual whiteList and whiteListContract together!");
        has_wlc = true;
        in3_whitelist_clear(data->whitelist);
        data->whitelist               = _calloc(1, sizeof(in3_whitelist_t));
        data->whitelist->needs_update = true;
        memcpy(data->whitelist->contract, cp.token->data, 20);
      }
      else if (cp.token->key == CONFIG_KEY("whiteList")) {
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
      else if (cp.token->key == CONFIG_KEY("needsUpdate")) {
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
      else if (cp.token->key == CONFIG_KEY("avgBlockTime")) {
        EXPECT_TOK_U16(cp.token);
        data->avg_block_time = (uint16_t) d_int(cp.token);
      }
      else if (cp.token->key == CONFIG_KEY("nodeList")) {
        EXPECT_TOK_ARR(cp.token);
        if (clear_nodes(data) < 0) goto cleanup;
        for (d_iterator_t n = d_iter(cp.token); n.left; d_iter_next(&n)) {
          EXPECT_CFG(d_get(n.token, CONFIG_KEY("url")) && d_get(n.token, CONFIG_KEY("address")), "expected URL & address");
          EXPECT_TOK_STR(d_get(n.token, CONFIG_KEY("url")));
          EXPECT_TOK_ADDR(d_get(n.token, CONFIG_KEY("address")));
          EXPECT_CFG(add_node(data, d_get_string(n.token, CONFIG_KEY("url")),
                              d_get_longd(n.token, CONFIG_KEY("props"), 65535),
                              d_get_byteskl(n.token, CONFIG_KEY("address"), 20)->data) == IN3_OK,
                     "add node failed");
          assert(data->nodelist_length > 0);
          BIT_SET(data->nodelist[data->nodelist_length - 1].attrs, ATTR_BOOT_NODE);
        }
      }
    }
#ifdef NODESELECT_DEF_WL
    in3_client_run_chain_whitelisting(data);
#endif
    // for supported chains we will get the registryId & contract from def cfg lazily
    if (!nodeselect_def_cfg_data(ctx->client->chain.id).data)
      EXPECT_CFG(!memiszero(data->registry_id, 32) && !memiszero(data->contract, 20), "missing registryId/contract!");
  }
  else if (token->key == CONFIG_KEY("rpc")) {
    EXPECT_TOK_STR(token);
    TRY(nodelist_seperate_from_registry(&data, w))
    in3_t* c           = ctx->client;
    c->proof           = PROOF_NONE;
    c->signature_count = 0;
    c->chain.id        = CHAIN_ID_LOCAL;
    w->request_count   = 1;

    clear_nodes(data);
    _free(data->nodelist_upd8_params);
    data->nodelist_length++;
    data->nodelist             = _calloc(1, sizeof(in3_node_t));
    data->weights              = _calloc(1, sizeof(in3_node_weight_t));
    data->nodelist[0].props    = NODE_PROP_DATA;
    in3_node_t* n              = &data->nodelist[0];
    n->url                     = _strdupn(d_string(token), -1);
    data->nodelist_upd8_params = NULL;
  }
  else {
    return IN3_EIGNORE;
  }

cleanup:
  ctx->error_msg = res;
  return ctx->error_msg ? IN3_ECONFIG : IN3_OK;
}

static in3_ret_t config_get(in3_nodeselect_config_t* w, in3_get_config_ctx_t* ctx) {
  in3_nodeselect_def_t* data = w->chains[0];
  sb_t*                 sb   = ctx->sb;
  in3_t*                c    = ctx->client;

  if (c->chain.id == CHAIN_ID_LOCAL)
    add_string(sb, ',', "rpc", data->nodelist->url);
  add_uint(sb, ',', "requestCount", w->request_count);
  add_uint(sb, ',', "minDeposit", w->min_deposit);
  add_uint(sb, ',', "nodeProps", w->node_props);
  add_uint(sb, ',', "nodeLimit", w->node_limit);

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

static in3_ret_t init_boot_nodes(in3_nodeselect_def_t* data, in3_t* c, chain_id_t chain_id) {
  json_ctx_t* json = nodeselect_def_cfg(chain_id);
  if (json == NULL)
    return IN3_ECONFIG;

  in3_configure_ctx_t cctx = {.client = c, .json = json, .token = json->result + 1, .error_msg = NULL};
  in3_ret_t           ret  = config_set(data, &cctx, NULL);
  json_free(json);
  if (IN3_OK != ret) {
    in3_log_error("nodeselect config error: %s\n", cctx.error_msg);
    return IN3_ECONFIG;
  }

  for (unsigned int i = 0; i < data->nodelist_length; ++i)
    BIT_SET(data->nodelist[i].attrs, ATTR_BOOT_NODE);

  return in3_cache_init(c, data);
}

static node_match_t* parse_signers(d_token_t* dsigners) {
  node_match_t *signers = NULL, **s = &signers;
  if (dsigners && d_type(dsigners) == T_ARRAY) {
    unsigned int len = d_len(dsigners);
    for (unsigned int i = 0; i < len; i++) {
      d_token_t* dsigner = d_get_at(dsigners, i);
      if (d_type(dsigner) == T_BYTES && d_len(dsigner) == 20) {
        *s = _calloc(1, sizeof(node_match_t));
        memcpy((*s)->address, d_bytes(dsigner)->data, 20);
        (*s)->next = NULL;
        s          = &(*s)->next;
      }
    }
  }
  return signers;
}

static void free_signers(node_match_t* signers) {
  node_match_t* tmp = NULL;
  while (signers) {
    tmp = signers->next;
    _free(signers);
    signers = tmp;
  }
}

NONULL static in3_ret_t pick_data(in3_nodeselect_config_t* w, in3_nodeselect_def_t* data, in3_req_t* ctx) {
  // init cache lazily this also means we can be sure that all other related plugins are registered by now
  if (data->nodelist == NULL && IN3_ECONFIG == init_boot_nodes(data, ctx->client, in3_chain_id(ctx)))
    return IN3_ECONFIG;

  in3_node_filter_t filter = NODE_FILTER_INIT;
  filter.nodes             = d_get(d_get(ctx->requests[0], K_IN3), K_DATA_NODES);
  filter.props             = (w->node_props & 0xFFFFFFFF) | NODE_PROP_DATA | ((ctx->client->flags & FLAGS_HTTP) ? NODE_PROP_HTTP : 0) | (in3_req_get_proof(ctx, 0) != PROOF_NONE ? NODE_PROP_PROOF : 0);
  filter.exclusions        = parse_signers(d_get(d_get(ctx->requests[0], K_IN3), K_SIGNER_NODES)); // we must exclude any manually specified signer nodes

  // if incentive is active we should now

  // Send parallel requests if signatures have been requested
  int rc = w->request_count;
  if (ctx->client->signature_count && w->request_count <= 1)
    rc = 2;

  in3_ret_t ret = in3_node_list_pick_nodes(ctx, w, data, &ctx->nodes, rc, &filter);
  free_signers(filter.exclusions);
  return ret;
}

NONULL static bool auto_ask_sig(const in3_req_t* ctx) {
  return (req_is_method(ctx, "in3_nodeList") && !(ctx->client->flags & FLAGS_NODE_LIST_NO_SIG) && in3_chain_id(ctx) != CHAIN_ID_BTC);
}

NONULL static in3_ret_t pick_signer(in3_nodeselect_config_t* w, in3_nodeselect_def_t* data, in3_req_t* ctx) {
  const in3_t* c = ctx->client;

  if (in3_req_get_proof(ctx, 0) == PROOF_NONE && !auto_ask_sig(ctx))
    return IN3_OK;

  // For nodeList request, we always ask for proof & atleast one signature
  uint8_t total_sig_cnt = c->signature_count
                              ? c->signature_count
                              : (auto_ask_sig(ctx) ? 1 : 0);

  if (total_sig_cnt) {
    node_match_t*     signer_nodes = NULL;
    in3_node_filter_t filter       = NODE_FILTER_INIT;
    filter.nodes                   = d_get(d_get(ctx->requests[0], K_IN3), K_SIGNER_NODES);
    filter.props                   = w->node_props | NODE_PROP_SIGNER;
    filter.exclusions              = ctx->nodes;
    const in3_ret_t res            = in3_node_list_pick_nodes(ctx, w, data, &signer_nodes, total_sig_cnt, &filter);
    if (res < 0)
      return req_set_error(ctx, "Could not find any nodes for requesting signatures", res);
    if (ctx->signers) _free(ctx->signers);
    const int node_count  = req_nodes_len(signer_nodes);
    ctx->signers_length   = node_count;
    ctx->signers          = _malloc(20 * node_count); // 20 bytes per address
    const node_match_t* w = signer_nodes;
    in3_node_t*         n = NULL;
    for (int i = 0; i < node_count; i++) {
      n = get_node(data, w);
      if (n) memcpy(ctx->signers + i * 20, n->address, 20);
      w = w->next;
    }
    if (signer_nodes) in3_req_free_nodes(signer_nodes);
  }

  return IN3_OK;
}

NONULL in3_ret_t handle_failable(in3_nodeselect_def_t* data, in3_req_t* ctx) {
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
      res = req_set_error(ctx, ctx->required->error ? ctx->required->error : "error handling subrequest", IN3_ERPC);

    if (res == IN3_OK) res = req_remove_required(ctx, ctx->required, true);
  }

  return res;
}

static node_offline_t* offline_get(in3_nodeselect_def_t* data, const uint8_t* offline) {
  node_offline_t* n = data->offlines;
  while (n) {
    if (!memcmp(n->offline->address, offline, 20))
      break;
    n = n->next;
  }
  return n;
}

static void offline_add(in3_nodeselect_def_t* data, in3_node_t* offline, const uint8_t* reporter) {
  node_offline_t** n = &data->offlines;
  while (*n)
    (*n) = (*n)->next;
  *n            = _malloc(sizeof(node_offline_t));
  (*n)->offline = offline;
  memcpy((*n)->reporter, reporter, 20);
  (*n)->next = NULL;
}

static void offline_remove(in3_nodeselect_def_t* data, const uint8_t* address) {
  node_offline_t **curr = &data->offlines, *next = NULL;
  while (*curr != NULL) {
    next = (*curr)->next;
    if (!memcmp((*curr)->offline->address, address, 20))
      _free(*curr);
    *curr = next;
  }
}

static void offline_free(in3_nodeselect_def_t* data) {
  node_offline_t* tmp = NULL;
  while (data->offlines) {
    tmp = data->offlines->next;
    _free(data->offlines);
    data->offlines = tmp;
  }
}

NONULL in3_ret_t handle_offline(in3_nodeselect_def_t* data, in3_nl_offline_ctx_t* ctx) {
  const uint8_t blen = sizeof(ctx->missing) * CHAR_BIT;
  for (unsigned int pos = 0; pos != blen; pos++) {
    if (!BIT_CHECK(ctx->missing, pos))
      continue;

    const uint8_t* address = ctx->vctx->req->signers + (pos * 20);
    for (unsigned int i = 0; i < data->nodelist_length; ++i) {
      if (memcmp(data->nodelist[i].address, address, 20) != 0)
        continue;

      node_offline_t* n = offline_get(data, address);
      if (n) {
        // Only blacklist if reported by another node, ignore otherwise
        // This also guarantees there's only one entry per offline address.
        if (memcmp(n->reporter, ctx->vctx->node->address, 20) != 0) {
          blacklist_node_addr(data, address, BLACKLISTTIME);
          offline_remove(data, address);
        }
      }
      else {
        offline_add(data, &data->nodelist[i], ctx->vctx->node->address);
      }
      break;
    }
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

static void check_autoupdate(const in3_req_t* ctx, in3_nodeselect_def_t* data, d_token_t* response_in3, node_match_t* node) {
  assert_in3_req(ctx);
  assert(data);
  if ((ctx->client->flags & FLAGS_AUTO_UPDATE_LIST) == 0) return;

  if (d_get_long(response_in3, K_LAST_NODE_LIST) > d_get_long(response_in3, K_CURRENT_BLOCK)) {
    // this shouldn't be possible, so we ignore this lastNodeList and do NOT try to update the nodeList
    return;
  }

  if (d_get_long(response_in3, K_LAST_NODE_LIST) > data->last_block) {
    if (data->nodelist_upd8_params == NULL)
      data->nodelist_upd8_params = _malloc(sizeof(*(data->nodelist_upd8_params)));
    in3_node_t* n = get_node(data, node);
    if (n) {
      // overwrite old params since we have a newer nodelist update now
      memcpy(data->nodelist_upd8_params->node, n->address, 20);
      data->nodelist_upd8_params->exp_last_block = d_get_long(response_in3, K_LAST_NODE_LIST);
      data->nodelist_upd8_params->timestamp      = in3_time(NULL) + update_waittime(d_get_long(response_in3, K_LAST_NODE_LIST),
                                                                                    d_get_long(response_in3, K_CURRENT_BLOCK),
                                                                                    ctx->client->replace_latest_block,
                                                                                    data->avg_block_time);
    }
  }

#ifdef NODESELECT_DEF_WL
  if (data->whitelist && d_get_long(response_in3, K_LAST_WHITE_LIST) > data->whitelist->last_block)
    data->whitelist->needs_update = true;
#endif
}

static void handle_times(in3_nodeselect_def_t* data, node_match_t* node, in3_response_t* response) {
  in3_node_t* n = get_node(data, node);
  if (!node || (n && n->blocked) || !response || !response->time) return;
  in3_node_weight_t* w = get_node_weight(data, node);
  if (!w) return;
  w->response_count++;
  w->total_response_time += response->time;
  response->time = 0; // make sure we count the time only once
}

static in3_ret_t pick_followup(in3_nodeselect_def_t* data, in3_nl_followup_ctx_t* fctx) {
  in3_req_t*    ctx         = fctx->req;
  node_match_t* vnode       = fctx->node;
  node_match_t* node        = ctx->nodes;
  int           nodes_count = ctx->nodes == NULL ? 1 : req_nodes_len(ctx->nodes);

  // no node - nothing to do here.
  if (!node) return IN3_EIGNORE;

  for (int n = 0; n < nodes_count; n++, node = node ? node->next : NULL)
    handle_times(data, node, ctx->raw_response + n);

  // check auto update opts only if this node wasn't blacklisted (due to wrong result/proof)
  if (!is_blacklisted(get_node(data, node)) && ctx->responses && d_get(ctx->responses[0], K_IN3) && !d_get(ctx->responses[0], K_ERROR))
    check_autoupdate(ctx, data, d_get(ctx->responses[0], K_IN3), vnode);

  // update weights in the cache
  return in3_cache_store_nodelist(ctx->client, data);
}

static in3_ret_t chain_change(in3_nodeselect_def_t* data, in3_t* c) {
  data->avg_block_time = avg_block_time_for_chain_id(c->chain.id);
  return init_boot_nodes(data, c, c->chain.id);
}

static void free_(void* p) {
  _free(p);
}

static void nodelist_return_or_free(in3_nodeselect_def_t* chain) {
  bool needs_freeing = false;
  LOCK_REGISTRY(
      chain->ref_counter--;
      if (chain->ref_counter == 0) {
        needs_freeing = true;
        for (in3_nodeselect_def_t** p = &nodelist_registry; *p; p = &((*p)->next)) {
          if (*p == chain) {
            *p = chain->next;
            break;
          }
        }
      })
#ifdef THREADSAFE
  MUTEX_UNLOCK(chain->mutex) // Important: this function is always called in a context where we have locked this mutex, so we need to unlock it here!
#endif

  if (!needs_freeing) return;
  offline_free(chain);
  in3_nodelist_clear(chain);
#ifdef NODESELECT_DEF_WL
  in3_whitelist_clear(chain->whitelist);
#endif
#ifdef THREADSAFE
  MUTEX_FREE(chain->mutex)
#endif
  b_free(chain->pre_address_filter);
  _free(chain->nodelist_upd8_params);
  _free(chain);
}

// finds or creates a new nodelist
static in3_nodeselect_def_t* nodelist_get_or_create(chain_id_t chain_id) {
  in3_nodeselect_def_t* data = NULL;
  LOCK_REGISTRY(
      for (data = nodelist_registry; data; data = data->next) {
        if (data->chain_id == chain_id) {
          data->ref_counter++;
          break;
        }
      }

      if (data == NULL) {
        data                       = _calloc(1, sizeof(*data));
        data->avg_block_time       = avg_block_time_for_chain_id(chain_id);
        data->nodelist_upd8_params = _calloc(1, sizeof(*(data->nodelist_upd8_params)));
        data->chain_id             = chain_id;
        data->next                 = nodelist_registry;
        data->ref_counter          = 1;
        nodelist_registry          = data;
#ifdef THREADSAFE
        MUTEX_INIT(data->mutex)
#endif
      })
  return data;
}
#ifdef THREADSAFE
#define UNLOCK_AND_RETURN(val) \
  {                            \
    in3_ret_t r = val;         \
    MUTEX_UNLOCK(data->mutex); \
    return r;                  \
  }
#else
#define UNLOCK_AND_RETURN(val) return val;
#endif

static in3_req_t* get_req_from_plgn(void* plugin_ctx, in3_plugin_act_t action) {
  switch (action) {
    case PLGN_ACT_RPC_VERIFY: return ((in3_vctx_t*) plugin_ctx)->req;
    case PLGN_ACT_NL_PICK: return ((in3_nl_pick_ctx_t*) plugin_ctx)->req;
    case PLGN_ACT_NL_PICK_FOLLOWUP: return ((in3_nl_followup_ctx_t*) plugin_ctx)->req;
    case PLGN_ACT_NL_BLACKLIST: return ((in3_nl_blacklist_ctx_t*) plugin_ctx)->req;
    case PLGN_ACT_NL_FAILABLE: return (in3_req_t*) plugin_ctx;
    case PLGN_ACT_NL_OFFLINE: return ((in3_nl_offline_ctx_t*) plugin_ctx)->vctx->req;
    case PLGN_ACT_GET_DATA: return ((in3_get_data_ctx_t*) plugin_ctx)->req;
    case PLGN_ACT_ADD_PAYLOAD: return ((in3_pay_payload_ctx_t*) plugin_ctx)->req;
    default: return NULL;
  }
  return NULL;
}

in3_ret_t in3_nodeselect_handle_action(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  in3_nodeselect_config_t* w    = plugin_data;
  in3_nodeselect_def_t*    data = w->chains[0]; // we  always use the default and only change it if needed.
  in3_req_t*               r    = get_req_from_plgn(plugin_ctx, action);
  if (r && r->client->chain.id != in3_chain_id(r)) data = in3_get_nodelist_data(w, in3_chain_id(r));

#ifdef THREADSAFE
  // lock only the nodelist
  MUTEX_LOCK(data->mutex)
#endif

  switch (action) {
    case PLGN_ACT_INIT:
      UNLOCK_AND_RETURN(IN3_OK)
    case PLGN_ACT_TERM: {
      for (uint32_t i = 0; i < w->chains_len; i++) {
#ifdef THREADSAFE
        // lock only the nodelist if this is not the default, since the default is already locked
        if (i) MUTEX_LOCK(w->chains[i]->mutex)
#endif
        nodelist_return_or_free(w->chains[i]); // the unlocking of the mutex is done inside nodelist_return_or_free!
      }
      _free(w->chains);
      _free(plugin_data);
      return IN3_OK;
    }
    case PLGN_ACT_RPC_VERIFY:
      UNLOCK_AND_RETURN(rpc_verify(data, (in3_vctx_t*) plugin_ctx))
    case PLGN_ACT_CONFIG_SET:
      UNLOCK_AND_RETURN(config_set(data, (in3_configure_ctx_t*) plugin_ctx, plugin_data))
    case PLGN_ACT_CONFIG_GET:
      UNLOCK_AND_RETURN(config_get(w, (in3_get_config_ctx_t*) plugin_ctx))
    case PLGN_ACT_NL_PICK: {
      in3_nl_pick_ctx_t* pctx = plugin_ctx;
      UNLOCK_AND_RETURN(pctx->type == NL_DATA ? pick_data(w, data, pctx->req) : pick_signer(w, data, pctx->req))
    }
    case PLGN_ACT_NL_PICK_FOLLOWUP:
      UNLOCK_AND_RETURN(pick_followup(data, plugin_ctx))
    case PLGN_ACT_NL_BLACKLIST: {
      in3_nl_blacklist_ctx_t* bctx = plugin_ctx;
      UNLOCK_AND_RETURN(bctx->is_addr ? blacklist_node_addr(data, bctx->address, BLACKLISTTIME)
                                      : blacklist_node_url(data, bctx->url, BLACKLISTTIME))
    }
    case PLGN_ACT_NL_FAILABLE:
      UNLOCK_AND_RETURN(plugin_ctx ? handle_failable(data, plugin_ctx) : IN3_EUNKNOWN)
    case PLGN_ACT_NL_OFFLINE:
      UNLOCK_AND_RETURN(plugin_ctx ? handle_offline(data, plugin_ctx) : IN3_EUNKNOWN)
    case PLGN_ACT_CHAIN_CHANGE: {
      nodelist_return_or_free(data); // this will always unlock the mutex of the nodelist and update the ref_counter!
      in3_nodeselect_config_t* w = plugin_data;
      in3_t*                   c = plugin_ctx;
      w->chains[0]               = nodelist_get_or_create(c->chain.id);
      data                       = w->chains[0]; // update data-pointer to the new nodelist
#ifdef THREADSAFE
      MUTEX_LOCK(data->mutex) // and lock it because we are about to initialize the chain
#endif
      UNLOCK_AND_RETURN(data->nodelist ? IN3_OK : chain_change(w->chains[0], c))
    }
    case PLGN_ACT_GET_DATA: {
      in3_get_data_ctx_t* pctx = plugin_ctx;
      if (pctx->type == GET_DATA_REGISTRY_ID) {
        pctx->data    = data->registry_id;
        pctx->cleanup = NULL;
        UNLOCK_AND_RETURN(IN3_OK)
      }
      else if (pctx->type == GET_DATA_NODE_MIN_BLK_HEIGHT) {
        assert(pctx->data);

        uint8_t*     address = pctx->data;
        bool         found   = false;
        unsigned int i;
        for (i = 0; i < data->nodelist_length; i++) {
          if (memcmp(data->nodelist[i].address, address, 20) == 0) {
            found = true;
            break;
          }
        }

        if (pctx->cleanup) pctx->cleanup(pctx->data);
        if (found) {
          in3_node_t*   n    = &data->nodelist[i];
          uint32_t      mbh  = in3_node_props_get(n->props, NODE_PROP_MIN_BLOCK_HEIGHT);
          unsigned int* mbhp = _malloc(sizeof(mbh));
          *mbhp              = mbh;
          pctx->data         = mbhp;
          pctx->cleanup      = free_;
        }
        else {
          pctx->data    = NULL;
          pctx->cleanup = NULL;
        }
        UNLOCK_AND_RETURN(IN3_OK)
      }
      UNLOCK_AND_RETURN(IN3_EIGNORE)
    }
    case PLGN_ACT_ADD_PAYLOAD: {
#ifdef NODESELECT_DEF_WL
      in3_pay_payload_ctx_t* payload = plugin_ctx;
      if (data->whitelist) {
        const bytes_t adr = bytes(data->whitelist->contract, 20);
        sb_add_bytes(payload->sb, ",\"whiteListContract\":", &adr, 1, false);
      }
#endif
      UNLOCK_AND_RETURN(IN3_OK)
    }
    default: break;
  }
  UNLOCK_AND_RETURN(IN3_EIGNORE)
}

in3_ret_t in3_register_nodeselect_def(in3_t* c) {
  if (in3_plugin_is_registered(c, PLGN_ACT_LIFECYCLE | PLGN_ACT_RPC_VERIFY | PLGN_ACT_NODELIST | PLGN_ACT_CONFIG | PLGN_ACT_CHAIN_CHANGE | PLGN_ACT_GET_DATA | PLGN_ACT_ADD_PAYLOAD))
    return IN3_EIGNORE;
  in3_nodeselect_config_t* data = _malloc(sizeof(*data));
  data->request_count           = 1;
  data->min_deposit             = 0;
  data->node_limit              = 0;
  data->node_props              = 0;
  data->chains                  = _malloc(sizeof(in3_nodeselect_def_t*));
  data->chains[0]               = nodelist_get_or_create(c->chain.id);
  data->chains_len              = 1;
  return in3_plugin_register(c, PLGN_ACT_LIFECYCLE | PLGN_ACT_RPC_VERIFY | PLGN_ACT_NODELIST | PLGN_ACT_CONFIG | PLGN_ACT_CHAIN_CHANGE | PLGN_ACT_GET_DATA | PLGN_ACT_ADD_PAYLOAD, in3_nodeselect_handle_action, data, false);
}

in3_nodeselect_config_t* in3_get_nodelist(in3_t* c) {
  for (in3_plugin_t* p = c->plugins; p; p = p->next) {
    if (p->action_fn == in3_nodeselect_handle_action) return p->data;
  }
  return NULL;
}

in3_nodeselect_def_t* in3_get_nodelist_data(in3_nodeselect_config_t* conf, chain_id_t chain_id) {
  for (uint32_t i = 0; i < conf->chains_len; i++) {
    in3_nodeselect_def_t* def = conf->chains[i];
    if (chain_id == def->chain_id) return def;
  }

  // we need to create a new nodelist
  conf->chains                   = _realloc(conf->chains, (conf->chains_len + 1) * sizeof(in3_nodeselect_def_t*), conf->chains_len * sizeof(in3_nodeselect_def_t*));
  conf->chains[conf->chains_len] = nodelist_get_or_create(chain_id);
  conf->chains_len++;
  return conf->chains[conf->chains_len - 1];
}