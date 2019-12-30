/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2019 slock.it GmbH, Blockchains LLC
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

#include "../util/data.h"
#include "../util/log.h"
#include "../util/mem.h"
#include "../util/stringbuilder.h"
#include "../util/utils.h"
#include "cache.h"
#include "client.h"
#include "context.h"
#include "keys.h"
#include "nodelist.h"
#include "verifier.h"
#include <stdint.h>
#include <string.h>
#include <time.h>
//static char* ctx_name(in3_ctx_t* ctx) {
//  return d_get_stringk(ctx->requests[0], K_METHOD);
//}

static void response_free(in3_ctx_t* ctx) {
  if (ctx->nodes) {
    int nodes_count = ctx_nodes_len(ctx->nodes);
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
  //  printf(" -- free ctx %s (%i)\n", ctx_name(ctx), is_sub ? 1 : 0);

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
static in3_ret_t configure_request(in3_ctx_t* ctx, in3_request_config_t* conf, d_token_t* req) {
  int    i;
  in3_t* c = ctx->client;

  conf->chain_id = c->chain_id;
  conf->finality = c->finality;
  //  if (c->key) {
  // TODO sign the request
  // conf->clientSignature =
  //  }
  conf->latest_block = c->replace_latest_block;
  conf->use_binary   = c->use_binary;
  if ((c->proof == PROOF_STANDARD || c->proof == PROOF_FULL)) {
    if (c->proof == PROOF_FULL)
      conf->use_full_proof = true;
    conf->verification = VERIFICATION_PROOF;

    if (c->signature_count) {
      node_weight_t*   sig_nodes = NULL;
      in3_node_props_t props     = c->node_props | NODE_PROP_SIGNER;
      in3_ret_t        res       = in3_node_list_pick_nodes(ctx, &sig_nodes, c->signature_count, props);
      if (res < 0)
        return ctx_set_error(ctx, "Could not find any nodes for requesting signatures", res);
      int node_count       = ctx_nodes_len(sig_nodes);
      conf->signers_length = node_count;
      conf->signers        = _malloc(sizeof(bytes_t) * node_count);
      node_weight_t* w     = sig_nodes;
      for (i = 0; i < node_count; i++) {
        conf->signers[i].len  = w->node->address->len;
        conf->signers[i].data = w->node->address->data;
        w                     = w->next;
      }
      in3_ctx_free_nodes(sig_nodes);
    }
  }

  if (req) {
    d_token_t* in3 = d_get(req, K_IN3);
    if (in3 == NULL) return IN3_OK;
    //TODO read config from request
  }
  return IN3_OK;
}

static void free_urls(char** urls, int len, uint8_t free_items) {
  if (free_items) {
    for (int i = 0; i < len; i++) _free(urls[i]);
  }
  _free(urls);
}

static unsigned long counter = 1;

static in3_ret_t ctx_create_payload(in3_ctx_t* c, sb_t* sb, bool multichain) {
  int        i;
  d_token_t *r, *t;
  char       temp[100];
  sb_add_char(sb, '[');

  for (i = 0; i < c->len; i++) {
    if (i > 0) sb_add_char(sb, ',');
    sb_add_char(sb, '{');
    r = c->requests[i];
    if ((t = d_get(r, K_ID)) == NULL)
      sb_add_key_value(sb, "id", temp, sprintf(temp, "%lu", counter++), false);
    else if (d_type(t) == T_INTEGER)
      sb_add_key_value(sb, "id", temp, sprintf(temp, "%i", d_int(t)), false);
    else
      sb_add_key_value(sb, "id", d_string(t), d_len(t), true);
    sb_add_char(sb, ',');
    sb_add_key_value(sb, "jsonrpc", "2.0", 3, true);
    sb_add_char(sb, ',');
    if ((t = d_get(r, K_METHOD)) == NULL)
      return ctx_set_error(c, "missing method-property in request", IN3_EINVAL);
    else
      sb_add_key_value(sb, "method", d_string(t), d_len(t), true);
    sb_add_char(sb, ',');
    if ((t = d_get(r, K_PARAMS)) == NULL)
      sb_add_key_value(sb, "params", "[]", 2, false);
    else {
      //TODO this only works with JSON!!!!
      str_range_t ps = d_to_json(t);
      sb_add_key_value(sb, "params", ps.data, ps.len, false);
    }

    in3_request_config_t* rc = c->requests_configs + i;
    if (rc->verification == VERIFICATION_PROOF) {
      // add in3
      //TODO This only works for chain_ids < uint_32t, but ZEPHYR has some issues with PRIu64
      sb_add_range(sb, temp, 0, sprintf(temp, ",\"in3\":{\"verification\":\"proof\",\"version\": \"%s\"", IN3_PROTO_VER));
      if (multichain)
        sb_add_range(sb, temp, 0, sprintf(temp, ",\"chainId\":\"0x%x\"", (unsigned int) rc->chain_id));
      in3_chain_t* chain = in3_find_chain(c->client, c->requests_configs->chain_id ? c->requests_configs->chain_id : c->client->chain_id);
      if (chain->whitelist) {
        bytes_t adr = bytes(chain->whitelist->contract, 20);
        sb_add_bytes(sb, ",\"whiteListContract\":", &adr, 1, false);
      }
      if (rc->client_signature)
        sb_add_bytes(sb, ",\"clientSignature\":", rc->client_signature, 1, false);
      if (rc->finality)
        sb_add_range(sb, temp, 0, sprintf(temp, ",\"finality\":%i", rc->finality));
      if (rc->latest_block)
        sb_add_range(sb, temp, 0, sprintf(temp, ",\"latestBlock\":%i", rc->latest_block));
      if (rc->signers_length)
        sb_add_bytes(sb, ",\"signers\":", rc->signers, rc->signers_length, true);
      if (rc->include_code && strcmp(d_get_stringk(r, K_METHOD), "eth_call") == 0)
        sb_add_chars(sb, ",\"includeCode\":true");
      if (rc->use_full_proof)
        sb_add_chars(sb, ",\"useFullProof\":true");
      if (rc->use_binary)
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

static in3_ret_t ctx_parse_response(in3_ctx_t* ctx, char* response_data, int len) {
  int        i;
  d_token_t* t = NULL;

  d_track_keynames(1);
  ctx->response_context = (response_data[0] == '{' || response_data[0] == '[') ? parse_json(response_data) : parse_binary_str(response_data, len);
  d_track_keynames(0);
  if (!ctx->response_context)
    return ctx_set_error(ctx, "Error parsing the JSON-response!", IN3_EINVALDT);

  if (d_type(ctx->response_context->result) == T_OBJECT) {
    // it is a single result
    ctx->responses    = _malloc(sizeof(d_token_t*));
    ctx->responses[0] = ctx->response_context->result;
    if (ctx->len != 1) return ctx_set_error(ctx, "The response must be a single object!", IN3_EINVALDT);
  } else if (d_type(ctx->response_context->result) == T_ARRAY) {
    if (d_len(ctx->response_context->result) != ctx->len)
      return ctx_set_error(ctx, "The responses must be a array with the same number as the requests!", IN3_EINVALDT);
    ctx->responses = _malloc(sizeof(d_token_t*) * ctx->len);
    for (i = 0, t = ctx->response_context->result + 1; i < ctx->len; i++, t = d_next(t))
      ctx->responses[i] = t;
  } else
    return ctx_set_error(ctx, "The response must be a Object or Array", IN3_EINVALDT);

  return IN3_OK;
}

static in3_ret_t find_valid_result(in3_ctx_t* ctx, int nodes_count, in3_response_t* response, in3_chain_t* chain, in3_verifier_t* verifier) {
  node_weight_t* w = ctx->nodes;
  int            n, i, res;

  // find the verifier
  in3_vctx_t vc;
  vc.ctx   = ctx;
  vc.chain = chain;

  // blacklist nodes for missing response
  for (n = 0; n < nodes_count; n++) {
    if (response[n].error.len || !response[n].result.len) {
      if (w) {
        // blacklist the node
        w->weight->blacklisted_until = _time() + 3600000;
        w->weight                    = NULL;
        in3_log_info("Blacklisting node for empty response: %s\n", w->node->url);
      }
    } else {
      // we need to clean up the previos responses if set
      if (ctx->responses) _free(ctx->responses);
      if (ctx->response_context) json_free(ctx->response_context);

      // parse the result
      res = ctx_parse_response(ctx, response[n].result.data, response[n].result.len);
      if (res < 0) {
        if (w) {
          // blacklist!
          w->weight->blacklisted_until = _time() + 3600000;
          w->weight                    = NULL;
          in3_log_info("Blacklisting node for invalid response: %s\n", w->node->url);
        }
      } else {
        // check each request
        for (i = 0; i < ctx->len; i++) {
          vc.request = ctx->requests[i];
          vc.result  = d_get(ctx->responses[i], K_RESULT);
          vc.id      = d_get_longk(ctx->responses[i], K_ID);
          vc.config  = ctx->requests_configs + i;

          if ((vc.proof = d_get(ctx->responses[i], K_IN3))) {
            vc.last_validator_change = d_get_longk(vc.proof, K_LAST_VALIDATOR_CHANGE);
            vc.currentBlock          = d_get_longk(vc.proof, K_CURRENT_BLOCK);
            vc.proof                 = d_get(vc.proof, K_PROOF);
            if (ctx->client->auto_update_list) {
              if (d_get_longk(vc.proof, K_LAST_NODE_LIST) > chain->last_block)
                chain->needs_update = true;

              if (chain->whitelist && d_get_longk(vc.proof, K_LAST_WHITE_LIST) > chain->whitelist->last_block)
                chain->whitelist->needs_update = true;
            }
          }

          if (verifier && (res = (ctx->verification_state = verifier->verify(&vc)))) {
            if (res == IN3_WAITING) return res;
            if (w) {
              // blacklist!
              w->weight->blacklisted_until = _time() + 3600000;
              w->weight                    = NULL;
              in3_log_info("Blacklisting node for verification failure: %s\n", w->node->url);
            }
            break;
          } else
            ctx->verification_state = IN3_OK;
        }
      }
    }
    if (!w || w->weight)
      // this reponse was successfully verified, so let us keep it.
      return IN3_OK;

    w = w->next;
  }
  return IN3_EINVAL;
}

in3_request_t* in3_create_request(in3_ctx_t* ctx) {

  int       nodes_count = ctx_nodes_len(ctx->nodes), n;
  in3_ret_t res;

  // prepare the payload
  sb_t* payload = sb_new(NULL);

  // create url-array
  char**         urls       = nodes_count ? _malloc(sizeof(char*) * nodes_count) : NULL;
  node_weight_t* w          = ctx->nodes;
  bool           multichain = false;
  for (n = 0; n < nodes_count; n++) {
    urls[n] = w->node->url;

    if (in3_node_props_get(w->node->props, NODE_PROP_MULTICHAIN)) multichain = true;

    if (ctx->client->use_http) {
      char* url = NULL;
      int   l   = strlen(urls[n]);
      if (strncmp(urls[n], "https://", 8) == 0) {
        url = _malloc(l);
        strcpy(url, urls[n] + 1);
        url[0] = 'h';
        url[2] = 't';
        url[3] = 'p';
      } else
        url = _strdupn(urls[n], l);
      urls[n] = url;
    }

    w = w->next;
  }

  res = ctx_create_payload(ctx, payload, multichain);
  if (res < 0) {
    sb_free(payload);
    free_urls(urls, nodes_count, ctx->client->use_http);
    ctx_set_error(ctx, "could not generate the payload", res);
    return NULL;
  }

  // prepare response-object
  in3_request_t* req = _malloc(sizeof(in3_request_t));
  req->payload       = payload->data;
  req->urls_len      = nodes_count;
  req->urls          = urls;

  if (!nodes_count) nodes_count = 1; // at least one result!
  req->results = _malloc(sizeof(in3_response_t) * nodes_count);
  for (n = 0; n < nodes_count; n++) {
    sb_init(&req->results[n].error);
    sb_init(&req->results[n].result);
  }

  // we set the raw_response
  ctx->raw_response = req->results;

  _free(payload); // we only clean up the the stringbuffer, but keep the content (payload->data)
  return req;
}

void request_free(in3_request_t* req, in3_ctx_t* ctx, bool response_free) {
  // free resources
  free_urls(req->urls, req->urls_len, ctx->client->use_http);

  if (response_free) {
    for (int n = 0; n < req->urls_len; n++) {
      _free(req->results[n].error.data);
      //      if (!ctx->response_context || ctx->response_context->c != req->results[n].result.data)
      _free(req->results[n].result.data);
    }
    _free(req->results);
  }

  _free(req->payload);
  _free(req);

  return;
}

in3_ret_t in3_send_ctx(in3_ctx_t* ctx) {
  int       count = 0;
  in3_ret_t ret;
  while ((ret = in3_ctx_execute(ctx))) {
    count++;
    if (count > 10) return ctx_set_error(ctx, "Looks like the response is not valid or not set, since we calling the execute over and over", IN3_ERPC);
    // error we stop here
    if (ret != IN3_WAITING) return ret;

    // handle subcontexts first
    while (ctx->required && in3_ctx_state(ctx->required) != CTX_SUCCESS) {
      if ((ret = in3_send_ctx(ctx->required)))
        return ret;
      // recheck in order to prepare the request.
      if ((ret = in3_ctx_execute(ctx)) != IN3_WAITING) return ret;
    }

    if (!ctx->raw_response) {
      switch (ctx->type) {
        case CT_RPC: {
          if (ctx->client->transport) {
            // handle transports
            in3_request_t* request = in3_create_request(ctx);
            if (request == NULL)
              return IN3_ENOMEM;
            in3_log_trace("... request to \x1B[35m%s\x1B[33m\n... %s\x1B[0m\n", request->urls[0], request->payload);
            ctx->client->transport(request);
            in3_log_trace("... response: \n... \x1B[%sm%s\x1B[0m\n", request->results[0].error.len ? "31" : "32", request->results[0].error.len ? request->results[0].error.data : request->results[0].result.data);
            request_free(request, ctx, false);
            break;
          } else
            return ctx_set_error(ctx, "no transport set", IN3_ECONFIG);
        }
        case CT_SIGN: {
          if (ctx->client->signer) {
            d_token_t* params = d_get(ctx->requests[0], K_PARAMS);
            bytes_t    data   = d_to_bytes(d_get_at(params, 0));
            bytes_t    from   = d_to_bytes(d_get_at(params, 1));
            if (!data.data) return ctx_set_error(ctx, "missing data to sign", IN3_ECONFIG);
            if (!from.data) return ctx_set_error(ctx, "missing account to sign", IN3_ECONFIG);

            ctx->raw_response = _malloc(sizeof(in3_response_t));
            sb_init(&ctx->raw_response[0].error);
            sb_init(&ctx->raw_response[0].result);
            in3_log_trace("... request to sign ");
            uint8_t sig[65];
            ret = ctx->client->signer->sign(ctx, SIGN_EC_HASH, data, from, sig);
            if (ret < 0) return ctx_set_error(ctx, ctx->raw_response->error.data, ret);
            sb_add_range(&ctx->raw_response->result, (char*) sig, 0, 65);
            break;
          } else
            return ctx_set_error(ctx, "no transport set", IN3_ECONFIG);
        }
      }
    }
  }
  return ret;
}

in3_ctx_t* ctx_find_required(in3_ctx_t* parent, char* method) {
  in3_ctx_t* p = parent->required;
  while (p) {
    if (!p->requests) continue;
    char* m = d_get_stringk(p->requests[0], K_METHOD);
    if (m && strcmp(m, method) == 0) return p;
    p = p->required;
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
        in3_node_props_t props = (ctx->client->node_props & 0xFFFFFFFF) | NODE_PROP_DATA | (ctx->client->use_http ? NODE_PROP_HTTP : 0) | (ctx->client->proof != PROOF_NONE ? NODE_PROP_PROOF : 0);
        if ((ret = in3_node_list_pick_nodes(ctx, &ctx->nodes, ctx->client->request_count, props)) == IN3_OK) {
          for (int i = 0; i < ctx->len; i++) {
            if ((ret = configure_request(ctx, ctx->requests_configs + i, ctx->requests[i])) < 0)
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

      // we wait or are have successfully verified the response
      if (ret == IN3_WAITING || ret == IN3_OK) return ret;

      // if not, then we clean up
      response_free(ctx);

      // we count this is an attempt
      ctx->attempt++;

      // should we retry?
      if (ctx->attempt < ctx->client->max_attempts - 1) {
        in3_log_debug("Retrying send request...\n");
        // reset the error and try again
        if (ctx->error) _free(ctx->error);
        ctx->error = NULL;
        // now try again, which should end in waiting for the next request.
        return in3_ctx_execute(ctx);
      } else
        // we give up
        return ctx->error ? (ret ? ret : IN3_ERPC) : ctx_set_error(ctx, "reaching max_attempts and giving up", IN3_ELIMIT);
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
