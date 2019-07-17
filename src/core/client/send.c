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

static in3_ret_t configure_request(in3_ctx_t* ctx, in3_request_config_t* conf, d_token_t* req) {
  int    i;
  in3_t* c = ctx->client;

  conf->chainId  = c->chainId;
  conf->finality = c->finality;
  if (c->key) {
    // TODO sign the request
    // conf->clientSignature =
  }
  conf->latestBlock = c->replaceLatestBlock;
  conf->useBinary   = c->use_binary;
  if ((c->proof == PROOF_STANDARD || c->proof == PROOF_FULL)) {
    if (c->proof == PROOF_FULL)
      conf->useFullProof = true;
    conf->verification = c->signatureCount ? VERIFICATION_PROOF_WITH_SIGNATURE : VERIFICATION_PROOF;

    if (c->signatureCount) {
      node_weight_t* sig_nodes = NULL;
      in3_ret_t      res       = in3_node_list_pick_nodes(ctx, &sig_nodes);
      if (res < 0)
        return ctx_set_error(ctx, "Could not find any nodes for requesting signatures", res);
      int node_count        = ctx_nodes_len(sig_nodes);
      conf->signaturesCount = node_count;
      conf->signatures      = _malloc(sizeof(bytes_t) * node_count);
      node_weight_t* w      = sig_nodes;
      for (i = 0; i < node_count; i++) {
        conf->signatures[i].len  = w->node->address->len;
        conf->signatures[i].data = w->node->address->data;
      }
      free_ctx_nodes(sig_nodes);
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

static in3_ret_t send_request(in3_ctx_t* ctx, int nodes_count, in3_response_t** response_result) {
  int       n;
  in3_ret_t res;

  *response_result = NULL;
  // prepare the payload
  sb_t* payload = sb_new(NULL);

  // create url-array
  char**         urls = _malloc(sizeof(char*) * nodes_count);
  node_weight_t* w    = ctx->nodes;
  for (n = 0; n < nodes_count; n++) {
    urls[n] = w->node->url;
    w       = w->next;

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
  }

  res = ctx_create_payload(ctx, payload);
  if (res < 0) {
    sb_free(payload);
    free_urls(urls, nodes_count, ctx->client->use_http);
    return ctx_set_error(ctx, "could not generate the payload", res);
  }

  // prepare response-object
  in3_response_t* response = _malloc(sizeof(in3_response_t) * nodes_count);
  for (n = 0; n < nodes_count; n++) {
    sb_init(&response[n].error);
    sb_init(&response[n].result);
    if (ctx->client->evm_flags & IN3_DEBUG) in3_log_debug("... request to \x1B[35m%s\x1B[33m\n... %s\x1B[0m\n", urls[n], payload->data);
  }

  // send requets
  res = ctx->client->transport(urls, nodes_count, payload->data, response);

  if (ctx->client->evm_flags & IN3_DEBUG) in3_log_debug("... response: \n... \x1B[32m%s\x1B[0m\n", response[0].error.len ? response[0].error.data : response[0].result.data);

  // free resources
  sb_free(payload);
  free_urls(urls, nodes_count, ctx->client->use_http);

  if (res < 0) {
    for (n = 0; n < nodes_count; n++) {
      _free(response[n].error.data);
      _free(response[n].result.data);
    }
    _free(response);
    return res;
  }
  *response_result = response;

  return res;
}

static bool find_valid_result(in3_ctx_t* ctx, int nodes_count, in3_response_t* response, in3_chain_t* chain, in3_verifier_t* verifier) {
  node_weight_t* w = ctx->nodes;
  int            n, i, res;

  // find the verifier
  in3_vctx_t vc;
  vc.ctx   = ctx;
  vc.chain = chain;

  // blacklist nodes for missing response
  for (n = 0; n < nodes_count; n++) {
    if (response[n].error.len || !response[n].result.len) {
      // blacklist the node
      w->weight->blacklistedUntil = _time() + 3600000;
      w->weight                   = NULL;
      in3_log_info("Blacklisting node for empty response: %s\n", w->node->url);
    } else {
      // we need to clean up the previos responses if set
      if (ctx->responses) _free(ctx->responses);
      if (ctx->response_context) free_json(ctx->response_context);

      // parse the result
      res = ctx_parse_response(ctx, response[n].result.data, response[n].result.len);
      if (res < 0) {
        // blacklist!
        w->weight->blacklistedUntil = _time() + 3600000;
        w->weight                   = NULL;
        in3_log_info("Blacklisting node for invalid response: %s\n", w->node->url);
      } else {
        //        printf("res:%s",ctx->response_data);
        // check each request
        for (i = 0; i < ctx->len; i++) {
          vc.request = ctx->requests[i];
          vc.result  = d_get(ctx->responses[i], K_RESULT);
          vc.id      = d_get_longk(ctx->responses[i], K_ID);
          vc.config  = ctx->requests_configs + i;

          if ((vc.proof = d_get(ctx->responses[i], K_IN3))) {
            vc.last_validator_change = d_get_longk(vc.proof, K_LAST_VALIDATOR_CHANGE);
            vc.proof                 = d_get(vc.proof, K_PROOF);
          }

          if (verifier && verifier->verify(&vc)) {
            // blacklist!
            w->weight->blacklistedUntil = _time() + 3600000;
            w->weight                   = NULL;
            in3_log_info("Blacklisting node for verification failure: %s\n", w->node->url);
            break;
          }
        }
      }
    }
    if (w->weight)
      // this reponse was successfully verified, so let us keep it.
      return true;

    w = w->next;
  }
  return false;
}

in3_ret_t in3_send_ctx(in3_ctx_t* ctx) {
  // find the nodes to send the request to
  int             i, nodes_count;
  in3_response_t* response = NULL;
  in3_chain_t*    chain    = NULL;
  in3_ret_t       res      = in3_node_list_pick_nodes(ctx, &ctx->nodes);
  if (res < 0)
    return ctx_set_error(ctx, "could not find any node", res);
  nodes_count = ctx_nodes_len(ctx->nodes);

  // configure the requests
  for (i = 0; i < ctx->len; i++) {
    res = configure_request(ctx, ctx->requests_configs + i, ctx->requests[i]);
    if (res < 0)
      return ctx_set_error(ctx, "error configuring the config for request", res);
  }
  // now send the request
  if (!ctx->client->transport)
    return ctx_set_error(ctx, "no transport set", IN3_ECONFIG);

  // find the chain-config.
  for (i = 0; i < ctx->client->chainsCount; i++) {
    if (ctx->client->chains[i].chainId == ctx->client->chainId) {
      chain = ctx->client->chains + i;
      break;
    }
  }
  if (chain == NULL) return ctx_set_error(ctx, "chain not found", IN3_EFIND);

  // find the verifier
  in3_verifier_t* verifier = in3_get_verifier(chain->type);
  if (verifier == NULL) {
    return ctx_set_error(ctx, "No Verifier found", IN3_EFIND);
  }

  // do we need to handle it inside?
  if (verifier->pre_handle) {
    res = verifier->pre_handle(ctx, &response);
    if (res < 0)
      return ctx_set_error(ctx, "The request could not be send", res);
  }

  // no response yet, so we ask the transport to give us a response.
  if (response == NULL)
    res = send_request(ctx, nodes_count, &response);

  // this is not acceptable, so we consider no reponse as an error.
  if (res < 0 || response == NULL)
    return ctx_set_error(ctx, "The request could not be send", res);

  // verify responses and return the node with the correct result.
  bool is_valid = find_valid_result(ctx, nodes_count, response, chain, verifier);

  // clean up responses exycept the response we want to keep.
  for (i = 0; i < nodes_count; i++) {
    _free(response[i].error.data);
    if (ctx->response_context == NULL || response[i].result.data != ctx->response_context->c) _free(response[i].result.data);
  }
  _free(response);

  if (!is_valid) {
    // this means all of the responses failed or could not be verified
    if (ctx->attempt < ctx->client->max_attempts - 1) {
      ctx->attempt++;
      // clean up old results
      if (ctx->responses) _free(ctx->responses);
      if (ctx->response_context) {
        _free(ctx->response_context->c);
        _free(ctx->response_context);
      }
      if (ctx->requests_configs) {
        for (i = 0; i < ctx->len; i++) {
          if (ctx->requests_configs[i].signaturesCount)
            _free(ctx->requests_configs[i].signatures);
        }
      }
      ctx->responses        = NULL;
      ctx->response_context = NULL;
      in3_log_debug("Retrying send request...\n");
      // now try again
      return in3_send_ctx(ctx);
    } else
      // we give up
      return ctx->client->max_attempts == 1 ? IN3_EUNKNOWN : ctx_set_error(ctx, "reaching max_attempts and giving up", IN3_ELIMIT);
  } else {
    //  check nodelist update
    d_token_t* in3 = d_get(ctx->responses[0], K_IN3);
    if (in3 && ctx->client->autoUpdateList && !chain->needsUpdate) {
      uint64_t last_nodelist = d_get_longk(in3, K_LAST_NODE_LIST);
      if (last_nodelist > chain->lastBlock) {
        in3_log_debug("found newer block, so we will update the nodelist next time.\n");
        chain->lastBlock   = last_nodelist;
        chain->needsUpdate = true;
      }
    }

    // we have a result
    return IN3_OK;
  }
}
