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
#include "verifier.h"
#include <stdint.h>
#include <string.h>
#include <time.h>

#define WAIT_TIME_CAP 3600
#define BLACKLISTTIME 24 * 3600

NONULL static void response_free(in3_ctx_t* ctx) {
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
}

NONULL static void free_ctx_intern(in3_ctx_t* ctx, bool is_sub) {
  // only for intern requests, we actually free the original request-string
  if (is_sub)
    _free(ctx->request_context->c);
  else
    ctx->client->pending--;
  if (ctx->error) _free(ctx->error);
  response_free(ctx);
  if (ctx->request_context)
    json_free(ctx->request_context);

  if (ctx->requests) _free(ctx->requests);
  if (ctx->cache) in3_cache_free(ctx->cache);
  if (ctx->required) free_ctx_intern(ctx->required, true);

  _free(ctx);
}

NONULL static bool auto_ask_sig(const in3_ctx_t* ctx) {
  return (ctx_is_method(ctx, "in3_nodeList") && !(ctx->client->flags & FLAGS_NODE_LIST_NO_SIG) && ctx->client->chain_id != ETH_CHAIN_ID_BTC);
}

NONULL static in3_ret_t pick_signers(in3_ctx_t* ctx, d_token_t* request) {

  const in3_t* c = ctx->client;

  if (in3_ctx_get_proof(ctx) == PROOF_NONE && !auto_ask_sig(ctx))
    return IN3_OK;

  // For nodeList request, we always ask for proof & atleast one signature
  uint8_t total_sig_cnt = c->signature_count ? c->signature_count : auto_ask_sig(ctx) ? 1 : 0;

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
    ctx->signers          = _malloc(sizeof(bytes_t) * node_count);
    const node_match_t* w = signer_nodes;
    for (int i = 0; i < node_count; i++) {
      ctx->signers[i].len  = w->node->address->len;
      ctx->signers[i].data = w->node->address->data;
      w                    = w->next;
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

NONULL static in3_ret_t ctx_create_payload(in3_ctx_t* c, sb_t* sb, bool multichain) {
  static unsigned long rpc_id_counter = 1;
  char                 temp[100];
  in3_t*               rc       = c->client;
  struct SHA3_CTX*     msg_hash = rc->key ? alloca(sizeof(struct SHA3_CTX)) : NULL;
  in3_proof_t          proof    = in3_ctx_get_proof(c);

  sb_add_char(sb, '[');

  for (uint_fast16_t i = 0; i < c->len; i++) {
    d_token_t *request_token = c->requests[i], *t;
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
      const in3_chain_t* chain = in3_find_chain(rc, c->client->chain_id);
      if (chain->whitelist) {
        const bytes_t adr = bytes(chain->whitelist->contract, 20);
        sb_add_bytes(sb, ",\"whiteListContract\":", &adr, 1, false);
      }
      if (msg_hash) {
        uint8_t sig[65], hash[32];
        bytes_t sig_bytes = bytes(sig, 65);
        keccak_Final(msg_hash, hash);
        if (ecdsa_sign_digest(&secp256k1, c->client->key, hash, sig, sig + 64, NULL) < 0)
          return ctx_set_error(c, "could not sign the request", IN3_EINVAL);
        sb_add_bytes(sb, ",\"sig\":", &sig_bytes, 1, false);
      }
      if (rc->finality)
        sb_add_range(sb, temp, 0, sprintf(temp, ",\"finality\":%i", rc->finality));
      if (rc->replace_latest_block)
        sb_add_range(sb, temp, 0, sprintf(temp, ",\"latestBlock\":%i", rc->replace_latest_block));
      if (c->signers_length)
        sb_add_bytes(sb, ",\"signers\":", c->signers, c->signers_length, true);
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
    } else
      sb_add_char(sb, '}');
  }
  sb_add_char(sb, ']');
  return IN3_OK;
}
NONULL static void update_nodelist_cache(in3_ctx_t* ctx) {
  // we don't update weights for local chains.
  if (!ctx->client->cache || ctx->client->chain_id == ETH_CHAIN_ID_LOCAL) return;
  chain_id_t chain_id = ctx->client->chain_id;
  in3_cache_store_nodelist(ctx->client, in3_find_chain(ctx->client, chain_id));
}

NONULL static in3_ret_t ctx_parse_response(in3_ctx_t* ctx, char* response_data, int len) {

  d_track_keynames(1);
  ctx->response_context = (response_data[0] == '{' || response_data[0] == '[') ? parse_json(response_data) : parse_binary_str(response_data, len);
  d_track_keynames(0);
  if (!ctx->response_context)
    return ctx_set_error(ctx, "Error in JSON-response : ", ctx_set_error(ctx, str_remove_html(response_data), IN3_EINVALDT));

  if (d_type(ctx->response_context->result) == T_OBJECT) {
    // it is a single result
    ctx->responses    = _malloc(sizeof(d_token_t*));
    ctx->responses[0] = ctx->response_context->result;
    if (ctx->len != 1) return ctx_set_error(ctx, "The response must be a single object!", IN3_EINVALDT);
  } else if (d_type(ctx->response_context->result) == T_ARRAY) {
    int        i;
    d_token_t* t = NULL;
    if (d_len(ctx->response_context->result) != (int) ctx->len)
      return ctx_set_error(ctx, "The responses must be a array with the same number as the requests!", IN3_EINVALDT);
    ctx->responses = _malloc(sizeof(d_token_t*) * ctx->len);
    for (i = 0, t = ctx->response_context->result + 1; i < (int) ctx->len; i++, t = d_next(t))
      ctx->responses[i] = t;
  } else
    return ctx_set_error(ctx, "The response must be a Object or Array", IN3_EINVALDT);

  return IN3_OK;
}

NONULL static void blacklist_node(node_match_t* node_weight) {
  if (node_weight && node_weight->weight) {
    // blacklist the node
    node_weight->weight->blacklisted_until = in3_time(NULL) + BLACKLISTTIME;
    node_weight->weight                    = NULL; // setting the weight to NULL means we reject the response.
    in3_log_debug("Blacklisting node for unverifiable response: %s\n", node_weight->node->url);
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
  if ((ctx->client->flags & FLAGS_AUTO_UPDATE_LIST) == 0) return;

  if (d_get_longk(response_in3, K_LAST_NODE_LIST) > d_get_longk(response_in3, K_CURRENT_BLOCK)) {
    // this shouldn't be possible, so we ignore this lastNodeList and do NOT try to update the nodeList
    return;
  }

  if (d_get_longk(response_in3, K_LAST_NODE_LIST) > chain->last_block) {
    if (chain->nodelist_upd8_params == NULL)
      chain->nodelist_upd8_params = _malloc(sizeof(*(chain->nodelist_upd8_params)));

    // overwrite old params since we have a newer nodelist update now
    memcpy(chain->nodelist_upd8_params->node, node->node->address->data, node->node->address->len);
    chain->nodelist_upd8_params->exp_last_block = d_get_longk(response_in3, K_LAST_NODE_LIST);
    chain->nodelist_upd8_params->timestamp      = in3_time(NULL) + update_waittime(d_get_longk(response_in3, K_LAST_NODE_LIST),
                                                                              d_get_longk(response_in3, K_CURRENT_BLOCK),
                                                                              ctx->client->replace_latest_block,
                                                                              chain->avg_block_time);
  }

  if (chain->whitelist && d_get_longk(response_in3, K_LAST_WHITE_LIST) > chain->whitelist->last_block)
    chain->whitelist->needs_update = true;
}

static inline bool is_blacklisted(const node_match_t* node_weight) { return node_weight && node_weight->weight == NULL; }

static bool is_user_error(d_token_t* error) {
  char* err_msg = d_type(error) == T_STRING ? d_string(error) : d_get_stringk(error, K_MESSAGE);
  // here we need to find a better way to detect user errors
  // currently we assume a error-message starting with 'Error:' is a server error and not a user error.
  return err_msg && strncmp(err_msg, "Error:", 6) != 0;
}

static in3_ret_t find_valid_result(in3_ctx_t* ctx, int nodes_count, in3_response_t* response, in3_chain_t* chain, in3_verifier_t* verifier) {
  node_match_t* node = ctx->nodes;

  // find the verifier
  in3_vctx_t vc;
  vc.ctx             = ctx;
  vc.chain           = chain;
  bool still_pending = false;

  // blacklist nodes for missing response
  for (int n = 0; n < nodes_count; n++, node = node ? node->next : NULL) {

    // if the response is still pending, we skip...
    if (response[n].state == IN3_WAITING) {
      still_pending = true;
      continue;
    }

    // handle times
    if (node && node->weight && ctx->raw_response && ctx->raw_response[n].time) {
      node->weight->response_count++;
      node->weight->total_response_time += ctx->raw_response[n].time;
      ctx->raw_response[n].time = 0; // make sure we count the time only once
    }

    // since nodes_count was detected before, this should not happen!
    if (response[n].state) {
      if (is_blacklisted(node))
        continue;
      else if (node)
        blacklist_node(node);
      ctx_set_error(ctx, response[n].data.len ? response[n].data.data : "no response from node", IN3_ERPC);
      if (response[n].data.data) {
        // clean up invalid data
        _free(response[n].data.data);
        response[n].data.data     = NULL;
        response[n].data.allocted = 0;
        response[n].data.len      = 0;
      }
    } else {
      // we need to clean up the previos responses if set
      if (ctx->error) _free(ctx->error);
      if (ctx->responses) _free(ctx->responses);
      if (ctx->response_context) json_free(ctx->response_context);
      ctx->error = NULL;

      if (node && node->weight) node->weight->blacklisted_until = 0;                        // we reset the blacklisted, because if the response was correct, no need to blacklist, otherwise we will set the blacklisted_until anyway
      in3_ret_t res = ctx_parse_response(ctx, response[n].data.data, response[n].data.len); // parse the result
      if (res < 0) {
        if (node) blacklist_node(node);
      } else {
        // check each request
        for (uint_fast16_t i = 0; i < ctx->len; i++) {
          vc.request = ctx->requests[i];
          vc.result  = d_get(ctx->responses[i], K_RESULT);
          vc.client  = ctx->client;

          if ((vc.proof = d_get(ctx->responses[i], K_IN3))) {
            // vc.proof is temporary set to the in3-section. It will be updated to real proof in the next lines.
#ifdef PAY
            // we update the payment info from the in3-section
            if (ctx->client->pay && ctx->client->pay->follow_up) {
              res = ctx->client->pay->follow_up(ctx, node, vc.proof, d_get(ctx->responses[i], K_ERROR), ctx->client->pay->cptr);
              if (res == IN3_WAITING && ctx->attempt < ctx->client->max_attempts - 1) {
                // this means we need to retry with the same node
                ctx->attempt++;
                for (int i = 0; i < nodes_count; i++) {
                  _free(ctx->raw_response[i].error.data);
                  _free(ctx->raw_response[i].result.data);
                }
                _free(ctx->raw_response);
                _free(ctx->responses);
                json_free(ctx->response_context);

                ctx->raw_response     = NULL;
                ctx->response_context = NULL;
                ctx->responses        = NULL;
                return res;

              } else if (res)
                return ctx_set_error(ctx, "Error following up the payment data", (ctx->verification_state = res));
            }
#endif
            vc.last_validator_change = d_get_longk(vc.proof, K_LAST_VALIDATOR_CHANGE);
            vc.currentBlock          = d_get_longk(vc.proof, K_CURRENT_BLOCK);
            vc.proof                 = d_get(vc.proof, K_PROOF);
          }

          if (!vc.result && ctx->attempt < ctx->client->max_attempts - 1) {
            // if we don't have a result, the node reported an error
            // since we don't know if this error is our fault or the server fault,we don't blacklist the node, but retry
            ctx->verification_state = IN3_ERPC;
            if (is_user_error(d_get(ctx->responses[i], K_ERROR)))
              node->weight = NULL; // we mark it as blacklisted, but not blacklist it in the nodelist, since it was not the nodes fault.
            else
              blacklist_node(node);
            break;
          } else if (verifier) {
            res = ctx->verification_state = verifier->verify(&vc);
            if (res == IN3_WAITING)
              return res;
            else if (res < 0) {
              blacklist_node(node);
              break;
            }
          } else
            // no verifier - nothing to verify
            ctx->verification_state = IN3_OK;
        }
      }
    }

    // check auto update opts only if this node wasn't blacklisted (due to wrong result/proof)
    if (!is_blacklisted(node) && ctx->responses && d_get(ctx->responses[0], K_IN3) && !d_get(ctx->responses[0], K_ERROR))
      check_autoupdate(ctx, chain, d_get(ctx->responses[0], K_IN3), node);

    // !node_weight is valid, because it means this is a internaly handled response
    if (!node || !is_blacklisted(node))
      return IN3_OK; // this reponse was successfully verified, so let us keep it.
  }
  // no valid response found,
  // if pending, we remove the error and wait
  if (still_pending) {
    if (ctx->error) _free(ctx->error);
    ctx->error              = NULL;
    ctx->verification_state = IN3_WAITING;
    return IN3_WAITING;
  }

  return IN3_EINVAL;
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
  } else
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
  int           nodes_count = ctx_nodes_len(ctx->nodes);
  char**        urls        = nodes_count ? _malloc(sizeof(char*) * nodes_count) : NULL;
  node_match_t* node        = ctx->nodes;
  bool          multichain  = false;

  for (int n = 0; n < nodes_count; n++) {
    urls[n] = node->node->url;

    // if the multichain-prop is set we need to specify the chain_id in the request
    if (in3_node_props_get(node->node->props, NODE_PROP_MULTICHAIN)) multichain = true;

    // cif we use_http, we need to malloc a new string, so we also need to free it later!
    if (ctx->client->flags & FLAGS_HTTP) urls[n] = convert_to_http_url(urls[n]);

    node = node->next;
  }

  // prepare the payload
  sb_t* payload = sb_new(NULL);
  res           = ctx_create_payload(ctx, payload, multichain);
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
  request->action        = REQ_ACTION_SEND;
  request->cptr          = NULL;

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
  in3_chain_t* chain = in3_find_chain(ctx->client, ctx->client->chain_id);

  if (nodelist_not_first_upd8(chain))
    blacklist_node_addr(chain, chain->nodelist_upd8_params->node, BLACKLISTTIME);
  _free(chain->nodelist_upd8_params);
  chain->nodelist_upd8_params = NULL;

  if (ctx->required) {
    // if first update return error otherwise return IN3_OK, this is because first update is
    // always from a boot node which is presumed to be trusted
    if (nodelist_first_upd8(chain))
      res = ctx_set_error(ctx, ctx->required->error ? ctx->required->error : "error handling subrequest", IN3_ERPC);

    if (res == IN3_OK) res = ctx_remove_required(ctx, ctx->required);
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

in3_ret_t in3_handle_sign(in3_ctx_t* ctx) {
  if (ctx->client->signer) {
    d_token_t*     params = d_get(ctx->requests[0], K_PARAMS);
    in3_sign_ctx_t sign_ctx;
    sign_ctx.message = d_to_bytes(d_get_at(params, 0));
    sign_ctx.account = d_to_bytes(d_get_at(params, 1));
    sign_ctx.type    = SIGN_EC_HASH;
    sign_ctx.ctx     = ctx;
    sign_ctx.wallet  = ctx->client->signer->wallet;
    if (!sign_ctx.message.data) return ctx_set_error(ctx, "missing data to sign", IN3_ECONFIG);
    if (!sign_ctx.account.data) return ctx_set_error(ctx, "missing account to sign", IN3_ECONFIG);

    ctx->raw_response = _calloc(sizeof(in3_response_t), 1);
    sb_init(&ctx->raw_response[0].data);
    in3_log_trace("... request to sign ");
    in3_ret_t res = ctx->client->signer->sign(&sign_ctx);
    if (res < 0) return ctx_set_error(ctx, ctx->raw_response->data.data, res);
    sb_add_range(&ctx->raw_response->data, (char*) sign_ctx.signature, 0, 65);
    return IN3_OK;
  } else
    return ctx_set_error(ctx, "no signer set", IN3_ECONFIG);
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
      in3_request_t req = {.action = REQ_ACTION_CLEANUP, .ctx = ctx, .cptr = transports->req[i].ptr, .urls_len = 0, .urls = NULL, .payload = NULL};
      ctx->client->transport(&req);
      if (!free_all) {
        transports->req[i].ctx = NULL;
        return;
      }
    }
  }
  if (free_all && transports->req) _free(transports->req);
}

static void in3_handle_rpc_next(in3_ctx_t* ctx, ctx_req_transports_t* transports) {
  in3_log_debug("waiting for the next response....");
  ctx = in3_ctx_last_waiting(ctx);
  for (int i = 0; i < transports->len; i++) {
    if (transports->req[i].ctx == ctx) {
      in3_request_t req = {.action = REQ_ACTION_RECEIVE, .ctx = ctx, .cptr = transports->req[i].ptr, .urls_len = 0, .urls = NULL, .payload = NULL};
      ctx->client->transport(&req);
#ifdef DEBUG
      node_match_t* w = ctx->nodes;
      int           i = 0;
      for (; w; i++, w = w->next) {
        if (ctx->raw_response[i].state != IN3_WAITING && ctx->raw_response[i].data.data)
          in3_log_trace(ctx->raw_response[i].state
                            ? "... response(%i): \n... " COLOR_RED_STR "\n"
                            : "... response(%i): \n... " COLOR_GREEN_STR "\n",
                        i, ctx->raw_response[i].data.data);
      }
#endif
      return;
    }
  }

  ctx_set_error(ctx, "waiting to fetch more responses, but no cptr was registered", IN3_ENOTSUP);
}

void in3_handle_rpc(in3_ctx_t* ctx, ctx_req_transports_t* transports) {
  // error check
  if (!ctx->client->transport) {
    ctx_set_error(ctx, "no transport set", IN3_ECONFIG);
    return;
  }

  // if we can't create the request, this function will put it into error-state
  in3_request_t* request = in3_create_request(ctx);
  if (!request) return;

  // in case there is still a old cptr we need to cleanup since this means this is a retry!
  transport_cleanup(ctx, transports, false);

  // debug output
  for (unsigned int i = 0; i < request->urls_len; i++)
    in3_log_trace("... request to " COLOR_YELLOW_STR "\n... " COLOR_MAGENTA_STR "\n", request->urls[i], i == 0 ? request->payload : "");

  // handle it
  ctx->client->transport(request);

  // debug output
  for (unsigned int i = 0; i < request->urls_len; i++) {
    if (request->ctx->raw_response[i].state != IN3_WAITING) {
      in3_log_trace(request->ctx->raw_response[i].state
                        ? "... response(%i): \n... " COLOR_RED_STR "\n"
                        : "... response(%i): \n... " COLOR_GREEN_STR "\n",
                    i, request->ctx->raw_response[i].data.data);
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
      transports->req = transports->len ? _realloc(transports->req, sizeof(ctx_req_t) * (transports->len + 1), sizeof(ctx_req_t) * transports->len) : _malloc(sizeof(ctx_req_t) * transports->len);
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

in3_ret_t ctx_remove_required(in3_ctx_t* parent, in3_ctx_t* ctx) {
  if (!ctx) return IN3_OK;
  in3_ctx_t* p = parent;
  while (p) {
    if (p->required == ctx) {
      //      printf(" -- remove required %s > %s\n", ctx_name(parent), ctx_name(ctx));
      p->required = NULL; //ctx->required;
      free_ctx_intern(ctx, true);
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
  if (ctx) free_ctx_intern(ctx, false);
}

static inline in3_ret_t pre_handle(in3_verifier_t* verifier, in3_ctx_t* ctx) {
  return verifier->pre_handle ? verifier->pre_handle(ctx, &ctx->raw_response) : IN3_OK;
}

in3_ctx_state_t in3_ctx_exec_state(in3_ctx_t* ctx) {
  in3_ctx_execute(ctx);
  return in3_ctx_state(ctx);
}

in3_ret_t in3_ctx_execute(in3_ctx_t* ctx) {
  in3_ret_t ret;
  // if there is an error it does not make sense to execute.
  if (ctx->error) return (ctx->verification_state && ctx->verification_state != IN3_WAITING) ? ctx->verification_state : IN3_EUNKNOWN;

  // is it a valid request?
  if (!ctx->request_context || !d_get(ctx->requests[0], K_METHOD)) return ctx_set_error(ctx, "No Method defined", IN3_ECONFIG);

  // if there is response we are done.
  if (ctx->response_context && ctx->verification_state == IN3_OK) return IN3_OK;

  // if we have required-contextes, we need to check them first
  if (ctx->required && (ret = in3_ctx_execute(ctx->required))) {
    if (ret == IN3_EIGNORE)
      ctx_handle_failable(ctx);
    else
      return ctx_set_error(ctx, ctx->required->error ? ctx->required->error : "error handling subrequest", ret);
  }

  switch (ctx->type) {
    case CT_RPC: {

      // check chain_id
      in3_chain_t* chain = in3_find_chain(ctx->client, ctx->client->chain_id);
      if (!chain) return ctx_set_error(ctx, "chain not found", IN3_EFIND);

      // find the verifier
      in3_verifier_t* verifier = in3_get_verifier(chain->type);
      if (verifier == NULL) return ctx_set_error(ctx, "No Verifier found", IN3_EFIND);

      // do we need to handle it internaly?
      if (!ctx->raw_response && !ctx->response_context && (ret = pre_handle(verifier, ctx)) < 0)
        return ctx_set_error(ctx, "The request could not be handled", ret);

      // if we don't have a nodelist, we try to get it.
      if (!ctx->raw_response && !ctx->nodes) {
        in3_node_filter_t filter = NODE_FILTER_INIT;
        filter.nodes             = d_get(d_get(ctx->requests[0], K_IN3), K_DATA_NODES);
        filter.props             = (ctx->client->node_props & 0xFFFFFFFF) | NODE_PROP_DATA | ((ctx->client->flags & FLAGS_HTTP) ? NODE_PROP_HTTP : 0) | (in3_ctx_get_proof(ctx) != PROOF_NONE ? NODE_PROP_PROOF : 0);
        if ((ret = in3_node_list_pick_nodes(ctx, &ctx->nodes, ctx->client->request_count, filter)) == IN3_OK) {
          if ((ret = pick_signers(ctx, ctx->requests[0])) < 0)
            return ctx_set_error(ctx, "error configuring the config for request", ret);

#ifdef PAY

          // now we have the nodes, we can prepare the payment
          if (ctx->client->pay && ctx->client->pay->prepare && (ret = ctx->client->pay->prepare(ctx, ctx->client->pay->cptr)) != IN3_OK) return ret;
#endif

        } else
          // since we could not get the nodes, we either report it as error or wait.
          return ctx_set_error(ctx, "could not find any node", ret);
      }

      // if we still don't have an response, we keep on waiting
      if (!ctx->raw_response) return IN3_WAITING;

      // ok, we have a response, then we try to evaluate the responses
      // verify responses and return the node with the correct result.
      ret = find_valid_result(ctx, ctx->nodes == NULL ? 1 : ctx_nodes_len(ctx->nodes), ctx->raw_response, chain, verifier);

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
      } else {
        if (ctx_is_allowed_to_fail(ctx)) {
          ret                     = IN3_EIGNORE;
          ctx->verification_state = IN3_EIGNORE;
        }
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
