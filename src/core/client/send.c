#include <stdlib.h>  
#include <string.h>
#include <stdio.h>
#include <stdint.h>  
#include "client.h"  
#include "../util/utils.h"
#include <errno.h> 
#include "context.h"
#include "cache.h"
#include <time.h>
#include "../util/stringbuilder.h"
#include "verifier.h"
#include "nodelist.h"
#include "../util/mem.h"
#include "../util/data.h"
#include "keys.h"


static int configure_request(in3_ctx_t* ctx, in3_request_config_t* conf, d_token_t* req ) {
   int i;
   in3_t* c         = ctx->client;

   conf->chainId  = c->chainId;
   conf->finality = c->finality;
   if (c->key) {
      // TODO sign the request
      // conf->clientSignature = 
   }
   conf->latestBlock = c->replaceLatestBlock;
   if ((c->proof==PROOF_STANDARD || c->proof==PROOF_FULL) ) {
     if (c->proof==PROOF_FULL)
       conf->useFullProof=true;
     conf->verification = c->signatureCount ? VERIFICATION_PROOF_WITH_SIGNATURE : VERIFICATION_PROOF;

     if (c->signatureCount) {
       node_weight_t* sig_nodes=NULL;
       int res = in3_node_list_pick_nodes(ctx, &sig_nodes);
       if (res<0) 
         return ctx_set_error(ctx,"Could not find any nodes for requesting signatures",IN3_ERR_NO_NODES_FOUND);
       int node_count = ctx_nodes_len(sig_nodes);
       conf->signaturesCount = node_count;
       conf->signatures = _malloc(sizeof(bytes_t)*node_count);
       node_weight_t* w= sig_nodes;
       for (i=0;i<node_count;i++) {
         conf->signatures[i].len = w->node->address->len;
         conf->signatures[i].data = w->node->address->data;
       }
       free_ctx_nodes(sig_nodes);
     }
   }

   if (req) {
     d_token_t* in3 = d_get(req,K_IN3);
     if (in3==NULL) return 0;
     //TODO read config from request
   }
   return 0;
}


static int send_request(in3_ctx_t* ctx, int nodes_count,in3_response_t** response_result ) {
  int n,res;

  *response_result = NULL;
  // prepare the payload
  sb_t* payload = sb_new(NULL);

  // create url-array
  char** urls = _malloc(sizeof(char*)* nodes_count);
  node_weight_t* w = ctx->nodes;
  for (n=0;n<nodes_count;n++) {
    urls[n]=w->node->url;
    w=w->next;
  }

  res = ctx_create_payload(ctx, payload);
  if (res<0)  {
     sb_free(payload);
    _free(urls);
    return ctx_set_error(ctx,"could not generate the payload",IN3_ERR_CONFIG_ERROR);
  }

  // prepare response-object
  in3_response_t* response = _malloc( sizeof(in3_response_t)*nodes_count );
  for (n=0;n<nodes_count;n++) {
    sb_init(&response[n].error);
    sb_init(&response[n].result);
  }

  // send requets 
  res = ctx->client->transport(urls,nodes_count, payload->data,response);

  // free resources
  sb_free(payload);
 _free(urls);

  if (res<0) {
     for (n=0;n<nodes_count;n++) {
      _free(response[n].error.data);
      _free(response[n].result.data);
     }
    _free(response);
     return res;
  }
  *response_result = response;

  return res;

}


static bool find_valid_result(in3_ctx_t* ctx, int nodes_count,in3_response_t* response) {
  node_weight_t* w = ctx->nodes;
  in3_chain_t* chain = NULL;
  int n,i,res;

  // find the chain-config.
  for (i=0;i< ctx->client->serversCount;i++) {
    chain = ctx->client->servers + i;
    if (chain->chainId==ctx->client->chainId) break;
  } 

  if (chain==NULL) return false;

  // find the verifier
  in3_verifier_t* verifier = in3_get_verifier(chain->type);
  if (verifier==NULL && ctx->client->proof!=PROOF_NONE) {
    ctx_set_error(ctx,"No Verifier found",-1);
    return false;
  }

  in3_vctx_t vc;
  vc.ctx=ctx;
  vc.chain=chain;


  // blacklist nodes for missing response
  for (n=0;n<nodes_count;n++) {
    if (response[n].error.len || !response[n].result.len) {
      // blacklist the node
      w->weight->blacklistedUntil = _time() + 3600000;
      w->weight = NULL;
    }
    else {
      // we need to clean up the prev ios responses if set
      if (ctx->responses) _free(ctx->responses);
      if (ctx->response_context) free_json(ctx->response_context);

      // parse the result
      res = ctx_parse_response(ctx,response[n].result.data);
      if (res<0) {
        // blacklist!
        w->weight->blacklistedUntil = _time() + 3600000;
        w->weight = NULL;
      }
      else {
//        printf("res:%s",ctx->response_data);
        // check each request
        for (i=0;i<ctx->len;i++) {
          vc.request=ctx->requests[i];
          vc.result = d_get(ctx->responses[i],K_RESULT);
          if ((vc.proof =  d_get(ctx->responses[i],K_IN3)))
            vc.proof = d_get(vc.proof,K_PROOF);
          vc.config = ctx->requests_configs +i;

          if (verifier && verifier->verify(&vc)) {
            // blacklist!
            w->weight->blacklistedUntil = _time() + 3600000;
            w->weight = NULL;
            break;
          }
        } 
      }
    }
    if (w->weight) 
      // this reponse was successfully verified, so let us keep it.
      return true;

    w=w->next;
  }
  return false;
    
}

int in3_send_ctx( in3_ctx_t* ctx) {
  // find the nodes to send the request to
  int i,nodes_count;

  in3_response_t* response;
  int res = in3_node_list_pick_nodes(ctx, &ctx->nodes);
  if (res<0)
    return ctx_set_error(ctx, "could not find any node",res);
  nodes_count = ctx_nodes_len(ctx->nodes);

  // configure the requests
  for (i=0;i<ctx->len;i++) {
    res = configure_request(ctx,ctx->requests_configs+i,ctx->requests[i]);
    if (res<0) 
       return ctx_set_error(ctx, "error configuring the config for request",res);
  }
  // now send the request
  if (!ctx->client->transport)
     return ctx_set_error(ctx, "no transport set",IN3_ERR_CONFIG_ERROR);

  res = send_request(ctx,nodes_count,&response);
  if (res<0 || response==NULL) 
     return ctx_set_error(ctx, "The request could not be send",res);
    // verify responses

  // verify responses and return the node with the correct result.
  bool is_valid = find_valid_result(ctx,nodes_count,response);

  // clean up responses exycept the response we want to keep.
  for (i=0;i<nodes_count;i++) {
   _free(response[i].error.data);
    if (ctx->response_context ==NULL || response[i].result.data!=ctx->response_context->c) _free(response[i].result.data);
  }
 _free(response);

  if (!is_valid) {
    // this means all of the responses failed or could not be verified
    if (ctx->attempt< ctx->client->max_attempts-1) {
      ctx->attempt++;
      // clean up old results
      if (ctx->responses) _free(ctx->responses);
      if (ctx->response_context) { 
        _free(ctx->response_context->c);
        _free(ctx->response_context);
      }
      if (ctx->requests_configs ) {
          for (i=0;i<ctx->len;i++) {
              if (ctx->requests_configs[i].signaturesCount) 
               _free(ctx->requests_configs[i].signatures);
          }
      }
      ctx->responses = NULL;
      ctx->response_context = NULL;

      // now try again
      return  in3_send_ctx(ctx);
    }
    else 
      // we give up
      return ctx->client->max_attempts==1 ? -1 :  ctx_set_error(ctx, "reaching max_attempts and giving up",IN3_ERR_MAX_ATTEMPTS);
  }
  else 
    // we have a result
    return 0;
}

