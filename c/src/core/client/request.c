/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
 *
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 *
 *
 * COMMERCIAL LICENSE USAGE
 *
 * Licensees holding a valid commercial license may use this file in accordance
 * with the commercial license agreement provided with the Software or, alternatively,
 * in accordance with the terms contained in a written agreement between you and
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further
 * information please contact slock.it at in3@slock.it.
 *
 * Alternatively, this file may be used under the AGPL license as follows:
 *
 * AGPL LICENSE USAGE
 *
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available
 * complete source code of licensed works and modifications, which include larger
 * works using a licensed work, under the same license. Copyright and license notices
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

#include "request.h"
#include "../util/debug.h"
#include "../util/log.h"
#include "client.h"
#include "keys.h"
#include "plugin.h"
#include "request_internal.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static in3_ret_t in3_plugin_init(in3_req_t* ctx) {
  if ((ctx->client->plugin_acts & PLGN_ACT_INIT) == 0) return IN3_OK;
  for (in3_plugin_t* p = ctx->client->plugins; p; p = p->next) {
    if (p->acts & PLGN_ACT_INIT) {
      TRY(p->action_fn(p->data, PLGN_ACT_INIT, ctx))
      p->acts &= ~((uint64_t) PLGN_ACT_INIT);
    }
  }
  ctx->client->plugin_acts &= ~((uint64_t) PLGN_ACT_INIT);
  return IN3_OK;
}

bool in_property_name(char* c) {
  for (c++; *c && *c != '"'; c++) {
    if (*c == '\\') c++;
  }
  if (*c) c++;
  while (*c && (*c == ' ' || *c < 14)) c++;
  return *c == ':';
}

in3_req_t* req_new_clone(in3_t* client, const char* req_data) {
  char*      data = _strdupn(req_data, -1);
  in3_req_t* r    = req_new(client, data);
  if (r)
    in3_cache_add_ptr(&r->cache, data);
  else
    _free(data);
  return r;
}

void in3_set_chain_id(in3_req_t* req, chain_id_t id) {
  if (!id || in3_chain_id(req) == id) return;

  cache_entry_t* entry = in3_cache_add_entry(&req->cache, NULL_BYTES, NULL_BYTES);
  entry->props         = CACHE_PROP_CHAIN_ID | CACHE_PROP_INHERIT;
  int_to_bytes((uint32_t) id, entry->buffer);
}

chain_id_t in3_chain_id(const in3_req_t* req) {
  for (cache_entry_t* entry = req->cache; entry; entry = entry->next) {
    if (entry->props & CACHE_PROP_CHAIN_ID)
      return (chain_id_t) bytes_to_int(entry->buffer, 4);
  }
  return req->client->chain.id;
}
in3_ret_t in3_resolve_chain_id(in3_req_t* req, chain_id_t* chain_id) {
  // make sure, we have the correct chain_id
  *chain_id = in3_chain_id(req);
  if (*chain_id == CHAIN_ID_LOCAL) {
    char* cachekey = stack_printx(50, "chain_id_%x", (uint64_t) req->client->chain.version);
    if (req->client->chain.id == CHAIN_ID_LOCAL) {
      if (req->client->chain.version && !(req->client->chain.version & 0x80000000)) {
        // we have a url-hash, which can lookup in the cache
        in3_cache_ctx_t cache = {.content = NULL, .key = cachekey, .req = req};
        in3_plugin_execute_first_or_none(req, PLGN_ACT_CACHE_GET, &cache);
        if (cache.content && cache.content->len == 8) {
          *chain_id                  = bytes_to_long(cache.content->data, 8);
          req->client->chain.version = ((uint32_t) *chain_id) | 0x80000000;
          return IN3_OK;
        }
      }
    }

    d_token_t* r = NULL;
    TRY(req_send_sub_request(req, "eth_chainId", "", NULL, &r, NULL))
    *chain_id = d_long(r);
    if (req->client->chain.id == CHAIN_ID_LOCAL) {
      uint8_t data[8];
      bytes_t b = bytes(data, 8);
      long_to_bytes(*chain_id, data);
      in3_cache_ctx_t cache = {.content = &b, .key = cachekey, .req = req};
      in3_plugin_execute_first_or_none(req, PLGN_ACT_CACHE_SET, &cache);
      req->client->chain.version = ((uint32_t) *chain_id) | 0x80000000;
    }
  }
  return IN3_OK;
}

in3_chain_t* in3_get_chain(in3_t* c, chain_id_t id) {
  in3_chain_t* chain = &c->chain;
  while (chain) {
    if (chain->id == id) return chain;
    if (chain->next)
      chain = chain->next;
    else
      break;
  }
  chain->next = _calloc(1, sizeof(in3_chain_t));
  chain       = chain->next;
  chain->id   = id;
  switch (id) {
    case CHAIN_ID_BTC:
      chain->type = CHAIN_BTC;
      break;
    case CHAIN_ID_IPFS:
      chain->type = CHAIN_IPFS;
      break;
    default:
      chain->type = CHAIN_ETH;
  }
  return chain;
}

d_token_t* req_get_response(in3_req_t* req, size_t index) {
  d_token_t* res = req->response ? req->response->result : NULL;
  switch (d_type(res)) {
    case T_OBJECT: return res;
    case T_ARRAY: return (req->in3_state || !req_is_method(req, "in3_http")) ? d_get_at(res, index) : res;
    default: return req->in3_state ? NULL : res;
  }
}

d_token_t* req_get_request(const in3_req_t* req, size_t index) {
  d_token_t* res = req->request ? req->request->result : NULL;
  switch (d_type(res)) {
    case T_OBJECT: return res;
    case T_ARRAY: return d_get_at(res, index);
    default: return NULL;
  }
}

in3_req_t* req_new(in3_t* client, const char* req_data) {
  assert_in3(client);
  assert(req_data);

  if (req_data == NULL || client->pending == 0xFFFF) return NULL; // avoid overflows by not creating any new ctx anymore

  in3_req_t* ctx          = _calloc(1, sizeof(in3_req_t));
  ctx->client             = client;
  ctx->verification_state = IN3_WAITING;
  ctx->request            = parse_json(req_data);
  client->pending++;

  // was the json parseable?
  if (!ctx->request) {
    in3_log_error("Invalid json-request: %s\n", req_data);
    req_set_error(ctx, "Error parsing the JSON-request!", IN3_EINVAL);
    char* msg = parse_json_error(req_data);
    if (msg) {
      req_set_error(ctx, msg, IN3_EINVAL);
      _free(msg);
    }
    return ctx;
  }

  // determine the length
  d_token_t* req = req_get_request(ctx, 0);
  if (d_type(ctx->request->result) == T_OBJECT)
    ctx->len = 1;
  else if (d_type(ctx->request->result) == T_ARRAY)
    ctx->len = d_len(ctx->request->result);
  else {
    req_set_error(ctx, "The Request is not a valid structure!", IN3_EINVAL);
    return ctx;
  }

  // do we have a defined id?
  d_token_t* t = d_get(req, K_ID);
  if (t == NULL) {
    ctx->id = client->id_count;
    client->id_count += ctx->len;
  }
  else if (d_type(t) == T_INTEGER)
    ctx->id = d_int(t);

  // is there a set chain?
  in3_set_chain_id(ctx, (chain_id_t) d_get_long(d_get(req, K_IN3), K_CHAIN_ID));

  // is it a valid request?
  if (d_type(d_get(req, K_METHOD)) != T_STRING) req_set_error(ctx, "No Method defined", IN3_ECONFIG);

  // if this is the first request, we initialize the plugins now
  in3_plugin_init(ctx);

  in3_log_debug("::: exec " COLOR_BRIGHT_BLUE "%s" COLOR_RESET COLOR_MAGENTA " %j " COLOR_RESET "\n", d_get_string(req, K_METHOD), d_get(req, K_PARAMS));

  return ctx;
}

char* req_get_error_data(in3_req_t* ctx) {
  return ctx ? ctx->error : "No request context";
}

char* req_get_result_json(in3_req_t* ctx, int index) {
  assert_in3_req(ctx);
  d_token_t* res = d_get(req_get_response(ctx, (size_t) index), K_RESULT);
  return res ? d_create_json(ctx->response, res) : NULL;
}

char* req_get_response_data(in3_req_t* ctx) {
  assert_in3_req(ctx);

  sb_t sb = {0};
  if (d_type(ctx->request->result) == T_ARRAY) sb_add_char(&sb, '[');
  for (uint_fast16_t i = 0; i < ctx->len; i++) {
    if (i) sb_add_char(&sb, ',');
    d_token_t*  response = req_get_response(ctx, i);
    str_range_t rr       = d_to_json(response);
    char*       start    = NULL;
    if (!rr.data) { // it's a binary response, so we can't return the json
      _free(sb.data);
      return NULL;
    }
    if ((ctx->client->flags & FLAGS_KEEP_IN3) == 0 && (start = d_to_json(d_get(response, K_IN3)).data) && start < rr.data + rr.len) {
      while (*start != ',' && start > rr.data) start--; // NOSONAR -  we are already checking that start is within the range
      sb_add_range(&sb, rr.data, 0, start - rr.data + 1);
      sb.data[sb.len - 1] = '}';
    }
    else
      sb_add_range(&sb, rr.data, 0, rr.len);
  }
  if (d_type(ctx->request->result) == T_ARRAY) sb_add_char(&sb, ']');
  return sb.data;
}

req_type_t req_get_type(in3_req_t* ctx) {
  assert_in3_req(ctx);
  return ctx->type;
}

in3_ret_t req_check_response_error(in3_req_t* c, int i) {
  assert_in3_req(c);

  d_token_t* r = d_get(req_get_response(c, (size_t) i), K_ERROR);
  if (!r)
    return IN3_OK;
  else if (d_type(r) == T_OBJECT) {
    str_range_t s   = d_to_json(r);
    char*       req = alloca(s.len + 1);
    strncpy(req, s.data, s.len);
    req[s.len] = '\0';
    return req_set_error(c, req, IN3_ERPC);
  }
  else
    return req_set_error(c, d_string(r), IN3_ERPC);
}

in3_ret_t req_set_error_intern(in3_req_t* ctx, char* message, in3_ret_t errnumber, const char* filename, const char* function, int line, ...) {
  assert(ctx);

  // if this is just waiting, it is not an error!
  if (errnumber == IN3_WAITING || errnumber == IN3_OK) return errnumber;
  if (message) {
    sb_t sb = {0};
    if (strchr(message, '%')) {
      va_list args;
      va_start(args, line);
      sb_vprintx(&sb, message, args);
      va_end(args);
      message = sb.data;
    }

    const size_t l   = strlen(message);
    char*        dst = NULL;
    if (ctx->error) {
      dst = _malloc(l + 2 + strlen(ctx->error));
      strcpy(dst, message);
      dst[l] = ':';
      strcpy(dst + l + 1, ctx->error);
      _free(ctx->error);
    }
    else {
      dst = _malloc(l + 1);
      strcpy(dst, message);
    }
    ctx->error = dst;
    _free(sb.data);

    error_log_ctx_t sctx = {.msg = message, .error = -errnumber, .req = ctx};
    in3_plugin_execute_first_or_none(ctx, PLGN_ACT_LOG_ERROR, &sctx);

    if (filename && function)
      in3_log_trace("Intermediate error -> %s in %s:%i %s()\n", message, filename, line, function);
    else
      in3_log_trace("Intermediate error -> %s\n", message);
  }
  else if (!ctx->error) {
    ctx->error    = _malloc(2);
    ctx->error[0] = 'E';
    ctx->error[1] = 0;
  }
  ctx->verification_state = errnumber;
  return errnumber;
}

in3_ret_t req_get_error(in3_req_t* ctx, int id) {
  if (ctx->error)
    return IN3_ERPC;
  else if (id >= (int) ctx->len)
    return IN3_EINVAL;
  d_token_t* response = req_get_response(ctx, (size_t) id);
  if (!response)
    return IN3_ERPCNRES;
  else if (NULL == d_get(response, K_RESULT) || d_get(response, K_ERROR))
    return IN3_EINVALDT;
  return IN3_OK;
}

void in3_req_free_nodes(node_match_t* node) {
  node_match_t* last_node = NULL;
  while (node) {
    last_node = node;
    node      = node->next;
    _free(last_node->url);
    _free(last_node);
  }
}

int req_nodes_len(node_match_t* node) {
  int all = 0;
  while (node) {
    all++;
    node = node->next;
  }
  return all;
}

bool req_is_method(const in3_req_t* ctx, const char* method) {
  const char* required_method = d_get_string(req_get_request(ctx, 0), K_METHOD);
  return (required_method && strcmp(required_method, method) == 0);
}

in3_proof_t in3_req_get_proof(in3_req_t* ctx, int i) {
  if (ctx->request) {
    char* verfification = d_get_string(d_get(req_get_request(ctx, (size_t) i), K_IN3), key("verification"));
    if (verfification && strcmp(verfification, "none") == 0) return PROOF_NONE;
    if (verfification && strcmp(verfification, "proof") == 0) return PROOF_STANDARD;
  }
  if (ctx->in3_state && ctx->in3_state->signers_length && !ctx->client->proof) return PROOF_STANDARD;
  return ctx->client->proof;
}

NONULL void in3_req_add_response(
    in3_http_request_t* req,      /**< [in]the the request */
    int                 index,    /**< [in] the index of the url, since this request could go out to many urls */
    int                 error,    /**< [in] if true this will be reported as error. the message should then be the error-message */
    const char*         data,     /**<  the data or the the string*/
    int                 data_len, /**<  the length of the data or the the string (use -1 if data is a null terminated string)*/
    uint32_t            time) {
  in3_ctx_add_response(req->req, index, error, data, data_len, time);
}

void in3_ctx_add_response(
    in3_req_t*  ctx,      /**< [in] the context */
    int         index,    /**< [in] the index of the url, since this request could go out to many urls */
    int         error,    /**< [in] if true this will be reported as error. the message should then be the error-message */
    const char* data,     /**<  the data or the the string*/
    int         data_len, /**<  the length of the data or the the string (use -1 if data is a null terminated string)*/
    uint32_t    time) {

  assert_in3_req(ctx);
  assert(data);
  if (error == 1) error = IN3_ERPC;

  if (!ctx->raw_response) {
    req_set_error(ctx, "no request created yet!", IN3_EINVAL);
    return;
  }
  in3_response_t* response = ctx->raw_response + index;
  response->time += time;
  if (response->state == IN3_OK && error) response->data.len = 0;
  response->state = error;
  if (data_len == -1)
    sb_add_chars(&response->data, data);
  else
    sb_add_range(&response->data, data, 0, data_len);
}

sb_t* in3_rpc_handle_start(in3_rpc_handle_ctx_t* hctx) {
  assert(hctx);
  assert_in3_req(hctx->req);
  assert(hctx->request);
  assert(hctx->response);

  *hctx->response = _calloc(1, sizeof(in3_response_t));
  sb_add_chars(&(*hctx->response)->data, "{\"id\":");
  sb_add_int(&(*hctx->response)->data, hctx->req->id);
  return sb_add_chars(&(*hctx->response)->data, ",\"jsonrpc\":\"2.0\",\"result\":");
}
in3_ret_t in3_rpc_handle_finish(in3_rpc_handle_ctx_t* hctx) {
  sb_add_char(&(*hctx->response)->data, '}');
  return IN3_OK;
}

in3_ret_t in3_rpc_handle_with_bytes(in3_rpc_handle_ctx_t* hctx, bytes_t data) {
  sb_add_bytes(in3_rpc_handle_start(hctx), NULL, &data, 1, false);
  return in3_rpc_handle_finish(hctx);
}

in3_ret_t in3_rpc_handle(in3_rpc_handle_ctx_t* hctx, char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  sb_vprintx(in3_rpc_handle_start(hctx), fmt, args);
  va_end(args);
  return in3_rpc_handle_finish(hctx);
}

in3_ret_t in3_rpc_handle_with_uint256(in3_rpc_handle_ctx_t* hctx, bytes_t data) {
  b_optimize_len(&data);
  sb_t* sb = in3_rpc_handle_start(hctx);
  sb_add_rawbytes(sb, "\"0x", data, -1);
  sb_add_char(sb, '"');
  return in3_rpc_handle_finish(hctx);
}

in3_ret_t in3_rpc_handle_with_string(in3_rpc_handle_ctx_t* hctx, char* data) {
  sb_add_chars(in3_rpc_handle_start(hctx), data);
  return in3_rpc_handle_finish(hctx);
}

in3_ret_t in3_rpc_handle_with_json(in3_rpc_handle_ctx_t* ctx, d_token_t* result) {
  if (!result) return req_set_error(ctx->req, "No result", IN3_ERPC);
  sb_t* sb = in3_rpc_handle_start(ctx);
  sb_add_json(sb, "", result);
  return in3_rpc_handle_finish(ctx);
}

in3_ret_t in3_rpc_handle_with_int(in3_rpc_handle_ctx_t* hctx, uint64_t value) {
  return in3_rpc_handle(hctx, "\"0x%x\"", value);
}

static d_token_t* find_req(in3_req_t* found, char* req) {
  for (; found; found = found->required) {
    for (cache_entry_t* e = found->cache; e; e = e->next) {
      if (e->props & CACHE_PROP_SRC_REQ && strcmp((char*) e->value.data, req) == 0) return found;
    }
  }
  return NULL;
}

static in3_ret_t req_send_sub_request_internal(in3_req_t* parent, char* method, char* params, char* in3, d_token_t** result, in3_req_t** child, bool use_cache, bool allow_error) {
  if (params == NULL) params = "";
  in3_req_t* ctx      = parent->required;
  size_t     req_len  = _strnlen(params, 100000) + _strnlen(method, 100) + 30 + (in3 ? 10 + _strnlen(in3, 100) : 0); // calculate the memory needed to store the json-request
  bool       use_heap = req_len > 1000;                                                                              // anything bigger than 1000 should not be stored on the stack
  char*      _in3     = in3 ? stack_printx(10 + _strnlen(in3, 100), ",\"in3\":%s", in3) : "";                        // the full optional in3-section should always fit into heap
  char*      req      = use_cache                                                                                    // if we are using the cache we need the exact request as ke, which we will compare to a cached value in order to find the right request
                            ? (use_heap
                                   ? sprintx("{\"method\":\"%s\",\"params\":[%s]%s}", method, params, _in3)
                                   : stack_printx(req_len, "{\"method\":\"%s\",\"params\":[%s]%s}", method, params, _in3))
                            : NULL;

  // find the existing ctx
  if (use_cache)
    ctx = find_req(ctx, req);
  else
    for (; ctx; ctx = ctx->required) {
      // we simply check if the method and params of the first request match
      if (strcmp(d_get_string(req_get_request(ctx, 0), K_METHOD), method)) continue;
      d_token_t* t = d_get(req_get_request(ctx, 0), K_PARAMS);
      if (!t) continue;
      str_range_t p = d_to_json(t);
      if (strncmp(params, p.data + 1, p.len - 2) == 0) break;
    }

  if (ctx) {
    if (child) *child = ctx;                        // if we found the child assign it
    if (req && use_heap) _free(req);                // we only cleanup if the req is in the heap
    d_token_t* response = req_get_response(ctx, 0); // we are only taking the first response ( for now )

    switch (in3_req_state(ctx)) {
      case REQ_ERROR:
        return req_set_error(parent, ctx->error, ctx->verification_state ? ctx->verification_state : IN3_ERPC);
      case REQ_SUCCESS:
        *result = strcmp(method, "in3_http") == 0 ? response : d_get(response, K_RESULT);
        if (!*result) {
          d_token_t* error = d_get(response, K_ERROR);
          if (error && allow_error) {
            *result = error;
            return IN3_OK;
          }

          char* s = d_get_string(error, K_MESSAGE);
          UNUSED_VAR(s); // this makes sure we don't get a warning when building with _DLOGGING=false
          return req_set_error(parent, s ? s : "error executing provider call", IN3_ERPC);
        }
        return IN3_OK;
      case REQ_WAITING_TO_SEND:
      case REQ_WAITING_FOR_RESPONSE:
        return IN3_WAITING;
    }
  }

  // create the request, which will always be in the heap and cleaned up when freeing the request
  req = use_cache
            ? (use_heap ? req : _strdupn(req, -1))
            : sprintx("{\"method\":\"%s\",\"params\":[%s]%s}", method, params, _in3);
  ctx = req_new(parent->client, req);
  if (!ctx) {
    req_set_error(parent, req, IN3_ERPC);
    _free(req);
    return req_set_error(parent, "Invalid request!", IN3_ERPC);
  }
  if (child) *child = ctx;

  // inherit cache-entries
  for (cache_entry_t* ce = parent->cache; ce; ce = ce->next) {
    if (ce->props & CACHE_PROP_INHERIT) {
      cache_entry_t* entry = in3_cache_add_entry(&ctx->cache, ce->key, ce->value);
      entry->props         = ce->props & (~CACHE_PROP_MUST_FREE);
      memcpy(entry->buffer, ce->buffer, 4);
    }
  }

  if (use_cache) in3_cache_add_ptr(&ctx->cache, _strdupn(req, -1))->props |= CACHE_PROP_SRC_REQ | CACHE_PROP_MUST_FREE;
  in3_ret_t ret = req_add_required(parent, ctx);

  // if the request is an internal handled request, we willhave a result already, so we need to update the *result.
  if (ret == IN3_OK && ctx->response) {
    d_token_t* response = req_get_response(ctx, 0);
    *result             = d_get(response, K_RESULT);
    if (!*result) {
      char* s = d_get_string(d_get(response, K_ERROR), K_MESSAGE);
      UNUSED_VAR(s); // this makes sure we don't get a warning when building with _DLOGGING=false
      ret = req_set_error(parent, s ? s : "error executing provider call", IN3_ERPC);
    }
  }
  return ret;
}

in3_ret_t req_send_sub_request(in3_req_t* parent, char* method, char* params, char* in3, d_token_t** result, in3_req_t** child) {
  bool use_cache = strcmp(method, "eth_sendTransaction") == 0; // this subrequest will be converted into eth_sendRawTransaction, so we must keep the original request.
  return req_send_sub_request_internal(parent, method, params, in3, result, child, use_cache, false);
}

in3_ret_t req_get_sub_request_error(in3_req_t* parent, char* method, char* params, char* in3, d_token_t** result, in3_req_t** child) {
  return req_send_sub_request_internal(parent, method, params, in3, result, child, false, true);
}

in3_ret_t req_send_id_sub_request(in3_req_t* parent, char* method, char* params, char* in3, d_token_t** result, in3_req_t** child) {
  return req_send_sub_request_internal(parent, method, params, in3, result, child, true, false);
}

static inline const char* method_for_sigtype(d_digest_type_t type) {
  switch (type) {
    case SIGN_EC_RAW: return "sign_ec_raw";
    case SIGN_EC_HASH: return "sign_ec_hash";
    case SIGN_EC_PREFIX: return "sign_ec_prefix";
    case SIGN_EC_BTC: return "sign_ec_btc";
    default: return "sign_ec_hash";
  }
}

in3_ret_t req_send_sign_request(in3_req_t* ctx, d_digest_type_t type, in3_curve_type_t curve_type, d_payload_type_t pl_type, bytes_t* signature, bytes_t raw_data, bytes_t from, d_token_t* meta, bytes_t cache_key) {

  bytes_t* cached_sig = in3_cache_get_entry(ctx->cache, &cache_key);
  if (cached_sig) {
    *signature = *cached_sig;
    return IN3_OK;
  }

  // get the signature from required
  char*       params = sprintx("[\"%B\",\"%B\",%u,%u,%j]", raw_data, from, (uint32_t) pl_type, (uint32_t) curve_type, meta);
  const char* method = method_for_sigtype(type);
  in3_req_t*  c      = req_find_required(ctx, method, params);
  if (c) {
    _free(params);
    switch (in3_req_state(c)) {
      case REQ_ERROR:
        return req_set_error(ctx, c->error ? c->error : "Could not handle signing", IN3_ERPC);
      case REQ_WAITING_FOR_RESPONSE:
      case REQ_WAITING_TO_SEND:
        return IN3_WAITING;
      case REQ_SUCCESS: {
        if (c->raw_response && c->raw_response->state == IN3_OK && c->raw_response->data.len > 64) {
          *signature = cloned_bytes(bytes((uint8_t*) c->raw_response->data.data, c->raw_response->data.len));
          in3_cache_add_entry(&ctx->cache, cloned_bytes(cache_key), *signature);
          req_remove_required(ctx, c, false);
          return IN3_OK;
        }
        else if (c->raw_response && c->raw_response->state)
          return req_set_error(ctx, c->raw_response->data.data, c->raw_response->state);
        else
          return req_set_error(ctx, "no data to sign", IN3_EINVAL);
        default:
          return req_set_error(ctx, "invalid state", IN3_EINVAL);
      }
    }
  }
  else {
    char* req = sprintx("{\"method\":\"%s\",\"params\":%s}", method, params);
    _free(params);
    c = req_new(ctx->client, req);
    if (!c) {
      req_set_error(ctx, req, IN3_EINVAL);
      _free(req);
      return req_set_error(ctx, "Invalid sign request", IN3_EINVAL);
    }
    c->type = RT_SIGN;
    return req_add_required(ctx, c);
  }
}

in3_ret_t req_require_signature(in3_req_t* ctx, d_digest_type_t digest_type, in3_curve_type_t curve_type, d_payload_type_t pl_type, bytes_t* signature, bytes_t raw_data, bytes_t from, d_token_t* meta, sb_t* tx_output) {
  bytes_t cache_key = bytes(alloca(raw_data.len + from.len), raw_data.len + from.len);
  memcpy(cache_key.data, raw_data.data, raw_data.len);
  if (from.data) memcpy(cache_key.data + raw_data.len, from.data, from.len);
  bytes_t* cached_sig = in3_cache_get_entry(ctx->cache, &cache_key);
  if (cached_sig) {
    *signature = *cached_sig;
    return IN3_OK;
  }

  in3_log_debug("requesting signature type=%d from account %B for %B\n", digest_type, from, raw_data);

  // first try internal plugins for signing, before we create an context.
  if (in3_plugin_is_registered(ctx->client, PLGN_ACT_SIGN)) {
    in3_sign_ctx_t sc = {.account = from, .req = ctx, .message = raw_data, .signature = NULL_BYTES, .digest_type = digest_type, .payload_type = pl_type, .meta = meta, .curve_type = curve_type, .tx_output = tx_output};
    in3_ret_t      r  = in3_plugin_execute_first_or_none(ctx, PLGN_ACT_SIGN, &sc);
    if (r == IN3_OK && sc.signature.data) {
      in3_cache_add_entry(&ctx->cache, cloned_bytes(cache_key), sc.signature);
      *signature = sc.signature;
      return IN3_OK;
    }
    else if (r != IN3_EIGNORE && r != IN3_OK)
      return r;
  }
  in3_log_debug("nobody picked up the signature, sending req now \n");
  return req_send_sign_request(ctx, digest_type, curve_type, pl_type, signature, raw_data, from, meta, cache_key);
}

in3_ret_t send_http_request(in3_req_t* req, char* url, char* method, char* path, char* payload,
                            in3_http_header_t* headers, d_token_t** result, in3_req_t** child, uint32_t wait_in_ms) {
  sb_t rp = {0};
  // build payload
  sb_printx(&rp, "{\"method\":\"in3_http\",\"params\":[\"%s\",\"%S%S\",%s,[", method, url, path ? path : "", payload ? payload : "null");
  for (in3_http_header_t* h = headers; h; h = h->next) sb_add_value(&rp, "\"%S: %S\"", h->key, h->value);

  sb_add_chars(&rp, "]]");
  if (wait_in_ms > 0) sb_printx(&rp, ", \"in3\": {\"wait\":%u}", wait_in_ms);
  sb_add_chars(&rp, "}");

  // look for the subrequest
  in3_req_t* found = find_req(req, rp.data);
  if (found) {
    if (child) *child = found;
    _free(rp.data);
    switch (in3_req_state(found)) {
      case REQ_ERROR:
        return req_set_error(req, found->error, found->verification_state ? found->verification_state : IN3_ERPC);
      case REQ_SUCCESS:
        *result = found->response->result;
        return *result ? IN3_OK : req_set_error(req, "error executing provider call", IN3_ERPC);
      case REQ_WAITING_TO_SEND:
      case REQ_WAITING_FOR_RESPONSE:
        return IN3_WAITING;
    }
  }

  in3_log_debug("http-request: %s\n", rp.data);
  found = req_new(req->client, rp.data);
  if (!found) {
    _free(rp.data);
    return req_set_error(req, "Invalid request!", IN3_ERPC);
  }
  if (child) *child = found;

  in3_cache_add_ptr(&found->cache, rp.data)->props = CACHE_PROP_SRC_REQ;

  return req_add_required(req, found);
}

in3_ret_t req_require_pub_key(in3_req_t* ctx, in3_curve_type_t curve_type, in3_convert_type_t convert_type, bytes_t from, uint8_t dst[64]) {
  if (!dst) return req_set_error(ctx, "dst buffer cannot be null", IN3_EINVAL);
  in3_sign_public_key_ctx_t sc = {.account = from.data, .req = ctx, .curve_type = curve_type, .convert_type = convert_type};
  in3_ret_t                 r  = in3_plugin_execute_first_or_none(ctx, PLGN_ACT_SIGN_PUBLICKEY, &sc);
  if (r != IN3_WAITING && r != IN3_OK) return req_set_error(ctx, "Signer not found", r);
  memcpy(dst, sc.public_key, 64);
  return r;
}

in3_ret_t req_throw_unknown_prop(in3_req_t* r, d_token_t* ob, d_token_t* prop, char* ob_name) {
  char* missing = d_get_property_name(ob, d_get_key(prop));
  char* m       = missing ? sprintx("The property '%s' does not exist in %s", missing, ob_name) : sprintx("Unknown property for the the type %s", ob_name);
  req_set_error(r, m, IN3_EINVAL);
  _free(missing);
  _free(m);
  return IN3_EINVAL;
}
