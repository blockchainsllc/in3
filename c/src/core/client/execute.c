/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
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

#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/secp256k1.h"
#include "../../third-party/crypto/sha3.h"
#include "../util/data.h"
#include "../util/log.h"
#include "../util/mem.h"
#include "../util/stringbuilder.h"
#include "../util/utils.h"
#include "cache.h"
#include "client.h"
#include "context_internal.h"
#include "keys.h"
#include "nodelist.h"
#include "plugin.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define WAIT_TIME_CAP 3600
#define BLACKLISTTIME 24 * 3600

NONULL static void response_free(in3_ctx_t* ctx) {
  assert_in3_ctx(ctx);

  int nodes_count = 1;
  if (ctx->nodes) {
    nodes_count = ctx_nodes_len(ctx->nodes);
    in3_ctx_free_nodes(ctx->nodes);
  }
  if (ctx->raw_response) {
    for (int i = 0; i < nodes_count; i++) {
      if (ctx->raw_response[i].data.data) _free(ctx->raw_response[i].data.data);
    }
    _free(ctx->raw_response);
  }

  if (ctx->responses) _free(ctx->responses);
  if (ctx->response_context) json_free(ctx->response_context);
  if (ctx->signers) _free(ctx->signers);
  ctx->response_context = NULL;
  ctx->responses        = NULL;
  ctx->raw_response     = NULL;
  ctx->nodes            = NULL;
  ctx->signers          = NULL;
  ctx->signers_length   = 0;
}
NONULL void in3_check_verified_hashes(in3_t* c) {
  // shrink verified hashes to max_verified_hashes
  if (c->pending <= 1 && c->alloc_verified_hashes > c->max_verified_hashes) {
    in3_chain_t* chain = in3_get_chain(c);
    // we want to keep the newest entries, so we move them overriding the oldest
    memmove(chain->verified_hashes,
            chain->verified_hashes + (c->alloc_verified_hashes - c->max_verified_hashes),
            sizeof(in3_verified_hash_t) * c->max_verified_hashes);
    chain->verified_hashes   = _realloc(chain->verified_hashes,
                                      c->max_verified_hashes * sizeof(in3_verified_hash_t),
                                      c->alloc_verified_hashes * sizeof(in3_verified_hash_t));
    c->alloc_verified_hashes = c->max_verified_hashes;
  }
}
NONULL static void ctx_free_intern(in3_ctx_t* ctx, bool is_sub) {
  assert_in3_ctx(ctx);
  // only for intern requests, we actually free the original request-string
  if (is_sub)
    _free(ctx->request_context->c);
  ctx->client->pending--;
  if (ctx->error) _free(ctx->error);
  response_free(ctx);
  if (ctx->request_context)
    json_free(ctx->request_context);

  if (ctx->requests) _free(ctx->requests);
  if (ctx->cache) in3_cache_free(ctx->cache, !is_sub);
  if (ctx->required) ctx_free_intern(ctx->required, true);

  in3_check_verified_hashes(ctx->client);
  _free(ctx);
}

NONULL static bool auto_ask_sig(const in3_ctx_t* ctx) {
  return (ctx_is_method(ctx, "in3_nodeList") && !(ctx->client->flags & FLAGS_NODE_LIST_NO_SIG) && ctx->client->chain_id != CHAIN_ID_BTC);
}

NONULL static in3_ret_t pick_signers(in3_ctx_t* ctx, d_token_t* request) {
  assert_in3_ctx(ctx);

  const in3_t*       c     = ctx->client;
  const in3_chain_t* chain = in3_get_chain(c);

  if (in3_ctx_get_proof(ctx, 0) == PROOF_NONE && !auto_ask_sig(ctx))
    return IN3_OK;

  // For nodeList request, we always ask for proof & atleast one signature
  uint8_t total_sig_cnt = c->signature_count
                              ? c->signature_count
                              : (auto_ask_sig(ctx) ? 1 : 0);

  if (total_sig_cnt) {
    node_match_t*     signer_nodes = NULL;
    in3_node_filter_t filter       = NODE_FILTER_INIT;
    filter.nodes                   = d_get(d_get(request, K_IN3), K_SIGNER_NODES);
    filter.props                   = c->node_props | NODE_PROP_SIGNER;
    const in3_ret_t res            = in3_node_list_pick_nodes(ctx, &signer_nodes, total_sig_cnt, filter);
    if (res < 0)
      return ctx_set_error(ctx, "Could not find any nodes for requesting signatures", res);
    if (ctx->signers) _free(ctx->signers);
    const int node_count  = ctx_nodes_len(signer_nodes);
    ctx->signers_length   = node_count;
    ctx->signers          = _malloc(20 * node_count); // 20 bytes per address
    const node_match_t* w = signer_nodes;
    in3_node_t*         n = NULL;
    for (int i = 0; i < node_count; i++) {
      n = ctx_get_node(chain, w);
      if (n) memcpy(ctx->signers + i * 20, n->address, 20);
      w = w->next;
    }
    if (signer_nodes) in3_ctx_free_nodes(signer_nodes);
  }

  return IN3_OK;
}

static void free_urls(char** urls, int len, bool free_items) {
  if (!urls) return;
  if (free_items) {
    for (int i = 0; i < len; i++) _free(urls[i]);
  }
  _free(urls);
}

static int add_bytes_to_hash(struct SHA3_CTX* msg_hash, void* data, int len) {
  assert(data);
  if (msg_hash) sha3_Update(msg_hash, data, len);
  return len;
}

NONULL static void add_token_to_hash(struct SHA3_CTX* msg_hash, d_token_t* t) {
  switch (d_type(t)) {
    case T_ARRAY:
    case T_OBJECT:
      for (d_iterator_t iter = d_iter(t); iter.left; d_iter_next(&iter))
        add_token_to_hash(msg_hash, iter.token);
      return;
    case T_NULL:
      return;

    default: {
      bytes_t b = d_to_bytes(t);
      sha3_Update(msg_hash, b.data, b.len);
    }
  }
}

NONULL static in3_ret_t ctx_create_payload(in3_ctx_t* c, sb_t* sb, bool multichain, bool no_in3) {
  assert_in3_ctx(c);
  assert(sb);

  static unsigned long rpc_id_counter = 1;
  char                 temp[100];
  in3_t*               rc       = c->client;
  struct SHA3_CTX*     msg_hash = !no_in3 && in3_plugin_is_registered(c->client, PLGN_ACT_PAY_SIGN_REQ) ? alloca(sizeof(struct SHA3_CTX)) : NULL;

  sb_add_char(sb, '[');

  for (uint_fast16_t i = 0; i < c->len; i++) {
    d_token_t * request_token = c->requests[i], *t;
    in3_proof_t proof         = no_in3 ? PROOF_NONE : in3_ctx_get_proof(c, i);
    if (msg_hash) sha3_256_Init(msg_hash);

    if (i > 0) sb_add_char(sb, ',');
    sb_add_char(sb, '{');
    if ((t = d_get(request_token, K_ID)) == NULL)
      sb_add_key_value(sb, "id", temp, add_bytes_to_hash(msg_hash, temp, sprintf(temp, "%lu", rpc_id_counter++)), false);
    else if (d_type(t) == T_INTEGER)
      sb_add_key_value(sb, "id", temp, add_bytes_to_hash(msg_hash, temp, sprintf(temp, "%i", d_int(t))), false);
    else
      sb_add_key_value(sb, "id", d_string(t), add_bytes_to_hash(msg_hash, d_string(t), d_len(t)), true);
    sb_add_char(sb, ',');
    sb_add_key_value(sb, "jsonrpc", "2.0", 3, true);
    sb_add_char(sb, ',');
    if ((t = d_get(request_token, K_METHOD)) == NULL)
      return ctx_set_error(c, "missing method-property in request", IN3_EINVAL);
    else
      sb_add_key_value(sb, "method", d_string(t), add_bytes_to_hash(msg_hash, d_string(t), d_len(t)), true);
    sb_add_char(sb, ',');
    if ((t = d_get(request_token, K_PARAMS)) == NULL)
      sb_add_key_value(sb, "params", "[]", 2, false);
    else {
      if (d_is_binary_ctx(c->request_context)) return ctx_set_error(c, "only text json input is allowed", IN3_EINVAL);
      const str_range_t ps = d_to_json(t);
      if (msg_hash) add_token_to_hash(msg_hash, t);
      sb_add_key_value(sb, "params", ps.data, ps.len, false);
    }

    if (proof || msg_hash) {
      // add in3
      sb_add_range(sb, temp, 0, sprintf(temp, ",\"in3\":{\"verification\":\"%s\",\"version\": \"%s\"", proof == PROOF_NONE ? "never" : "proof", IN3_PROTO_VER));
      if (multichain)
        sb_add_range(sb, temp, 0, sprintf(temp, ",\"chainId\":\"0x%x\"", (unsigned int) rc->chain_id));
      const in3_chain_t* chain = in3_get_chain(rc);
      if (chain->whitelist) {
        const bytes_t adr = bytes(chain->whitelist->contract, 20);
        sb_add_bytes(sb, ",\"whiteListContract\":", &adr, 1, false);
      }
      if (msg_hash) {
        in3_pay_sign_req_ctx_t sctx      = {.ctx = c, .request = request_token, .signature = {0}};
        bytes_t                sig_bytes = bytes(sctx.signature, 65);
        keccak_Final(msg_hash, sctx.request_hash);
        TRY(in3_plugin_execute_first(c, PLGN_ACT_PAY_SIGN_REQ, &sctx))
        sb_add_bytes(sb, ",\"sig\":", &sig_bytes, 1, false);
      }
      if (rc->finality)
        sb_add_range(sb, temp, 0, sprintf(temp, ",\"finality\":%i", rc->finality));
      if (rc->replace_latest_block)
        sb_add_range(sb, temp, 0, sprintf(temp, ",\"latestBlock\":%i", rc->replace_latest_block));
      if (c->signers_length) {
        bytes_t* s = alloca(c->signers_length * sizeof(bytes_t));
        for (int i = 0; i < c->signers_length; i++)
          s[i] = bytes(c->signers + i * 20, 20);
        sb_add_bytes(sb, ",\"signers\":", s, c->signers_length, true);
      }
      if ((rc->flags & FLAGS_INCLUDE_CODE) && strcmp(d_get_stringk(request_token, K_METHOD), "eth_call") == 0)
        sb_add_chars(sb, ",\"includeCode\":true");
      if (proof == PROOF_FULL)
        sb_add_chars(sb, ",\"useFullProof\":true");
      if ((rc->flags & FLAGS_STATS) == 0)
        sb_add_chars(sb, ",\"noStats\":true");
      if ((rc->flags & FLAGS_BINARY))
        sb_add_chars(sb, ",\"useBinary\":true");

      // do we have verified hashes?
      if (chain->verified_hashes) {
        uint_fast16_t l = rc->max_verified_hashes;
        for (uint_fast16_t i = 0; i < l; i++) {
          if (!chain->verified_hashes[i].block_number) {
            l = i;
            break;
          }
        }
        if (l) {
          bytes_t* hashes = alloca(sizeof(bytes_t) * l);
          for (uint_fast16_t i = 0; i < l; i++) hashes[i] = bytes(chain->verified_hashes[i].hash, 32);
          sb_add_bytes(sb, ",\"verifiedHashes\":", hashes, l, true);
        }
      }

#ifdef PAY
      if (c->client->pay && c->client->pay->handle_request) {
        in3_ret_t ret = c->client->pay->handle_request(c, sb, rc, c->client->pay->cptr);
        if (ret != IN3_OK) return ret;
      }
#endif
      sb_add_range(sb, "}}", 0, 2);
    }
    else
      sb_add_char(sb, '}');
  }
  sb_add_char(sb, ']');
  return IN3_OK;
}
NONULL static void update_nodelist_cache(in3_ctx_t* ctx) {
  // we don't update weights for local chains.
  if (!in3_plugin_is_registered(ctx->client, PLGN_ACT_CACHE_SET) || ctx->client->chain_id == CHAIN_ID_LOCAL) return;
  in3_cache_store_nodelist(ctx->client, in3_get_chain(ctx->client));
}

NONULL static in3_ret_t ctx_parse_response(in3_ctx_t* ctx, char* response_data, int len) {
  assert_in3_ctx(ctx);
  assert(response_data);
  assert(len);

  ctx->response_context = (response_data[0] == '{' || response_data[0] == '[') ? parse_json(response_data) : parse_binary_str(response_data, len);

  if (!ctx->response_context)
    return ctx_set_error(ctx, "Error in JSON-response : ", ctx_set_error(ctx, str_remove_html(response_data), IN3_EINVALDT));

  if (d_type(ctx->response_context->result) == T_OBJECT) {
    // it is a single result
    ctx->responses    = _malloc(sizeof(d_token_t*));
    ctx->responses[0] = ctx->response_context->result;
    if (ctx->len != 1) return ctx_set_error(ctx, "The response must be an array!", IN3_EINVALDT);
  }
  else if (d_type(ctx->response_context->result) == T_ARRAY) {
    int        i;
    d_token_t* t = NULL;
    if (d_len(ctx->response_context->result) != (int) ctx->len)
      return ctx_set_error(ctx, "The responses must be a array with the same number as the requests!", IN3_EINVALDT);
    ctx->responses = _malloc(sizeof(d_token_t*) * ctx->len);
    for (i = 0, t = ctx->response_context->result + 1; i < (int) ctx->len; i++, t = d_next(t))
      ctx->responses[i] = t;
  }
  else
    return ctx_set_error(ctx, "The response must be a Object or Array", IN3_EINVALDT);

  return IN3_OK;
}

NONULL static void blacklist_node(in3_chain_t* chain, node_match_t* node_weight) {
  assert(chain);

  if (node_weight && !node_weight->blocked) {
    in3_node_weight_t* w = ctx_get_node_weight(chain, node_weight);
    if (!w) return;
    // blacklist the node
    uint64_t blacklisted_until_ = in3_time(NULL) + BLACKLISTTIME;
    if (w->blacklisted_until != blacklisted_until_)
      chain->dirty = true;
    w->blacklisted_until = blacklisted_until_;
    node_weight->blocked = true;
    in3_log_debug("Blacklisting node for unverifiable response: %s\n", ctx_get_node(chain, node_weight)->url);
  }
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

static void check_autoupdate(const in3_ctx_t* ctx, in3_chain_t* chain, d_token_t* response_in3, node_match_t* node) {
  assert_in3_ctx(ctx);
  assert(chain);
  if ((ctx->client->flags & FLAGS_AUTO_UPDATE_LIST) == 0) return;

  if (d_get_longk(response_in3, K_LAST_NODE_LIST) > d_get_longk(response_in3, K_CURRENT_BLOCK)) {
    // this shouldn't be possible, so we ignore this lastNodeList and do NOT try to update the nodeList
    return;
  }

  if (d_get_longk(response_in3, K_LAST_NODE_LIST) > chain->last_block) {
    if (chain->nodelist_upd8_params == NULL)
      chain->nodelist_upd8_params = _malloc(sizeof(*(chain->nodelist_upd8_params)));
    in3_node_t* n = ctx_get_node(chain, node);
    if (n) {
      // overwrite old params since we have a newer nodelist update now
      memcpy(chain->nodelist_upd8_params->node, n->address, 20);
      chain->nodelist_upd8_params->exp_last_block = d_get_longk(response_in3, K_LAST_NODE_LIST);
      chain->nodelist_upd8_params->timestamp      = in3_time(NULL) + update_waittime(d_get_longk(response_in3, K_LAST_NODE_LIST),
                                                                                d_get_longk(response_in3, K_CURRENT_BLOCK),
                                                                                ctx->client->replace_latest_block,
                                                                                chain->avg_block_time);
    }
  }

  if (chain->whitelist && d_get_longk(response_in3, K_LAST_WHITE_LIST) > chain->whitelist->last_block)
    chain->whitelist->needs_update = true;
}

static inline bool is_blacklisted(const node_match_t* node_weight) { return node_weight && node_weight->blocked; }

static bool is_user_error(d_token_t* error, char** err_msg) {
  *err_msg = d_type(error) == T_STRING ? d_string(error) : d_get_stringk(error, K_MESSAGE);
  // here we need to find a better way to detect user errors
  // currently we assume a error-message starting with 'Error:' is a server error and not a user error.
  return *err_msg && strncmp(*err_msg, "Error:", 6) && strncmp(*err_msg, "TypeError:", 10);
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

static in3_ret_t handle_error_response(in3_ctx_t* ctx, node_match_t* node, in3_response_t* response, in3_chain_t* chain) {
  assert_in3_ctx(ctx);
  assert_in3_response(response);
  if (is_blacklisted(node)) return IN3_ERPC;                                                        // already handled
  if (node) blacklist_node(chain, node);                                                            // we block this node
  ctx_set_error(ctx, response->data.len ? response->data.data : "no response from node", IN3_ERPC); // and copy the error to the ctx
  clear_response(response);                                                                         // free up memory
  return IN3_ERPC;
}

static void clean_up_ctx(in3_ctx_t* ctx, node_match_t* node, in3_chain_t* chain) {
  assert_in3_ctx(ctx);
  assert(chain);

  if (ctx->verification_state != IN3_OK && ctx->verification_state != IN3_WAITING) ctx->verification_state = IN3_WAITING;
  if (ctx->error) _free(ctx->error);
  if (ctx->responses) _free(ctx->responses);
  if (ctx->response_context) json_free(ctx->response_context);
  ctx->error           = NULL;
  in3_node_weight_t* w = node ? ctx_get_node_weight(chain, node) : NULL;
  if (w) w->blacklisted_until = 0; // we reset the blacklisted, because if the response was correct, no need to blacklist, otherwise we will set the blacklisted_until anyway
}

static in3_ret_t handle_payment(in3_ctx_t* ctx, node_match_t* node, int index) {
#ifdef PAY
  // we update the payment info from the in3-section
  if (ctx->client->pay && ctx->client->pay->follow_up) {
    in3_ret_t res = ctx->client->pay->follow_up(ctx, node, vc.proof, d_get(ctx->responses[index], K_ERROR), ctx->client->pay->cptr);
    if (res == IN3_WAITING && ctx->attempt < ctx->client->max_attempts - 1) {
      int nodes_count = ctx_nodes_len(ctx->node);
      // this means we need to retry with the same node
      ctx->attempt++;
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
      return res;
    }
    else if (res)
      return ctx_set_error(ctx, "Error following up the payment data", res);
  }
#else
  UNUSED_VAR(ctx);
  UNUSED_VAR(node);
  UNUSED_VAR(index);
#endif
  return IN3_OK;
}

static in3_ret_t verify_response(in3_ctx_t* ctx, in3_chain_t* chain, node_match_t* node, in3_response_t* response) {
  assert_in3_ctx(ctx);
  assert(chain);
  assert_in3_response(response);

  in3_ret_t res = IN3_OK;

  if (response->state || !response->data.len) // reponse has an error
    return handle_error_response(ctx, node, response, chain);

  // we need to clean up the previos responses if set
  clean_up_ctx(ctx, node, chain);

  // parse
  if (ctx_parse_response(ctx, response->data.data, response->data.len)) { // in case of an error we get a error-code and error is set in the ctx?
    if (node) blacklist_node(chain, node);                                // so we need to block the node.
    clear_response(response);                                             // we want to save memory and free the invalid response
    return ctx->verification_state;
  }

  // this was a internal response, so we don't need to verify the response
  if (!node) return (ctx->verification_state = IN3_OK);

  // check each request
  for (uint_fast16_t i = 0; i < ctx->len; i++) {

#ifdef LOGGING
    in3_node_t* n = node ? ctx_get_node(chain, node) : NULL;
#endif
    in3_vctx_t vc;
    vc.ctx     = ctx;
    vc.chain   = chain;
    vc.request = ctx->requests[i];
    vc.result  = d_get(ctx->responses[i], K_RESULT);
    vc.client  = ctx->client;
    vc.index   = (int) i;

    if ((vc.proof = d_get(ctx->responses[i], K_IN3))) { // vc.proof is temporary set to the in3-section. It will be updated to real proof in the next lines.
      if ((res = handle_payment(ctx, node, i))) return res;
      vc.last_validator_change = d_get_longk(vc.proof, K_LAST_VALIDATOR_CHANGE);
      vc.currentBlock          = d_get_longk(vc.proof, K_CURRENT_BLOCK);
      vc.proof                 = d_get(vc.proof, K_PROOF);
    }

    // no result?
    if (!vc.result) {
      char* err_msg;
      // if we don't have a result, the node reported an error
      if (is_user_error(d_get(ctx->responses[i], K_ERROR), &err_msg)) {
        if (node) node->blocked = true; // we mark it as blacklisted, but not blacklist it in the nodelist, since it was not the nodes fault.
        in3_log_debug("we have a user-error from %s, so we reject the response, but don't blacklist ..\n", n ? n->url : "intern");
        continue;
      }
      else {
        if (!node->blocked) in3_log_debug("we have a system-error from %s, so we block it ..\n", n ? n->url : "intern");
        blacklist_node(chain, node);
        return ctx_set_error(ctx, err_msg ? err_msg : "Invalid response", IN3_EINVAL);
      }
    }

    // verify the response
    res = ctx->verification_state = in3_plugin_execute_first(ctx, PLGN_ACT_RPC_VERIFY, &vc);

    // Waiting is ok, but we stop here
    if (res == IN3_WAITING)
      return res;

    // if this is an error, we blacklist the node and return the error.
    if (res) {
      // before we blacklist the node, we remove the data and replace it with the error-message
      // this is needed in case it will be cleared and we don't want to lose the error message
      if (ctx->error && response->data.data) {
        _free(response->data.data);
        int l           = strlen(ctx->error);
        response->state = res;
        response->data  = (sb_t){.data = _strdupn(ctx->error, l), .allocted = l + 1, .len = l};
      }
      blacklist_node(chain, node);
      return res;
    }

    // if it was ok, we continue to verify the other responses.
  }

  // all is ok
  return (ctx->verification_state = IN3_OK);
}

static void handle_times(in3_chain_t* chain, node_match_t* node, in3_response_t* response) {
  if (!node || node->blocked || !response || !response->time) return;
  in3_node_weight_t* w = ctx_get_node_weight(chain, node);
  if (!w) return;
  w->response_count++;
  w->total_response_time += response->time;
  response->time = 0; // make sure we count the time only once
}

static in3_ret_t find_valid_result(in3_ctx_t* ctx, int nodes_count, in3_response_t* response, in3_chain_t* chain) {
  node_match_t* node          = ctx->nodes;
  bool          still_pending = false;
  in3_ret_t     state         = IN3_ERPC;

  // blacklist nodes for missing response
  for (int n = 0; n < nodes_count; n++, node = node ? node->next : NULL) {

#ifdef LOGGING
    // get the connected node
    const in3_node_t* node_data = node ? ctx_get_node(chain, node) : NULL;
#endif

    // if the response is still pending, we skip...
    if (response[n].state == IN3_WAITING) {
      still_pending = true;
      in3_log_debug("request from node %s is still pending ..\n", node_data ? node_data->url : "intern");
      continue;
    }

    handle_times(chain, node, response + n);

    state = verify_response(ctx, chain, node, response + n);
    if (state == IN3_OK) {
      in3_log_debug(COLOR_GREEN "accepted response for %s from %s\n" COLOR_RESET, d_get_stringk(ctx->requests[0], K_METHOD), node_data ? node_data->url : "intern");
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

  // check auto update opts only if this node wasn't blacklisted (due to wrong result/proof)
  if (!is_blacklisted(node) && ctx->responses && d_get(ctx->responses[0], K_IN3) && !d_get(ctx->responses[0], K_ERROR))
    check_autoupdate(ctx, chain, d_get(ctx->responses[0], K_IN3), node);

  return IN3_OK;
}

NONULL static char* convert_to_http_url(char* src_url) {
  const int l = strlen(src_url);
  if (strncmp(src_url, "https://", 8) == 0) {
    char* url = _malloc(l);
    strcpy(url, src_url + 1);
    url[0] = 'h';
    url[2] = 't';
    url[3] = 'p';
    return url;
  }
  else
    return _strdupn(src_url, l);
}

NONULL in3_request_t* in3_create_request(in3_ctx_t* ctx) {
  switch (in3_ctx_state(ctx)) {
    case CTX_ERROR:
      ctx_set_error(ctx, "You cannot create an request if the was an error!", IN3_EINVAL);
      return NULL;
    case CTX_SUCCESS:
      return NULL;
    case CTX_WAITING_FOR_RESPONSE:
      ctx_set_error(ctx, "There are pending requests, finish them before creating a new one!", IN3_EINVAL);
      return NULL;
    case CTX_WAITING_TO_SEND: {
      in3_ctx_t* p = ctx;
      for (; p; p = p->required) {
        if (!p->raw_response) ctx = p;
      }
    }
  }

  in3_ret_t     res;
  char*         rpc         = d_get_stringk(d_get(ctx->requests[0], K_IN3), K_RPC);
  int           nodes_count = rpc ? 1 : ctx_nodes_len(ctx->nodes);
  char**        urls        = nodes_count ? _malloc(sizeof(char*) * nodes_count) : NULL;
  node_match_t* node        = ctx->nodes;
  in3_chain_t*  chain       = in3_get_chain(ctx->client);
  bool          multichain  = false;

  for (int n = 0; n < nodes_count; n++) {
    in3_node_t* node_data = rpc ? NULL : ctx_get_node(chain, node);
    urls[n]               = rpc ? rpc : node_data->url;

    // cif we use_http, we need to malloc a new string, so we also need to free it later!
    if (ctx->client->flags & FLAGS_HTTP) urls[n] = convert_to_http_url(urls[n]);

    // this is all we need to do if we have a rpc-node
    if (rpc) break;

    // if the multichain-prop is set we need to specify the chain_id in the request
    if (in3_node_props_get(node_data->props, NODE_PROP_MULTICHAIN)) multichain = true;

    node = node->next;
  }

  // prepare the payload
  sb_t* payload = sb_new(NULL);
  res           = ctx_create_payload(ctx, payload, multichain, rpc != NULL);
  if (res < 0) {
    // we clean up
    sb_free(payload);
    free_urls(urls, nodes_count, ctx->client->flags & FLAGS_HTTP);
    // since we cannot return an error, we set the error in the context and return NULL, indicating the error.
    ctx_set_error(ctx, "could not generate the payload", res);
    return NULL;
  }

  // prepare response-object
  in3_request_t* request = _calloc(sizeof(in3_request_t), 1);
  request->ctx           = ctx;
  request->payload       = payload->data;
  request->urls_len      = nodes_count;
  request->urls          = urls;
  request->cptr          = NULL;
  request->wait          = d_get_intk(d_get(ctx->requests[0], K_IN3), K_WAIT);

  if (!nodes_count) nodes_count = 1; // at least one result, because for internal response we don't need nodes, but a result big enough.
  ctx->raw_response = _calloc(sizeof(in3_response_t), nodes_count);
  for (int n = 0; n < nodes_count; n++) ctx->raw_response[n].state = IN3_WAITING;

  // we only clean up the the stringbuffer, but keep the content (payload->data)
  _free(payload);

  return request;
}

NONULL void request_free(in3_request_t* req) {
  // free resources
  free_urls(req->urls, req->urls_len, req->ctx->client->flags & FLAGS_HTTP);
  _free(req->payload);
  _free(req);
}

NONULL static bool ctx_is_allowed_to_fail(in3_ctx_t* ctx) {
  return ctx_is_method(ctx, "in3_nodeList");
}

NONULL in3_ret_t ctx_handle_failable(in3_ctx_t* ctx) {
  in3_ret_t res = IN3_OK;

  // blacklist node that gave us an error response for nodelist (if not first update)
  // and clear nodelist params
  in3_chain_t* chain = in3_get_chain(ctx->client);

  if (nodelist_not_first_upd8(chain))
    blacklist_node_addr(chain, chain->nodelist_upd8_params->node, BLACKLISTTIME);
  _free(chain->nodelist_upd8_params);
  chain->nodelist_upd8_params = NULL;

  if (ctx->required) {
    // if first update return error otherwise return IN3_OK, this is because first update is
    // always from a boot node which is presumed to be trusted
    if (nodelist_first_upd8(chain))
      res = ctx_set_error(ctx, ctx->required->error ? ctx->required->error : "error handling subrequest", IN3_ERPC);

    if (res == IN3_OK) res = ctx_remove_required(ctx, ctx->required, true);
  }

  return res;
}
in3_ctx_t* in3_ctx_last_waiting(in3_ctx_t* ctx) {
  in3_ctx_t* last = ctx;
  for (; ctx; ctx = ctx->required) {
    if (!ctx->response_context) last = ctx;
  }
  return last;
}

static void init_sign_ctx(in3_ctx_t* ctx, in3_sign_ctx_t* sign_ctx) {
  d_token_t* params = d_get(ctx->requests[0], K_PARAMS);
  sign_ctx->message = d_to_bytes(d_get_at(params, 0));
  sign_ctx->account = d_to_bytes(d_get_at(params, 1));
  sign_ctx->type    = SIGN_EC_HASH;
  sign_ctx->ctx     = ctx;
}

in3_sign_ctx_t* create_sign_ctx(in3_ctx_t* ctx) {
  in3_sign_ctx_t* res = _malloc(sizeof(in3_sign_ctx_t));
  init_sign_ctx(ctx, res);
  return res;
}

in3_ret_t in3_handle_sign(in3_ctx_t* ctx) {
  in3_sign_ctx_t sign_ctx;
  init_sign_ctx(ctx, &sign_ctx);
  if (!sign_ctx.message.data) return ctx_set_error(ctx, "missing data to sign", IN3_ECONFIG);
  if (!sign_ctx.account.data) return ctx_set_error(ctx, "missing account to sign", IN3_ECONFIG);

  ctx->raw_response = _calloc(sizeof(in3_response_t), 1);
  sb_init(&ctx->raw_response[0].data);
  in3_log_trace("... request to sign ");
  TRY(in3_plugin_execute_first(ctx, PLGN_ACT_SIGN, &sign_ctx))
  sb_add_range(&ctx->raw_response->data, (char*) sign_ctx.signature, 0, 65);
  return IN3_OK;
}

typedef struct {
  in3_ctx_t* ctx;
  void*      ptr;
} ctx_req_t;
typedef struct {
  int        len;
  ctx_req_t* req;
} ctx_req_transports_t;

static void transport_cleanup(in3_ctx_t* ctx, ctx_req_transports_t* transports, bool free_all) {
  for (int i = 0; i < transports->len; i++) {
    if (free_all || transports->req[i].ctx == ctx) {
      in3_request_t req = {.ctx = ctx, .cptr = transports->req[i].ptr, .urls_len = 0, .urls = NULL, .payload = NULL};
      in3_plugin_execute_first_or_none(ctx, PLGN_ACT_TRANSPORT_CLEAN, &req);
      if (!free_all) {
        transports->req[i].ctx = NULL;
        return;
      }
    }
  }
  if (free_all && transports->req) _free(transports->req);
}

static void in3_handle_rpc_next(in3_ctx_t* ctx, ctx_req_transports_t* transports) {
  in3_log_debug("waiting for the next respone ...\n");
  ctx = in3_ctx_last_waiting(ctx);
  for (int i = 0; i < transports->len; i++) {
    if (transports->req[i].ctx == ctx) {
      in3_request_t req = {.ctx = ctx, .cptr = transports->req[i].ptr, .urls_len = 0, .urls = NULL, .payload = NULL};
      in3_plugin_execute_first(ctx, PLGN_ACT_TRANSPORT_RECEIVE, &req);
#ifdef DEBUG
      const in3_chain_t* chain = in3_get_chain(ctx->client);
      node_match_t*      w     = ctx->nodes;
      int                i     = 0;
      for (; w; i++, w = w->next) {
        if (ctx->raw_response[i].state != IN3_WAITING && ctx->raw_response[i].data.data && ctx->raw_response[i].time) {
          in3_node_t* node = ctx_get_node(chain, w);
          char*       data = ctx->raw_response[i].data.data;
          data             = format_json(data);

          in3_log_trace(ctx->raw_response[i].state
                            ? "... response(%s): \n... " COLOR_RED_STR "\n"
                            : "... response(%s): \n... " COLOR_GREEN_STR "\n",
                        node ? node->url : "intern", data);
          _free(data);
        }
      }
#endif
      return;
    }
  }

  ctx_set_error(ctx, "waiting to fetch more responses, but no cptr was registered", IN3_ENOTSUP);
}

void in3_handle_rpc(in3_ctx_t* ctx, ctx_req_transports_t* transports) {
  // if we can't create the request, this function will put it into error-state
  in3_request_t* request = in3_create_request(ctx);
  if (!request) return;

  // do we need to wait?
  if (request->wait)
    in3_sleep(request->wait);
  // in case there is still a old cptr we need to cleanup since this means this is a retry!
  transport_cleanup(ctx, transports, false);

  // debug output
  for (unsigned int i = 0; i < request->urls_len; i++)
    in3_log_trace("... request to " COLOR_YELLOW_STR "\n... " COLOR_MAGENTA_STR "\n", request->urls[i], i == 0 ? request->payload : "");

  // handle it
  in3_plugin_execute_first(ctx, PLGN_ACT_TRANSPORT_SEND, request);

  // debug output
  node_match_t*      node  = request->ctx->nodes;
  const in3_chain_t* chain = in3_get_chain(ctx->client);

  for (unsigned int i = 0; i < request->urls_len; i++, node = node ? node->next : NULL) {
    if (request->ctx->raw_response[i].state != IN3_WAITING) {
      char*             data      = request->ctx->raw_response[i].data.data;
      const in3_node_t* node_data = node ? ctx_get_node(chain, node) : NULL;
#ifdef DEBUG
      data = format_json(data);
#endif
      in3_log_trace(request->ctx->raw_response[i].state
                        ? "... response(%s): \n... " COLOR_RED_STR "\n"
                        : "... response(%s): \n... " COLOR_GREEN_STR "\n",
                    node_data ? node_data->url : "intern", data);
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
      if (!transports->req[i].ctx) {
        index = i;
        break;
      }
    }
    if (index == -1) {
      transports->req = transports->len ? _realloc(transports->req, sizeof(ctx_req_t) * (transports->len + 1), sizeof(ctx_req_t) * transports->len) : _malloc(sizeof(ctx_req_t));
      index           = transports->len++;
    }

    // store the pointers
    transports->req[index].ctx = request->ctx;
    transports->req[index].ptr = request->cptr;
  }

  // we will cleanup even though the reponses may still be pending
  request_free(request);
}

in3_ret_t in3_send_ctx(in3_ctx_t* ctx) {
  ctx_req_transports_t transports = {0};
  while (true) {
    switch (in3_ctx_exec_state(ctx)) {
      case CTX_ERROR:
      case CTX_SUCCESS:
        transport_cleanup(ctx, &transports, true);
        return ctx->verification_state;
      case CTX_WAITING_FOR_RESPONSE:
        in3_handle_rpc_next(ctx, &transports);
        break;
      case CTX_WAITING_TO_SEND: {
        in3_ctx_t* last = in3_ctx_last_waiting(ctx);
        switch (last->type) {
          case CT_SIGN:
            in3_handle_sign(last);
            break;
          case CT_RPC:
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
    in3_ctx_t*      ctx,
    in3_sign_ctx_t* sign_ctx) {
  ctx->raw_response = _calloc(sizeof(in3_response_t), 1);
  sb_init(&ctx->raw_response[0].data);
  sb_add_range(&ctx->raw_response->data, (char*) sign_ctx->signature, 0, 65);
}

in3_ctx_t* ctx_find_required(const in3_ctx_t* parent, const char* search_method) {
  in3_ctx_t* sub_ctx = parent->required;
  while (sub_ctx) {
    if (!sub_ctx->requests) continue;
    if (ctx_is_method(sub_ctx, search_method)) return sub_ctx;
    sub_ctx = sub_ctx->required;
  }
  return NULL;
}

in3_ret_t ctx_add_required(in3_ctx_t* parent, in3_ctx_t* ctx) {
  //  printf(" ++ add required %s > %s\n", ctx_name(parent), ctx_name(ctx));
  ctx->required    = parent->required;
  parent->required = ctx;
  return in3_ctx_execute(ctx);
}

in3_ret_t ctx_remove_required(in3_ctx_t* parent, in3_ctx_t* ctx, bool rec) {
  if (!ctx) return IN3_OK;
  in3_ctx_t* p = parent;
  while (p) {
    if (p->required == ctx) {
      //      printf(" -- remove required %s > %s\n", ctx_name(parent), ctx_name(ctx));
      in3_ctx_t* next = rec ? NULL : ctx->required;
      if (!rec) ctx->required = NULL;
      ctx_free_intern(ctx, true);
      p->required = next;
      return IN3_OK;
    }
    p = p->required;
  }
  return IN3_EFIND;
}

in3_ctx_state_t in3_ctx_state(in3_ctx_t* ctx) {
  if (ctx == NULL) return CTX_SUCCESS;
  in3_ctx_state_t required_state = ctx->required ? in3_ctx_state(ctx->required) : CTX_SUCCESS;
  if (required_state == CTX_ERROR || ctx->error) return CTX_ERROR;
  if (ctx->required && required_state != CTX_SUCCESS) return required_state;
  if (!ctx->raw_response) return CTX_WAITING_TO_SEND;
  if (ctx->type == CT_RPC && !ctx->response_context) return CTX_WAITING_FOR_RESPONSE;
  if (ctx->type == CT_SIGN && ctx->raw_response->state == IN3_WAITING) return CTX_WAITING_FOR_RESPONSE;
  return CTX_SUCCESS;
}

void ctx_free(in3_ctx_t* ctx) {
  if (ctx) ctx_free_intern(ctx, false);
}

static inline in3_ret_t handle_internally(in3_ctx_t* ctx) {
  if (ctx->len != 1) return IN3_OK; //  currently we do not support bulk requests forr internal calls
  in3_rpc_handle_ctx_t vctx = {.ctx = ctx, .response = &ctx->raw_response, .request = ctx->requests[0]};
  in3_ret_t            res  = in3_plugin_execute_first_or_none(ctx, PLGN_ACT_RPC_HANDLE, &vctx);
  return res == IN3_EIGNORE ? IN3_OK : res;
}

in3_ctx_state_t in3_ctx_exec_state(in3_ctx_t* ctx) {
  in3_ctx_execute(ctx);
  return in3_ctx_state(ctx);
}

in3_ret_t in3_ctx_execute(in3_ctx_t* ctx) {
  in3_ret_t ret = IN3_OK;

  // if there is an error it does not make sense to execute.
  if (ctx->error) return (ctx->verification_state && ctx->verification_state != IN3_WAITING) ? ctx->verification_state : IN3_EUNKNOWN;

  // is it a valid request?
  if (!ctx->request_context || d_type(d_get(ctx->requests[0], K_METHOD)) != T_STRING) return ctx_set_error(ctx, "No Method defined", IN3_ECONFIG);

  // if there is response we are done.
  if (ctx->response_context && ctx->verification_state == IN3_OK) return IN3_OK;

  // if we have required-contextes, we need to check them first
  if (ctx->required && (ret = in3_ctx_execute(ctx->required))) {
    if (ret == IN3_EIGNORE)
      ctx_handle_failable(ctx);
    else
      return ctx_set_error(ctx, ctx->required->error ? ctx->required->error : "error handling subrequest", ret);
  }

  in3_log_debug("ctx_execute %s ... attempt %i\n", d_get_stringk(ctx->requests[0], K_METHOD), ctx->attempt + 1);

  switch (ctx->type) {
    case CT_RPC: {

      // check chain_id
      in3_chain_t* chain = in3_get_chain(ctx->client);
      if (!chain) return ctx_set_error(ctx, "chain not found", IN3_EFIND);

      // do we need to handle it internaly?
      if (!ctx->raw_response && !ctx->response_context && (ret = handle_internally(ctx)) < 0)
        return ctx_set_error(ctx, "The request could not be handled", ret);

      // if we don't have a nodelist, we try to get it.
      if (!ctx->raw_response && !ctx->nodes && !d_get(d_get(ctx->requests[0], K_IN3), K_RPC)) {
        in3_node_filter_t filter = NODE_FILTER_INIT;
        filter.nodes             = d_get(d_get(ctx->requests[0], K_IN3), K_DATA_NODES);
        filter.props             = (ctx->client->node_props & 0xFFFFFFFF) | NODE_PROP_DATA | ((ctx->client->flags & FLAGS_HTTP) ? NODE_PROP_HTTP : 0) | (in3_ctx_get_proof(ctx, 0) != PROOF_NONE ? NODE_PROP_PROOF : 0);
        if ((ret = in3_node_list_pick_nodes(ctx, &ctx->nodes, ctx->client->request_count, filter)) == IN3_OK) {
          if ((ret = pick_signers(ctx, ctx->requests[0])) < 0)
            return ctx_set_error(ctx, "error configuring the config for request", ret < 0 && ret != IN3_WAITING && ctx_is_allowed_to_fail(ctx) ? IN3_EIGNORE : ret);

#ifdef PAY
          // now we have the nodes, we can prepare the payment
          if (ctx->client->pay && ctx->client->pay->prepare && (ret = ctx->client->pay->prepare(ctx, ctx->client->pay->cptr)) != IN3_OK) return ret;
#endif
        }
        else
          // since we could not get the nodes, we either report it as error or wait.
          return ctx_set_error(ctx, "could not find any node", ret < 0 && ret != IN3_WAITING && ctx_is_allowed_to_fail(ctx) ? IN3_EIGNORE : ret);
      }

      // if we still don't have an response, we keep on waiting
      if (!ctx->raw_response) return IN3_WAITING;

      // ok, we have a response, then we try to evaluate the responses
      // verify responses and return the node with the correct result.
      ret = find_valid_result(ctx, ctx->nodes == NULL ? 1 : ctx_nodes_len(ctx->nodes), ctx->raw_response, chain);

      // update weights in the cache
      update_nodelist_cache(ctx);

      // we wait or are have successfully verified the response
      if (ret == IN3_WAITING || ret == IN3_OK) return ret;

      // if not, then we clean up
      response_free(ctx);

      // we count this is an attempt
      ctx->attempt++;

      // should we retry?
      if (ctx->attempt < ctx->client->max_attempts) {
        in3_log_debug("Retrying send request...\n");
        // reset the error and try again
        if (ctx->error) _free(ctx->error);
        ctx->error              = NULL;
        ctx->verification_state = IN3_WAITING;
        // now try again, which should end in waiting for the next request.
        return in3_ctx_execute(ctx);
      }
      else {
        if (ctx_is_allowed_to_fail(ctx))
          ctx->verification_state = ret = IN3_EIGNORE;
        // we give up
        return ctx->error ? (ret ? ret : IN3_ERPC) : ctx_set_error(ctx, "reaching max_attempts and giving up", IN3_ELIMIT);
      }
    }

    case CT_SIGN: {
      if (!ctx->raw_response || ctx->raw_response->state == IN3_WAITING)
        return IN3_WAITING;
      else if (ctx->raw_response->state)
        return IN3_ERPC;
      return IN3_OK;
    }
    default:
      return IN3_EINVAL;
  }
}
