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

static void response_free(in3_ctx_t* ctx) {
  if (ctx->nodes) {
    const int nodes_count = ctx_nodes_len(ctx->nodes);
    in3_ctx_free_nodes(ctx->nodes);
    if (ctx->raw_response) {
      for (int i = 0; i < nodes_count; i++) {
        _free(ctx->raw_response[i].error.data);
        _free(ctx->raw_response[i].result.data);
      }
      _free(ctx->raw_response);
    }
  } else if (ctx->raw_response) {
    _free(ctx->raw_response[0].error.data);
    _free(ctx->raw_response[0].result.data);
    _free(ctx->raw_response);
  }

  if (ctx->responses) _free(ctx->responses);
  if (ctx->response_context) json_free(ctx->response_context);
  ctx->response_context = NULL;
  ctx->responses        = NULL;
  ctx->raw_response     = NULL;
  ctx->nodes            = NULL;
  if (ctx->requests_configs) {
    for (int i = 0; i < ctx->len; i++) {
      if (ctx->requests_configs[i].verified_hashes) {
        _free(ctx->requests_configs[i].verified_hashes);
        ctx->requests_configs[i].verified_hashes = NULL;
      }
      if (ctx->requests_configs[i].signers_length) {
        if (ctx->requests_configs[i].signers) {
          _free(ctx->requests_configs[i].signers);
          ctx->requests_configs[i].signers = NULL;
        }
      }
    }
  }
}

static void free_ctx_intern(in3_ctx_t* ctx, bool is_sub) {
  // only for intern requests, we actually free the original request-string
  if (is_sub) _free(ctx->request_context->c);
  if (ctx->error) _free(ctx->error);
  response_free(ctx);
  if (ctx->request_context)
    json_free(ctx->request_context);

  if (ctx->requests) _free(ctx->requests);
  if (ctx->requests_configs) _free(ctx->requests_configs);
  if (ctx->cache) in3_cache_free(ctx->cache);
  if (ctx->required) free_ctx_intern(ctx->required, true);

  _free(ctx);
}

static bool auto_ask_sig(const in3_ctx_t* ctx) {
  return (ctx_is_method(ctx, "in3_nodeList") && !(ctx->client->flags & FLAGS_NODE_LIST_NO_SIG));
}

static in3_ret_t configure_request(in3_ctx_t* ctx, in3_request_config_t* conf, d_token_t* request, in3_chain_t* chain) {
  UNUSED_VAR(request);

  const in3_t* c     = ctx->client;
  conf->chain_id     = c->chain_id;
  conf->finality     = c->finality;
  conf->latest_block = c->replace_latest_block;
  conf->flags        = c->flags;

  if (c->proof == PROOF_NONE && !auto_ask_sig(ctx))
    return IN3_OK;

  // For nodeList request, we always ask for proof & atleast one signature
  conf->use_full_proof  = c->proof == PROOF_FULL;
  conf->verification    = VERIFICATION_PROOF;
  uint8_t total_sig_cnt = c->signature_count ? c->signature_count
                                             : auto_ask_sig(ctx) ? 1 : 0;

  if (total_sig_cnt) {
    node_match_t*     signer_nodes = NULL;
    in3_node_filter_t filter       = NODE_FILTER_INIT;
    filter.nodes                   = d_get(d_get(ctx->requests[0], K_IN3), K_SIGNER_NODES);
    filter.props                   = c->node_props | NODE_PROP_SIGNER;
    const in3_ret_t res            = in3_node_list_pick_nodes(ctx, &signer_nodes, total_sig_cnt, filter);
    if (res < 0)
      return ctx_set_error(ctx, "Could not find any nodes for requesting signatures", res);
    const int node_count  = ctx_nodes_len(signer_nodes);
    conf->signers_length  = node_count;
    conf->signers         = _malloc(sizeof(bytes_t) * node_count);
    const node_match_t* w = signer_nodes;
    for (int i = 0; i < node_count; i++) {
      conf->signers[i].len  = w->node->address->len;
      conf->signers[i].data = w->node->address->data;
      w                     = w->next;
    }
    in3_ctx_free_nodes(signer_nodes);

    if (chain->verified_hashes) {
      conf->verified_hashes_length = ctx->client->max_verified_hashes;
      for (int i = 0; i < conf->verified_hashes_length; i++) {
        if (!chain->verified_hashes[i].block_number) {
          conf->verified_hashes_length = i;
          break;
        }
      }
      if (conf->verified_hashes_length) {
        conf->verified_hashes = _malloc(sizeof(bytes_t) * conf->verified_hashes_length);
        for (int i = 0; i < conf->verified_hashes_length; i++)
          conf->verified_hashes[i] = bytes(chain->verified_hashes[i].hash, 32);
      }
    }
  }
  return IN3_OK;
}

static void free_urls(char** urls, int len, bool free_items) {
  if (free_items) {
    for (int i = 0; i < len; i++) _free(urls[i]);
  }
  _free(urls);
}

static int add_bytes_to_hash(struct SHA3_CTX* msg_hash, void* data, int len) {
  if (msg_hash) sha3_Update(msg_hash, data, len);
  return len;
}

static void add_token_to_hash(struct SHA3_CTX* msg_hash, d_token_t* t) {
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

static in3_ret_t ctx_create_payload(in3_ctx_t* c, sb_t* sb, bool multichain) {
  static unsigned long rpc_id_counter = 1;
  char                 temp[100];
  struct SHA3_CTX*     msg_hash = c->client->key ? alloca(sizeof(struct SHA3_CTX)) : NULL;
  //  sha3_Update(&ctx, data->data, data->len);
  //  keccak_Final(&ctx, dst);

  sb_add_char(sb, '[');

  for (int i = 0; i < c->len; i++) {
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
      //TODO this only works with JSON!!!!
      const str_range_t ps = d_to_json(t);
      if (msg_hash) add_token_to_hash(msg_hash, t);
      sb_add_key_value(sb, "params", ps.data, ps.len, false);
    }

    in3_request_config_t* rc = c->requests_configs + i;
    if (rc->verification == VERIFICATION_PROOF || msg_hash) {
      // add in3
      sb_add_range(sb, temp, 0, sprintf(temp, ",\"in3\":{\"verification\":\"%s\",\"version\": \"%s\"", rc->verification == VERIFICATION_NEVER ? "never" : "proof", IN3_PROTO_VER));
      if (multichain)
        sb_add_range(sb, temp, 0, sprintf(temp, ",\"chainId\":\"0x%x\"", (unsigned int) rc->chain_id));
      const in3_chain_t* chain = in3_find_chain(c->client, c->requests_configs->chain_id ? c->requests_configs->chain_id : c->client->chain_id);
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
      if (rc->latest_block)
        sb_add_range(sb, temp, 0, sprintf(temp, ",\"latestBlock\":%i", rc->latest_block));
      if (rc->signers_length)
        sb_add_bytes(sb, ",\"signers\":", rc->signers, rc->signers_length, true);
      if ((rc->flags & FLAGS_INCLUDE_CODE) && strcmp(d_get_stringk(request_token, K_METHOD), "eth_call") == 0)
        sb_add_chars(sb, ",\"includeCode\":true");
      if (rc->use_full_proof)
        sb_add_chars(sb, ",\"useFullProof\":true");
      if ((rc->flags & FLAGS_STATS) == 0)
        sb_add_chars(sb, ",\"noStats\":true");
      if ((rc->flags & FLAGS_BINARY))
        sb_add_chars(sb, ",\"useBinary\":true");
      if (rc->verified_hashes_length)
        sb_add_bytes(sb, ",\"verifiedHashes\":", rc->verified_hashes, rc->verified_hashes_length, true);
      sb_add_range(sb, "}}", 0, 2);
    } else
      sb_add_char(sb, '}');
  }
  sb_add_char(sb, ']');
  return IN3_OK;
}
static void update_nodelist_cache(in3_ctx_t* ctx) {
  // we don't update weights for local chains.
  if (!ctx->client->cache || ctx->client->chain_id == ETH_CHAIN_ID_LOCAL) return;
  chain_id_t chain_id = ctx->requests_configs[0].chain_id;
  if (!chain_id) chain_id = ctx->client->chain_id;
  in3_cache_store_nodelist(ctx, in3_find_chain(ctx->client, chain_id));
}

static in3_ret_t ctx_parse_response(in3_ctx_t* ctx, char* response_data, int len) {

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
    if (d_len(ctx->response_context->result) != ctx->len)
      return ctx_set_error(ctx, "The responses must be a array with the same number as the requests!", IN3_EINVALDT);
    ctx->responses = _malloc(sizeof(d_token_t*) * ctx->len);
    for (i = 0, t = ctx->response_context->result + 1; i < ctx->len; i++, t = d_next(t))
      ctx->responses[i] = t;
  } else
    return ctx_set_error(ctx, "The response must be a Object or Array", IN3_EINVALDT);

  return IN3_OK;
}

static void blacklist_node(node_match_t* node_weight) {
  if (node_weight && node_weight->weight) {
    // blacklist the node
    node_weight->weight->blacklisted_until = in3_time(NULL) + 3600;
    node_weight->weight                    = NULL; // setting the weight to NULL means we reject the response.
    in3_log_info("Blacklisting node for empty response: %s\n", node_weight->node->url);
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

static in3_ret_t find_valid_result(in3_ctx_t* ctx, int nodes_count, in3_response_t* response, in3_chain_t* chain, in3_verifier_t* verifier) {
  node_match_t* node = ctx->nodes;

  // find the verifier
  in3_vctx_t vc;
  vc.ctx   = ctx;
  vc.chain = chain;

  // blacklist nodes for missing response
  for (int n = 0; n < nodes_count; n++) {

    // handle times
    in3_request_config_t* req_conf = ctx->requests_configs + n;
    if (req_conf->time && node && node->weight) {
      node->weight->response_count++;
      node->weight->total_response_time += req_conf->time;
      req_conf->time = 0; // make sure we count the time only once
    }

    // since nodes_count was detected before, this should not happen!

    if (response[n].error.len || !response[n].result.len)
      blacklist_node(node);
    else {
      // we need to clean up the previos responses if set
      if (ctx->responses) _free(ctx->responses);
      if (ctx->response_context) json_free(ctx->response_context);

      // parse the result
      in3_ret_t res = ctx_parse_response(ctx, response[n].result.data, response[n].result.len);
      if (res < 0)
        blacklist_node(node);
      else {
        // check each request
        for (int i = 0; i < ctx->len; i++) {
          vc.request = ctx->requests[i];
          vc.result  = d_get(ctx->responses[i], K_RESULT);
          vc.config  = ctx->requests_configs + i;

          if ((vc.proof = d_get(ctx->responses[i], K_IN3))) {
            // vc.proof is temporary set to the in3-section. It will be updated to real proof in the next lines.
            vc.last_validator_change = d_get_longk(vc.proof, K_LAST_VALIDATOR_CHANGE);
            vc.currentBlock          = d_get_longk(vc.proof, K_CURRENT_BLOCK);
            vc.proof                 = d_get(vc.proof, K_PROOF);
          }

          if (!vc.result && ctx->attempt < ctx->client->max_attempts - 1) {
            // if we don't have a result, the node reported an error
            // since we don't know if this error is our fault or the server fault,we don't blacklist the node, but retry
            ctx->verification_state = IN3_ERPC;
            d_token_t* error        = d_get(ctx->responses[i], K_ERROR);
            char*      err_msg      = d_type(error) == T_STRING ? d_string(error) : d_get_stringk(error, K_MESSAGE);
            // this is a workaround to check whether this is
            if (err_msg && strncmp(err_msg, "Error:", 6) == 0)
              blacklist_node(node);
            else
              node->weight = NULL;
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
            ctx->verification_state = IN3_OK;
        }
      }
    }

    // check auto update opts only if this node wasn't blacklisted (due to wrong result/proof)
    if (!is_blacklisted(node) && d_get(ctx->responses[0], K_IN3))
      check_autoupdate(ctx, chain, d_get(ctx->responses[0], K_IN3), node);

    // !node_weight is valid, because it means this is a internaly handled response
    if (!node || !is_blacklisted(node))
      return IN3_OK; // this reponse was successfully verified, so let us keep it.

    node = node->next;
  }
  // no valid response found
  return IN3_EINVAL;
}

static char* convert_to_http_url(char* src_url) {
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

in3_request_t* in3_create_request(in3_ctx_t* ctx) {

  int       nodes_count = ctx_nodes_len(ctx->nodes);
  in3_ret_t res;

  // create url-array
  char**        urls       = nodes_count ? _malloc(sizeof(char*) * nodes_count) : NULL;
  node_match_t* node       = ctx->nodes;
  bool          multichain = false;

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
  in3_request_t* request = _malloc(sizeof(in3_request_t));
  request->payload       = payload->data;
  request->urls_len      = nodes_count;
  request->urls          = urls;
  request->times         = NULL;
  request->timeout       = ctx->client->timeout;

  if (!nodes_count) nodes_count = 1; // at least one result, because for internal response we don't need nodes, but a result big enough.
  request->results = _malloc(sizeof(in3_response_t) * nodes_count);
  for (int n = 0; n < nodes_count; n++) {
    sb_init(&request->results[n].error);
    sb_init(&request->results[n].result);
  }

  // we set the raw_response
  ctx->raw_response = request->results;

  // we only clean up the the stringbuffer, but keep the content (payload->data)
  _free(payload);
  return request;
}

void request_free(in3_request_t* req, const in3_ctx_t* ctx, bool free_response) {
  // free resources
  free_urls(req->urls, req->urls_len, ctx->client->flags & FLAGS_HTTP);

  if (req->times) {
    for (int i = 0; i < req->urls_len; i++)
      ctx->requests_configs[i].time = req->times[i];
    _free(req->times);
  }

  if (free_response) {
    for (int n = 0; n < req->urls_len; n++) {
      _free(req->results[n].error.data);
      _free(req->results[n].result.data);
    }
    _free(req->results);
  }

  _free(req->payload);
  _free(req);
}

static bool ctx_is_allowed_to_fail(in3_ctx_t* ctx) {
  return ctx_is_method(ctx, "in3_nodeList");
}

in3_ret_t ctx_handle_failable(in3_ctx_t* ctx) {
  ctx_remove_required(ctx, ctx->required);

  // blacklist node that gave us an error response for nodelist (if not first update)
  // and clear nodelist params
  in3_chain_t* chain = in3_find_chain(ctx->client, ctx->client->chain_id);

  if (nodelist_not_first_upd8(chain))
    blacklist_node_addr(chain, chain->nodelist_upd8_params->node, 3600);
  _free(chain->nodelist_upd8_params);
  chain->nodelist_upd8_params = NULL;

  // if first update return error otherwise return IN3_OK, this is because first update is
  // always from a boot node which is presumed to be trusted
  if (nodelist_first_upd8(chain))
    return ctx_set_error(ctx, ctx->required->error ? ctx->required->error : "error handling subrequest", IN3_ERPC);
  return IN3_OK;
}

in3_ret_t in3_send_ctx(in3_ctx_t* ctx) {
  int       retry_count = 0;
  in3_ret_t res;

  while ((res = in3_ctx_execute(ctx)) != IN3_OK) {

    // error we stop here
    if (res != IN3_WAITING) return res;

    // we are waiting for an response.
    retry_count++;
    if (retry_count > 10) return ctx_set_error(ctx, "Looks like the response is not valid or not set, since we are calling the execute over and over", IN3_ERPC);

    // handle subcontexts first
    while (ctx->required && in3_ctx_state(ctx->required) != CTX_SUCCESS) {
      res = in3_send_ctx(ctx->required);
      if (res == IN3_EIGNORE)
        ctx_handle_failable(ctx);
      else if (res != IN3_OK)
        return ctx_set_error(ctx, ctx->required->error ? ctx->required->error : "error handling subrequest", res);

      // recheck in order to prepare the request.
      if ((res = in3_ctx_execute(ctx)) != IN3_WAITING) return res;
    }

    if (!ctx->raw_response) {
      switch (ctx->type) {
        case CT_RPC: {
          if (ctx->client->transport) {
            // handle transports
            in3_request_t* request = in3_create_request(ctx);
            if (request == NULL)
              return IN3_ENOMEM;
            in3_log_trace("... request to " COLOR_YELLOW_STR "\n... " COLOR_MAGENTA_STR "\n", request->urls[0], request->payload);
            ctx->client->transport(request);
            if (request->results[0].error.len) {
              in3_log_trace("... response: \n... " COLOR_RED_STR "\n", request->results[0].error.len ? request->results[0].error.data : request->results[0].result.data);
            } else {
              in3_log_trace("... response: \n... " COLOR_GREEN_STR "\n", request->results[0].error.len ? request->results[0].error.data : request->results[0].result.data);
            }
            request_free(request, ctx, false);
            break;
          } else
            return ctx_set_error(ctx, "no transport set", IN3_ECONFIG);
        }
        case CT_SIGN: {
          if (ctx->client->signer) {
            d_token_t*    params = d_get(ctx->requests[0], K_PARAMS);
            const bytes_t data   = d_to_bytes(d_get_at(params, 0));
            const bytes_t from   = d_to_bytes(d_get_at(params, 1));
            if (!data.data) return ctx_set_error(ctx, "missing data to sign", IN3_ECONFIG);
            if (!from.data) return ctx_set_error(ctx, "missing account to sign", IN3_ECONFIG);

            ctx->raw_response = _malloc(sizeof(in3_response_t));
            sb_init(&ctx->raw_response[0].error);
            sb_init(&ctx->raw_response[0].result);
            in3_log_trace("... request to sign ");
            uint8_t sig[65];
            res = ctx->client->signer->sign(ctx, SIGN_EC_HASH, data, from, sig);
            if (res < 0) return ctx_set_error(ctx, ctx->raw_response->error.data, res);
            sb_add_range(&ctx->raw_response->result, (char*) sig, 0, 65);
            break;
          } else
            return ctx_set_error(ctx, "no transport set", IN3_ECONFIG);
        }
      }
    }
  }
  return res;
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
  in3_ctx_state_t required_state = in3_ctx_state(ctx->required);
  if (required_state == CTX_ERROR) return CTX_ERROR;
  if (ctx->error) return CTX_ERROR;
  if (ctx->required && required_state != CTX_SUCCESS) return CTX_WAITING_FOR_REQUIRED_CTX;
  if (!ctx->raw_response) return CTX_WAITING_FOR_RESPONSE;
  if (ctx->type == CT_RPC && !ctx->response_context) return CTX_WAITING_FOR_RESPONSE;
  return CTX_SUCCESS;
}

void ctx_free(in3_ctx_t* ctx) {
  if (ctx) free_ctx_intern(ctx, false);
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
  if (ctx->required && (ret = in3_ctx_execute(ctx->required)))
    return ret;

  switch (ctx->type) {
    case CT_RPC: {

      // check chain_id
      in3_chain_t* chain = in3_find_chain(ctx->client, ctx->requests_configs->chain_id ? ctx->requests_configs->chain_id : ctx->client->chain_id);
      if (!chain) return ctx_set_error(ctx, "chain not found", IN3_EFIND);

      // find the verifier
      in3_verifier_t* verifier = in3_get_verifier(chain->type);
      if (verifier == NULL)
        return ctx_set_error(ctx, "No Verifier found", IN3_EFIND);

      // do we need to handle it internaly?
      if (!ctx->raw_response && !ctx->response_context && verifier->pre_handle && (ret = verifier->pre_handle(ctx, &ctx->raw_response)) < 0)
        return ctx_set_error(ctx, "The request could not be handled", ret);

      // if we don't have a nodelist, we try to get it.
      if (!ctx->raw_response && !ctx->nodes) {
        in3_node_filter_t filter = NODE_FILTER_INIT;
        filter.nodes             = d_get(d_get(ctx->requests[0], K_IN3), K_DATA_NODES);
        filter.props             = (ctx->client->node_props & 0xFFFFFFFF) | NODE_PROP_DATA | ((ctx->client->flags & FLAGS_HTTP) ? NODE_PROP_HTTP : 0) | (ctx->client->proof != PROOF_NONE ? NODE_PROP_PROOF : 0);
        if ((ret = in3_node_list_pick_nodes(ctx, &ctx->nodes, ctx->client->request_count, filter)) == IN3_OK) {
          for (int i = 0; i < ctx->len; i++) {
            if ((ret = configure_request(ctx, ctx->requests_configs + i, ctx->requests[i], chain)) < 0)
              return ctx_set_error(ctx, "error configuring the config for request", ret);
          }
        } else
          // since we could not get the nodes, we either report it as error or wait.
          return ret == IN3_WAITING ? ret : ctx_set_error(ctx, "could not find any node", ret);
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
        ctx->error = NULL;
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
      if (!ctx->raw_response)
        return IN3_WAITING;
      else if (ctx->raw_response->error.len)
        return IN3_ERPC;
      else if (!ctx->raw_response->result.len)
        return IN3_WAITING;
      return IN3_OK;
    }
    default:
      return IN3_EINVAL;
  }
}

void in3_req_add_response(
    in3_response_t* res,      /**< [in] the request-pointer passed to the transport-function containing the payload and url */
    int             index,    /**< [in] the index of the url, since this request could go out to many urls */
    bool            is_error, /**< [in] if true this will be reported as error. the message should then be the error-message */
    void*           data,     /**<  the data or the the string*/
    int             data_len  /**<  the length of the data or the the string (use -1 if data is a null terminated string)*/
) {
  sb_t* sb = is_error ? &res[index].error : &res[index].result;
  if (data_len == -1)
    sb_add_chars(sb, data);
  else
    sb_add_range(sb, data, 0, data_len);
}
