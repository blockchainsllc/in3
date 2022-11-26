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
#define IN3_INTERNAL

#include "../util/crypto.h"
#include "../util/data.h"
#include "../util/log.h"
#include "client.h"
#include "keys.h"
#include "plugin.h"
#include "request_internal.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

NONULL static bool is_raw_http(in3_req_t* ctx) {
  return !ctx->in3_state && strcmp("in3_http", d_get_string(ctx->requests[0], K_METHOD)) == 0;
}

NONULL static void response_free(in3_req_t* ctx) {
  assert_in3_req(ctx);

  int nodes_count = 1;
  if (ctx->in3_state) {
    if (ctx->in3_state->nodes) {
      nodes_count = req_nodes_len(ctx->in3_state->nodes);
      in3_req_free_nodes(ctx->in3_state->nodes);
    }
    _free(ctx->in3_state->signers);
    _free(ctx->in3_state);
  }
  if (ctx->raw_response) {
    for (int i = 0; i < nodes_count; i++) {
      if (ctx->raw_response[i].data.data) _free(ctx->raw_response[i].data.data);
    }
    _free(ctx->raw_response);
  }

  if (ctx->responses) _free(ctx->responses);
  if (ctx->response_context) json_free(ctx->response_context);
  ctx->response_context = NULL;
  ctx->responses        = NULL;
  ctx->raw_response     = NULL;
  ctx->raw_response     = NULL;
  ctx->in3_state        = NULL;
}

NONULL void in3_check_verified_hashes(in3_t* c) {
  // shrink verified hashes to max_verified_hashes
  if (c->pending <= 1 && c->alloc_verified_hashes > c->max_verified_hashes) {
    // we want to keep the newest entries, so we move them overriding the oldest
    memmove(c->chain.verified_hashes,
            c->chain.verified_hashes + (c->alloc_verified_hashes - c->max_verified_hashes),
            sizeof(in3_verified_hash_t) * c->max_verified_hashes);
    c->chain.verified_hashes = _realloc(c->chain.verified_hashes,
                                        c->max_verified_hashes * sizeof(in3_verified_hash_t),
                                        c->alloc_verified_hashes * sizeof(in3_verified_hash_t));
    c->alloc_verified_hashes = c->max_verified_hashes;
  }
}

NONULL static void req_free_intern(in3_req_t* ctx, bool is_sub) {
  assert_in3_req(ctx);
  // only for intern requests, we actually free the original request-string
  if (is_sub && ctx->request_context)
    _free(ctx->request_context->c);
  ctx->client->pending--;
  if (ctx->error) _free(ctx->error);
  response_free(ctx);
  if (ctx->request_context)
    json_free(ctx->request_context);

  if (ctx->requests) _free(ctx->requests);
  if (ctx->cache) in3_cache_free(ctx->cache, !is_sub);
  if (ctx->required) req_free_intern(ctx->required, true);

  in3_check_verified_hashes(ctx->client);
  _free(ctx);
}

static void free_urls(char** urls, int len) {
  if (!urls) return;
  for (int i = 0; i < len; i++)
    _free(urls[i]);
  _free(urls);
}

static int add_bytes_to_hash(in3_digest_t msg_hash, void* data, int len) {
  assert(data);
  if (msg_hash.ctx) crypto_update_hash(msg_hash, bytes(data, len));
  return len;
}

NONULL static void add_token_to_hash(in3_digest_t msg_hash, d_token_t* t) {
  switch (d_type(t)) {
    case T_ARRAY:
    case T_OBJECT:
      for_children_of(iter, t) add_token_to_hash(msg_hash, iter.token);
      return;
    case T_NULL:
      return;

    default: {
      bytes_t b = d_bytes(t);
      crypto_update_hash(msg_hash, bytes(b.data, b.len));
    }
  }
}

static void add_id(in3_req_t* c, sb_t* sb, in3_digest_t msg_hash, d_token_t* request_token) {
  char*      temp = NULL;
  d_token_t* t    = d_get(request_token, K_ID);
  if (t == NULL)
    temp = stack_printx(40, "%u", c->id + 1);
  else if (d_type(t) == T_INTEGER)
    temp = stack_printx(40, "%u", (uint32_t) d_int(t));
  else
    temp = d_string(t);

  sb_add_chars(sb, "{\"id\":\"");
  sb_add_range(sb, temp, 0, add_bytes_to_hash(msg_hash, temp, _strnlen(temp, 40)));
  sb_add_char(sb, '\"');
}

NONULL static in3_ret_t ctx_create_payload(in3_req_t* c, sb_t* sb, bool no_in3) {
  assert_in3_req(c);
  assert(sb);

  in3_t* rc           = c->client;
  bool   use_msg_hash = !no_in3 && in3_plugin_is_registered(rc, PLGN_ACT_PAY_SIGN_REQ);

  sb_add_char(sb, '[');

  for (uint16_t i = 0; i < c->len; i++) {
    d_token_t *  request_token = c->requests[i], *t;
    in3_proof_t  proof         = no_in3 ? PROOF_NONE : in3_req_get_proof(c, i);
    in3_digest_t msg_hash      = use_msg_hash ? crypto_create_hash(DIGEST_KECCAK) : ((in3_digest_t){0});

    if (i > 0) sb_add_char(sb, ',');
    add_id(c, sb, msg_hash, request_token);
    sb_add_chars(sb, ",\"jsonrpc\":\"2.0\",");
    if ((t = d_get(request_token, K_METHOD)) == NULL)
      return req_set_error(c, "missing method-property in request", IN3_EINVAL);
    else
      sb_add_key_value(sb, "method", d_string(t), add_bytes_to_hash(msg_hash, d_string(t), d_len(t)), true);
    if ((t = d_get(request_token, K_PARAMS))) {
      if (d_is_binary_ctx(c->request_context)) return req_set_error(c, "only text json input is allowed", IN3_EINVAL);
      if (msg_hash.ctx) add_token_to_hash(msg_hash, t);
      sb_add_json(sb, ",\"params\":", t);
    }
    else
      sb_add_chars(sb, ",\"params\":[]");

    if (proof || msg_hash.ctx) {
      // add in3
      sb_printx(sb, ",\"in3\":{\"verification\":\"%s\",\"version\":\"%s\"", proof == PROOF_NONE ? "never" : "proof", IN3_PROTO_VER);

      // allow plugins to add their metadata
      in3_pay_payload_ctx_t pctx = {.req = c, .request = request_token, .sb = sb};
      TRY(in3_plugin_execute_first_or_none(c, PLGN_ACT_ADD_PAYLOAD, &pctx))

      if (msg_hash.ctx) {
        // ask the plugin to sign the request
        in3_pay_sign_req_ctx_t sctx = {.req = c, .request = request_token, .signature = {0}};
        crypto_finalize_hash(msg_hash, sctx.request_hash);
        TRY(in3_plugin_execute_first(c, PLGN_ACT_PAY_SIGN_REQ, &sctx))
        sb_printx(sb, ",\"sig\":\"%B\"", bytes(sctx.signature, 65));
      }
      if (rc->finality)
        sb_printx(sb, ",\"finality\":%i", (int32_t) rc->finality);
      if (rc->replace_latest_block)
        sb_printx(sb, ",\"latestBlock\":%i", (int32_t) rc->replace_latest_block);
      if (c->in3_state && c->in3_state->signers_length) {
        bytes_t* s = alloca(c->in3_state->signers_length * sizeof(bytes_t));
        for (int j = 0; j < c->in3_state->signers_length; j++)
          s[j] = bytes(c->in3_state->signers + j * 20, 20);
        sb_add_bytes(sb, ",\"signers\":", s, c->in3_state->signers_length, true);
      }
      if ((rc->flags & FLAGS_INCLUDE_CODE) && strcmp(d_get_string(request_token, K_METHOD), "eth_call") == 0)
        sb_add_chars(sb, ",\"includeCode\":true");
      if (proof == PROOF_FULL)
        sb_add_chars(sb, ",\"useFullProof\":true");
      if ((rc->flags & FLAGS_STATS) == 0)
        sb_add_chars(sb, ",\"noStats\":true");
      if ((rc->flags & FLAGS_BINARY))
        sb_add_chars(sb, ",\"useBinary\":true");
#ifdef BTC_PRE_BPI34
      if (rc->chain.type == CHAIN_BTC)
        sb_add_chars(sb, ",\"preBIP34\":true");
#endif

      // do we have verified hashes?
      if (rc->chain.verified_hashes) {
        uint_fast16_t l = rc->max_verified_hashes;
        for (uint_fast16_t j = 0; j < l; j++) {
          if (!rc->chain.verified_hashes[j].block_number) {
            l = j;
            break;
          }
        }
        if (l) {
          bytes_t* hashes = alloca(sizeof(bytes_t) * l);
          for (uint_fast16_t j = 0; j < l; j++) hashes[j] = bytes(rc->chain.verified_hashes[j].hash, 32);
          sb_add_bytes(sb, ",\"verifiedHashes\":", hashes, l, true);
        }
      }

      in3_pay_handle_ctx_t payload_ctx = {.req = c, .payload = sb};
      TRY(in3_plugin_execute_first_or_none(c, PLGN_ACT_PAY_HANDLE, &payload_ctx))

      sb_add_range(sb, "}}", 0, 2);
    }
    else
      sb_add_char(sb, '}');
  }
  sb_add_char(sb, ']');
  return IN3_OK;
}

NONULL static in3_ret_t ctx_parse_response(in3_req_t* ctx, char* response_data, int len) {
  assert_in3_req(ctx);
  assert(response_data);
  const bool is_json = response_data[0] == '{' || response_data[0] == '[' || response_data[0] == '"';

  if (is_raw_http(ctx)) {
    ctx->response_context = is_json ? parse_json(response_data) : NULL;
    if (!ctx->response_context) {
      // we create a context only holding the raw data
      ctx->response_context               = _calloc(1, sizeof(json_ctx_t));
      ctx->response_context->c            = response_data;
      ctx->response_context->len          = 1;
      ctx->response_context->result       = _calloc(1, sizeof(d_token_t));
      ctx->response_context->result->len  = len;
      ctx->response_context->result->data = (uint8_t*) response_data;
      ctx->response_context->result->state |= TOKEN_STATE_RAW;
    }
    ctx->responses    = _malloc(sizeof(d_token_t*));
    ctx->responses[0] = ctx->response_context->result;
    return IN3_OK;
  }
  else {
    assert(len);
  }

  ctx->response_context = is_json ? parse_json(response_data) : parse_binary_str(response_data, len);

  if (!ctx->response_context) {
    char* error = is_json ? parse_json_error(response_data) : NULL;
    req_set_error(ctx, "\nError in JSON-response : ", req_set_error(ctx, error ? error : str_remove_html(response_data), IN3_EINVALDT));
    _free(error);
    return IN3_EINVALDT;
  }

  if (d_type(ctx->response_context->result) == T_OBJECT) {
    // it is a single result
    ctx->responses    = _malloc(sizeof(d_token_t*));
    ctx->responses[0] = ctx->response_context->result;
    if (ctx->len != 1) return req_set_error(ctx, "The response must be an array!", IN3_EINVALDT);
  }
  else if (d_type(ctx->response_context->result) == T_ARRAY) {
    int        i;
    d_token_t* t = NULL;
    if (d_len(ctx->response_context->result) != (int) ctx->len)
      return req_set_error(ctx, "The responses must be a array with the same number as the requests!", IN3_EINVALDT);
    ctx->responses = _malloc(sizeof(d_token_t*) * ctx->len);
    for (i = 0, t = ctx->response_context->result + 1; i < (int) ctx->len; i++, t = d_next(t))
      ctx->responses[i] = t;
  }
  else
    return req_set_error(ctx, "The response must be a Object or Array", IN3_EINVALDT);

  return IN3_OK;
}

static bool is_user_error(d_token_t* error, char** err_msg) {
  *err_msg = d_type(error) == T_STRING ? d_string(error) : d_get_string(error, K_MESSAGE);
  // here we need to find a better way to detect user errors
  // currently we assume a error-message starting with 'Error:' is a server error and not a user error.
  return *err_msg && strncmp(*err_msg, "Error:", 6) != 0 && strncmp(*err_msg, "TypeError:", 10) != 0 && strncmp(*err_msg, "Error connect", 13) != 0;
}

NONULL static void clear_response(in3_response_t* response) {
  assert_in3_response(response);

  if (response->data.data) { // free up memory
    // clean up invalid data
    _free(response->data.data);
    response->data.data     = NULL;
    response->data.allocted = 0;
    response->data.len      = 0;
  }
}

static in3_ret_t handle_error_response(in3_req_t* ctx, node_match_t* node, in3_response_t* response) {
  assert_in3_req(ctx);
  assert_in3_response(response);

  // and copy the error to the ctx
  if (response->data.len && response->data.data[0] == '{') {
    d_token_t*  value = NULL;
    json_ctx_t* j     = parse_json(response->data.data);
    if (!j)
      req_set_error(ctx, response->data.len ? response->data.data : "no response from node", IN3_ERPC);
    else if ((value = d_get(j->result, K_VALUE))) {
      char* tmp = d_get_string(value, key("detail"));
      if (tmp) req_set_error(ctx, tmp, IN3_ERPC);
      if ((tmp = d_get_string(value, key("title"))))
        req_set_error(ctx, tmp, IN3_ERPC);
      else
        req_set_error(ctx, response->data.data, IN3_ERPC);
    }
  }
  else
    req_set_error(ctx, response->data.len ? response->data.data : "no response from node", IN3_ERPC);

  // we block this node
  in3_nl_blacklist_ctx_t bctx = {.address = node->address, .is_addr = true, .req = ctx};
  if (node && IN3_OK != in3_plugin_execute_first(ctx, PLGN_ACT_NL_BLACKLIST, &bctx))
    clear_response(response); // free up memory

  return IN3_ERPC;
}

static void clean_up_ctx(in3_req_t* ctx) {
  assert_in3_req(ctx);

  if (ctx->verification_state != IN3_OK && ctx->verification_state != IN3_WAITING) ctx->verification_state = IN3_WAITING;
  if (ctx->error) _free(ctx->error);
  if (ctx->responses) _free(ctx->responses);
  if (ctx->response_context) json_free(ctx->response_context);
  ctx->error = NULL;
}

NONULL in3_ret_t in3_retry_same_node(in3_req_t* ctx) {

  int nodes_count = ctx->in3_state ? req_nodes_len(ctx->in3_state->nodes) : 1;
  // this means we need to retry with the same node
  for (int i = 0; i < nodes_count; i++) {
    if (ctx->raw_response[i].data.data)
      _free(ctx->raw_response[i].data.data);
  }
  _free(ctx->raw_response);
  _free(ctx->responses);
  json_free(ctx->response_context);

  ctx->raw_response     = NULL;
  ctx->response_context = NULL;
  ctx->responses        = NULL;
  return IN3_OK;
}

static in3_ret_t handle_payment(in3_vctx_t* vc, node_match_t* node, int index) {
  in3_req_t*             ctx  = vc->req;
  in3_pay_followup_ctx_t fctx = {.req = ctx, .node = node, .resp_in3 = vc->proof, .resp_error = d_get(ctx->responses[index], K_ERROR)};
  return req_set_error(ctx, "Error following up the payment data", in3_plugin_execute_first_or_none(ctx, PLGN_ACT_PAY_FOLLOWUP, &fctx));
}

static in3_ret_t verify_response(in3_req_t* ctx, in3_chain_t* chain, node_match_t* node, in3_response_t* response) {
  assert_in3_req(ctx);
  assert(chain);
  assert_in3_response(response);

  in3_ret_t res = IN3_OK;

  if (response->state || (!is_raw_http(ctx) && !response->data.len)) // reponse has an error
    return handle_error_response(ctx, node, response);

  // we need to clean up the previos responses if set
  clean_up_ctx(ctx);

  // parse
  if (ctx_parse_response(ctx, response->data.data, response->data.len)) {
    // in case of an error we get a error-code and error is set in the ctx?
    // so we need to block the node.
    if (node) {
      in3_nl_blacklist_ctx_t bctx = {.address = node->address, .is_addr = true, .req = ctx};
      in3_plugin_execute_first(ctx, PLGN_ACT_NL_BLACKLIST, &bctx);
    }
    clear_response(response); // we want to save memory and free the invalid response
    return ctx->verification_state;
  }

  // this was a internal response, so we don't need to verify the response
  if (!node) return (ctx->verification_state = IN3_OK);

  // check each request
  for (uint_fast16_t i = 0; i < ctx->len; i++) {
    in3_vctx_t vc;
    vc.req            = ctx;
    vc.chain          = chain;
    vc.request        = ctx->requests[i];
    vc.result         = d_get(ctx->responses[i], K_RESULT);
    vc.client         = ctx->client;
    vc.index          = (int) i;
    vc.method         = d_get_string(vc.request, K_METHOD);
    vc.node           = node;
    vc.dont_blacklist = false;
    vc.proof          = d_get(ctx->responses[i], K_IN3); // vc.proof is temporary set to the in3-section. It will be updated to real proof in the next lines.
    res               = handle_payment(&vc, node, i);

    if (vc.proof) { // vc.proof is temporary set to the in3-section. It will be updated to real proof in the next lines.
      vc.last_validator_change = d_get_long(vc.proof, K_LAST_VALIDATOR_CHANGE);
      vc.currentBlock          = d_get_long(vc.proof, K_CURRENT_BLOCK);
      vc.proof                 = d_get(vc.proof, K_PROOF);
    }

    // no result?
    if (!res && !vc.result) {
      char* err_msg;
      // if we don't have a result, the node reported an error
      if (is_user_error(d_get(ctx->responses[i], K_ERROR), &err_msg)) {
        in3_log_debug("we have a user-error from node, so we reject the response, but don't blacklist ..\n");
        continue;
      }
      else {
        in3_log_debug("we have a system-error from node, so we block it ..\n");
        in3_nl_blacklist_ctx_t bctx = {.address = node->address, .is_addr = true, .req = ctx};
        in3_plugin_execute_first(ctx, PLGN_ACT_NL_BLACKLIST, &bctx);
        return req_set_error(ctx, err_msg ? err_msg : "Invalid response", IN3_EINVAL);
      }
    }

    // verify the response
    if (res == IN3_OK) res = ctx->verification_state = in3_plugin_execute_first(ctx, PLGN_ACT_RPC_VERIFY, &vc);

    // Waiting is ok, but we stop here
    if (res == IN3_WAITING)
      return res;

    // if this is an error, we blacklist the node and return the error.
    if (res) {
      // before we blacklist the node, we remove the data and replace it with the error-message
      // this is needed in case it will be cleared and we don't want to lose the error message
      if (ctx->error && response->data.data) {
        _free(response->data.data);
        size_t l        = strlen(ctx->error);
        response->state = res;
        response->data  = (sb_t){.data = _strdupn(ctx->error, l), .allocted = l + 1, .len = l};
      }
      if (!vc.dont_blacklist) {
        in3_nl_blacklist_ctx_t bctx = {.address = node->address, .is_addr = true, .req = ctx};
        in3_plugin_execute_first(ctx, PLGN_ACT_NL_BLACKLIST, &bctx);
      }
      return res;
    }

    // if it was ok, we continue to verify the other responses.
  }

  // all is ok
  return (ctx->verification_state = IN3_OK);
}

static in3_ret_t find_valid_result(in3_req_t* ctx, node_match_t** vnode) {

  int             nodes_count   = (!ctx->in3_state || !ctx->in3_state->nodes) ? 1 : req_nodes_len(ctx->in3_state->nodes);
  in3_response_t* response      = ctx->raw_response;
  in3_chain_t*    chain         = in3_get_chain(ctx->client, in3_chain_id(ctx));
  node_match_t*   node          = ctx->in3_state ? ctx->in3_state->nodes : NULL;
  bool            still_pending = false;
  in3_ret_t       state         = IN3_ERPC;

  // blacklist nodes for missing response
  for (int n = 0; n < nodes_count; n++, node = node ? node->next : NULL) {
    // if the response is still pending, we skip...
    if (response[n].state == IN3_WAITING) {
      still_pending = true;
      in3_log_debug("request from node is still pending ..\n");
      continue;
    }

    state = verify_response(ctx, chain, node, response + n);
    if (state == IN3_OK) {
      in3_log_debug(COLOR_GREEN "accepted response for %s from %s\n" COLOR_RESET, d_get_string(ctx->requests[0], K_METHOD), node ? node->url : "intern");
      break;
    }
    else if (state == IN3_WAITING)
      return state;
    // in case of an error, we keep on trying....
  }

  // no valid response found,
  // if pending, we remove the error and wait
  if (state && still_pending) {
    in3_log_debug("failed to verify, but waiting for pending\n");
    if (ctx->error) _free(ctx->error);
    if (ctx->responses) _free(ctx->responses);
    if (ctx->response_context) json_free(ctx->response_context);
    ctx->error              = NULL;
    ctx->verification_state = IN3_WAITING;
    ctx->response_context   = NULL;
    ctx->responses          = NULL;
    return IN3_WAITING;
  }

  // if the last state is an error we report this as failed
  if (state) return state;

  *vnode = node;
  return IN3_OK;
}

NONULL in3_http_request_t* in3_create_request(in3_req_t* ctx) {
  switch (in3_req_state(ctx)) {
    case REQ_ERROR:
      req_set_error(ctx, "You cannot create an request if the was an error!", IN3_EINVAL);
      return NULL;
    case REQ_SUCCESS:
      return NULL;
    case REQ_WAITING_FOR_RESPONSE:
      req_set_error(ctx, "There are pending requests, finish them before creating a new one!", IN3_EINVAL);
      return NULL;
    case REQ_WAITING_TO_SEND: {
      in3_req_t* p = ctx;
      for (; p; p = p->required) {
        if (!p->raw_response) ctx = p;
      }
    }
  }

  if (is_raw_http(ctx)) {
    // prepare response-object
    d_token_t* params = d_get(ctx->requests[0], K_PARAMS);
    if (d_len(params) < 2) {
      req_set_error(ctx, "invalid number of arguments, must be [METHOD,URL,PAYLOAD,HEADER]", IN3_EINVAL);
      return NULL;
    }
    char*               method  = d_get_string_at(params, 0);
    d_token_t*          tmp     = d_get_at(params, 2);
    in3_http_request_t* request = _calloc(sizeof(in3_http_request_t), 1);
    request->req                = ctx;
    request->urls_len           = 1;
    request->urls               = _malloc(sizeof(char*));
    request->urls[0]            = _strdupn(d_get_string_at(params, 1), -1);
    request->method             = method ? method : (*request->payload ? "POST" : "GET");
    request->wait               = d_get_int(d_get(ctx->requests[0], K_IN3), K_WAIT);
    ctx->raw_response           = _calloc(sizeof(in3_response_t), 1);
    ctx->raw_response[0].state  = IN3_WAITING;

    switch (d_type(tmp)) {
      case T_NULL:
        request->payload = _calloc(1, 1);
        break;
      case T_STRING:
        request->payload     = _strdupn(d_string(tmp), -1);
        request->payload_len = d_len(tmp);
        break;
      case T_BYTES:
        request->payload     = _strdupn((void*) tmp->data, tmp->len);
        request->payload_len = d_len(tmp);
        break;
      default:
        request->payload     = d_create_json(ctx->request_context, tmp);
        request->payload_len = strlen(request->payload);
        break;
    }

    for_children_of(iter, d_get_at(params, 3)) {
      in3_req_header_t* t = _malloc(sizeof(in3_req_header_t));
      t->value            = d_string(iter.token);
      t->next             = request->headers;
      request->headers    = t;
    }

    return request;
  }

  in3_ret_t     res;
  node_match_t* node        = ctx->in3_state ? ctx->in3_state->nodes : NULL;
  char*         rpc         = d_get_string(d_get(ctx->requests[0], K_IN3), K_RPC);
  int           nodes_count = rpc ? 1 : req_nodes_len(node);
  char**        urls        = nodes_count ? _malloc(sizeof(char*) * nodes_count) : NULL;

  for (int n = 0; n < nodes_count; n++) {
    urls[n] = _strdupn(rpc ? rpc : node->url, -1);
    assert(urls[n] != NULL);

    // this is all we need to do if we have a rpc-node
    if (rpc) break;

    node = node->next;
  }

  // prepare the payload
  sb_t payload = {0};
  res          = ctx_create_payload(ctx, &payload, rpc != NULL);
  if (res < 0) {
    // we clean up
    _free(payload.data);
    free_urls(urls, nodes_count);
    // since we cannot return an error, we set the error in the context and return NULL, indicating the error.
    req_set_error(ctx, "could not generate the payload", res);
    return NULL;
  }

  // prepare response-object
  in3_http_request_t* request = _calloc(sizeof(in3_http_request_t), 1);
  request->req                = ctx;
  request->payload            = payload.data;
  request->payload_len        = payload.len;
  request->urls_len           = nodes_count;
  request->urls               = urls;
  request->cptr               = NULL;
  request->wait               = d_get_int(d_get(ctx->requests[0], K_IN3), K_WAIT);
  request->method             = payload.len ? "POST" : "GET";

  if (!nodes_count) nodes_count = 1; // at least one result, because for internal response we don't need nodes, but a result big enough.
  ctx->raw_response = _calloc(sizeof(in3_response_t), nodes_count);
  for (int n = 0; n < nodes_count; n++) ctx->raw_response[n].state = IN3_WAITING;

  return request;
}

NONULL void request_free(in3_http_request_t* req) {
  // free resources
  free_urls(req->urls, req->urls_len);
  for (in3_req_header_t* h = req->headers; h; h = req->headers) {
    req->headers = h->next;
    _free(h);
  }
  _free(req->payload);
  _free(req);
}

NONULL static bool ctx_is_allowed_to_fail(in3_req_t* ctx) {
  return req_is_method(ctx, "in3_nodeList");
}

in3_req_t* in3_req_last_waiting(in3_req_t* ctx) {
  in3_req_t* last = ctx;
  for (; ctx; ctx = ctx->required) {
    if (!ctx->response_context) last = ctx;
  }
  return last;
}

static void init_sign_ctx(in3_req_t* ctx, in3_sign_ctx_t* sign_ctx) {
  d_token_t* params     = d_get(ctx->requests[0], K_PARAMS);
  sign_ctx->message     = d_bytes(d_get_at(params, 0));
  sign_ctx->account     = d_bytes(d_get_at(params, 1));
  sign_ctx->digest_type = SIGN_EC_HASH;
  sign_ctx->curve_type  = ECDSA_SECP256K1;
  sign_ctx->req         = ctx;
  sign_ctx->signature   = NULL_BYTES;
}

in3_sign_ctx_t* create_sign_ctx(in3_req_t* ctx) {
  in3_sign_ctx_t* res = _malloc(sizeof(in3_sign_ctx_t));
  init_sign_ctx(ctx, res);
  return res;
}

in3_ret_t in3_handle_sign(in3_req_t* ctx) {
  in3_sign_ctx_t sign_ctx;
  init_sign_ctx(ctx, &sign_ctx);
  if (!sign_ctx.message.data) return req_set_error(ctx, "missing data to sign", IN3_ECONFIG);
  if (!sign_ctx.account.data) return req_set_error(ctx, "missing account to sign", IN3_ECONFIG);

  ctx->raw_response = _calloc(sizeof(in3_response_t), 1);
  sb_init(&ctx->raw_response[0].data);
  in3_log_trace("... request to sign ");
  in3_ret_t r = in3_plugin_execute_first(ctx, PLGN_ACT_SIGN, &sign_ctx);
  if (r == IN3_OK)
    sb_add_range(&ctx->raw_response->data, (char*) sign_ctx.signature.data, 0, sign_ctx.signature.len);
  if (sign_ctx.signature.data) _free(sign_ctx.signature.data);
  return r;
}

typedef struct {
  in3_req_t* req;
  void*      ptr;
} ctx_req_t;
typedef struct {
  int        len;
  ctx_req_t* req;
} ctx_req_transports_t;

static void transport_cleanup(in3_req_t* ctx, ctx_req_transports_t* transports, bool free_all) {
  for (int i = 0; i < transports->len; i++) {
    if ((free_all && transports->req[i].req) || transports->req[i].req == ctx) {
      in3_http_request_t req = {.req = ctx, .cptr = transports->req[i].ptr, .urls_len = 0, .urls = NULL, .payload = NULL};
      in3_plugin_execute_first_or_none(ctx, PLGN_ACT_TRANSPORT_CLEAN, &req);
      if (!free_all) {
        transports->req[i].req = NULL;
        return;
      }
    }
  }
  if (free_all && transports->req) _free(transports->req);
}

static void in3_handle_rpc_next(in3_req_t* ctx, ctx_req_transports_t* transports) {
  in3_log_debug("waiting for the next response ...\n");
  ctx = in3_req_last_waiting(ctx);
  for (int i = 0; i < transports->len; i++) {
    if (transports->req[i].req == ctx) {
      in3_http_request_t req = {.req = ctx, .cptr = transports->req[i].ptr, .urls_len = 0, .urls = NULL, .payload = NULL};
      in3_plugin_execute_first(ctx, PLGN_ACT_TRANSPORT_RECEIVE, &req);
#ifdef DEBUG
      node_match_t* w = ctx->in3_state ? ctx->in3_state->nodes : NULL;
      int           j = 0;
      for (; w; j++, w = w->next) {
        if (ctx->raw_response[j].state != IN3_WAITING && ctx->raw_response[j].data.data && ctx->raw_response[j].time) {
          char* data = ctx->raw_response[j].data.data;
          data       = format_json(data);

          in3_log_trace(ctx->raw_response[j].state
                            ? "... response(%s): \n... " COLOR_RED_STR "\n"
                            : "... response(%s): \n... " COLOR_GREEN_STR "\n",
                        w ? w->url : "intern", data);
          _free(data);
        }
      }
#endif
      return;
    }
  }

  // looks like we were not able to send out the first request, so waiting for the second won't help.
  node_match_t* w = ctx->in3_state ? ctx->in3_state->nodes : NULL;
  for (int j = 0; w; j++, w = w->next) {
    if (ctx->raw_response[j].state == IN3_WAITING && !ctx->raw_response[j].data.data) {
      in3_ctx_add_response(ctx, j, IN3_ERPC, "The request could not be send!", -1, 1);
      return;
    }
  }

  req_set_error(ctx, "waiting to fetch more responses, but no cptr was registered", IN3_ENOTSUP);
}

void in3_handle_rpc(in3_req_t* ctx, ctx_req_transports_t* transports) {
  // if we can't create the request, this function will put it into error-state
  in3_http_request_t* request = in3_create_request(ctx);
  if (!request) return;

  // do we need to wait?
  if (request->wait)
    in3_sleep(request->wait);
  // in case there is still a old cptr we need to cleanup since this means this is a retry!
  transport_cleanup(ctx, transports, false);

  // debug output
  for (unsigned int i = 0; i < request->urls_len; i++)
    in3_log_trace("... request to " COLOR_YELLOW_STR "\n... " COLOR_MAGENTA_STR "\n", request->urls[i], i == 0 ? request->payload : "");

  // handle it (only if there is a transport)
  in3_plugin_execute_first(ctx, PLGN_ACT_TRANSPORT_SEND, request);

  // debug output
  node_match_t* node = request->req->in3_state ? request->req->in3_state->nodes : NULL;
  for (unsigned int i = 0; i < request->urls_len; i++, node = node ? node->next : NULL) {
    if (request->req->raw_response[i].state != IN3_WAITING) {
      char* data = request->req->raw_response[i].data.data;
      UNUSED_VAR(data); // this makes sure we don't get a warning when building with _DLOGGING=false
#ifdef DEBUG
      data = format_json(data);
#endif
      in3_log_trace(request->req->raw_response[i].state
                        ? "... response(%s): \n... " COLOR_RED_STR "\n"
                        : "... response(%s): \n... " COLOR_GREEN_STR "\n",
                    node ? node->url : "intern", data ? data : "no data");
#ifdef DEBUG
      _free(data);
#endif
    }
  }

  // in case we have a cptr, we need to save it in the transports
  if (request && request->cptr) {
    // find a free spot
    int index = -1;
    for (int i = 0; i < transports->len; i++) {
      if (!transports->req[i].req) {
        index = i;
        break;
      }
    }
    if (index == -1) {
      transports->req = transports->len ? _realloc(transports->req, sizeof(ctx_req_t) * (transports->len + 1), sizeof(ctx_req_t) * transports->len) : _malloc(sizeof(ctx_req_t));
      index           = transports->len++;
    }

    // store the pointers
    transports->req[index].req = request->req;
    transports->req[index].ptr = request->cptr;
  }

  // we will cleanup even though the reponses may still be pending
  request_free(request);
}

in3_ret_t in3_send_req(in3_req_t* ctx) {
  ctx_req_transports_t transports = {0};
  while (true) {
    switch (in3_req_exec_state(ctx)) {
      case REQ_ERROR:
      case REQ_SUCCESS:
        transport_cleanup(ctx, &transports, true);
        return ctx->verification_state;
      case REQ_WAITING_FOR_RESPONSE:
        in3_handle_rpc_next(ctx, &transports);
        break;
      case REQ_WAITING_TO_SEND: {
        in3_req_t* last = in3_req_last_waiting(ctx);
        switch (last->type) {
          case RT_SIGN:
            in3_handle_sign(last);
            break;
          case RT_RPC:
            in3_handle_rpc(last, &transports);
        }
      }
    }
  }
}

/**
 * helper function to set the signature on the signer context and rpc context
 */
void in3_sign_ctx_set_signature(
    in3_req_t*      ctx,
    in3_sign_ctx_t* sign_ctx) {
  ctx->raw_response = _calloc(sizeof(in3_response_t), 1);
  sb_init(&ctx->raw_response[0].data);
  sb_add_range(&ctx->raw_response->data, (char*) sign_ctx->signature.data, 0, sign_ctx->signature.len);
  _free(sign_ctx->signature.data);
}

in3_req_t* req_find_required(const in3_req_t* parent, const char* search_method, const char* param_query) {
  for (in3_req_t* r = parent->required; r; r = r->required) {
    if (!r->requests) continue;
    if (req_is_method(r, search_method)) {
      d_token_t* params = d_get(r->requests[0], K_PARAMS);
      if (param_query && (!params || !params->data || !str_find((void*) params->data, param_query))) continue;
      return r;
    }
  }
  return NULL;
}

in3_ret_t req_add_required(in3_req_t* parent, in3_req_t* ctx) {
  //  printf(" ++ add required %s > %s\n", ctx_name(parent), ctx_name(ctx));
  ctx->required    = parent->required;
  parent->required = ctx;
  return in3_req_execute(ctx);
}

in3_ret_t req_remove_required(in3_req_t* parent, in3_req_t* ctx, bool rec) {
  if (!ctx) return IN3_OK;
  in3_req_t* p = parent;
  while (p) {
    if (p->required == ctx) {
      //      printf(" -- remove required %s > %s\n", ctx_name(parent), ctx_name(ctx));
      in3_req_t* next = rec ? NULL : ctx->required;
      if (!rec) ctx->required = NULL;
      req_free_intern(ctx, true);
      p->required = next;
      return IN3_OK;
    }
    p = p->required;
  }
  return IN3_EFIND;
}

in3_req_state_t in3_req_state(in3_req_t* ctx) {
  if (ctx == NULL) return REQ_SUCCESS;
  in3_req_state_t required_state = ctx->required ? in3_req_state(ctx->required) : REQ_SUCCESS;
  if (required_state == REQ_ERROR || ctx->error) return REQ_ERROR;
  if (ctx->required && required_state != REQ_SUCCESS) return required_state;
  if (!ctx->raw_response) return REQ_WAITING_TO_SEND;
  if (ctx->type == RT_RPC && !ctx->response_context) return REQ_WAITING_FOR_RESPONSE;
  if (ctx->type == RT_SIGN && ctx->raw_response->state == IN3_WAITING) return REQ_WAITING_FOR_RESPONSE;
  return REQ_SUCCESS;
}

void req_free(in3_req_t* ctx) {
  if (ctx) req_free_intern(ctx, false);
}

static inline in3_ret_t handle_internally(in3_req_t* ctx) {
  if (ctx->len != 1) return IN3_OK; //  currently we do not support bulk requests forr internal calls
  in3_rpc_handle_ctx_t vctx = {.req = ctx, .response = &ctx->raw_response, .request = ctx->requests[0], .method = d_get_string(ctx->requests[0], K_METHOD), .params = d_get(ctx->requests[0], K_PARAMS)};
  in3_ret_t            res  = in3_plugin_execute_first_or_none(ctx, PLGN_ACT_RPC_HANDLE, &vctx);
  if (res == IN3_OK && ctx->raw_response && ctx->raw_response->data.data) in3_log_debug("internal response: %s\n", ctx->raw_response->data.data);
  return res == IN3_EIGNORE ? IN3_OK : res;
}

static inline char* get_error_message(in3_req_t* ctx, in3_ret_t e) {
  if (e == IN3_WAITING) return NULL;
  for (; ctx; ctx = ctx->required) {
    if (ctx->error) return ctx->error;
  }

  return in3_errmsg(e);
}

in3_req_state_t in3_req_exec_state(in3_req_t* req) {
  in3_req_execute(req);
  return in3_req_state(req);
}

static inline in3_ret_t handle_error(in3_req_t* req, in3_ret_t ret) {

  // we count this is an attempt ( if the raw_response is null, it means we retry because of payments, so we don't count it)
  if (req->raw_response) req->attempt++;

  // if not, then we clean up
  response_free(req);

  // should we retry?
  if (req->attempt < req->client->max_attempts) {
    in3_log_debug("Retrying send request...\n");
    // reset the error and try again
    if (req->error) _free(req->error);
    req->error              = NULL;
    req->verification_state = IN3_WAITING;
    // now try again, which should end in waiting for the next request.
    return in3_req_execute(req);
  }
  else {
    if (ctx_is_allowed_to_fail(req))
      req->verification_state = ret = IN3_EIGNORE;
    // we give up
    return req->error ? (ret ? ret : IN3_ERPC) : req_set_error(req, "reaching max_attempts and giving up", IN3_ELIMIT);
  }
}

static inline in3_ret_t select_nodes(in3_req_t* req) {
  in3_ret_t ret = IN3_OK;

  // we only need to pick nodes, if we don't have an anser or no nodes picked
  if (req->raw_response || (req->in3_state && req->in3_state->nodes)) return ret;

  // if the request has a rpc-url or a REST-request, we don't pick nodes.
  if (d_get(d_get(req->requests[0], K_IN3), K_RPC) || is_raw_http(req)) return ret;

  // time to create the in3_state
  if (!req->in3_state) req->in3_state = _calloc(1, sizeof(in3_state_t));

  // pick data nodes first
  in3_nl_pick_ctx_t pctx = {.type = NL_DATA, .req = req};
  if ((ret = in3_plugin_execute_first(req, PLGN_ACT_NL_PICK, &pctx)))                                       // did a plugin select the nodes successfully?
    return req_set_error(req, "could not find any node",                                                    // report the error
                         ret < 0 && ret != IN3_WAITING && ctx_is_allowed_to_fail(req) ? IN3_EIGNORE : ret); // the error-code depends if it is allowed to fail

  // pick signer nodes now
  pctx.type = NL_SIGNER;                                                                                    // we now select the signer-nodes
  if ((ret = in3_plugin_execute_first(req, PLGN_ACT_NL_PICK, &pctx)) < 0)                                   // did it fail?
    return req_set_error(req, "error configuring the config for request",                                   // report the error
                         ret < 0 && ret != IN3_WAITING && ctx_is_allowed_to_fail(req) ? IN3_EIGNORE : ret); // the error-code depends if it is allowed to fail

  // if a plugin needs to prepare some payment, now is a good time....
  return in3_plugin_execute_first_or_none(req, PLGN_ACT_PAY_PREPARE, req);
}

in3_ret_t in3_req_execute(in3_req_t* req) {
  in3_ret_t ret = IN3_OK;

  // if there is an error it does not make sense to execute.
  if (req->error) return (req->verification_state && req->verification_state != IN3_WAITING) ? req->verification_state : IN3_EUNKNOWN;

  // if we have required-contextes, we need to check them first
  if (req->required && (ret = in3_req_execute(req->required))) {
    if (ret == IN3_EIGNORE)
      in3_plugin_execute_first(req, PLGN_ACT_NL_FAILABLE, req);
    else
      return req_set_error(req, get_error_message(req->required, ret), ret);
  }

  // if there is response we are done.
  if (req->response_context && req->verification_state == IN3_OK) return IN3_OK;

  switch (req->type) {
    case RT_RPC: {

      // do we need to handle it internaly?
      if (!req->raw_response && !req->response_context && (ret = handle_internally(req)) < 0)
        // there was weither an error or a WAITING, so we return here
        return req->error ? ret : req_set_error(req, get_error_message(req, ret), ret);

      // if this is an incubed-request, we need the nodelist and pick the nodes.
      if ((ret = select_nodes(req))) return ret;

      // if we still don't have an response, we keep on waiting
      if (!req->raw_response) return IN3_WAITING;

      // ok, we have a response, then we try to evaluate the responses
      // verify responses and return the node with the correct result.
      node_match_t* valid_node = NULL;
      ret                      = find_valid_result(req, &valid_node);

      // if all is good and there was a in3 node delivering the response, we may want to do a followup now
      if (ret == IN3_OK && req->in3_state && valid_node) {
        // allow payments to handle post actions
        in3_nl_followup_ctx_t fctx = {.req = req, .node = valid_node};
        in3_plugin_execute_first_or_none(req, PLGN_ACT_NL_PICK_FOLLOWUP, &fctx);
      }

      // we wait or are have successfully verified the response
      if (ret == IN3_WAITING || ret == IN3_OK)
        return ret;
      else
        return handle_error(req, ret);
    }

    case RT_SIGN: {
      if (!req->raw_response || req->raw_response->state == IN3_WAITING)
        return IN3_WAITING;
      else if (req->raw_response->state)
        return IN3_ERPC;
      return IN3_OK;
    }
    default:
      return IN3_EINVAL;
  }
}
