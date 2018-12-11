#include <stdlib.h>  
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>  
#include "client.h"  
#include "../jsmn/jsmnutil.h"
#include "../util/utils.h"
#include <errno.h> 
#include "context.h"
#include <time.h>

static node_weight_t*  fill_weight_list(in3* c, in3_node_t* all_nodes, in3_node_weight_t* weights, int len, time_t now, float* total_weight, int* total_found) {
  int i,p;
  float s=0;
  in3_node_t* nodeDef;
  in3_node_weight_t* weightDef;
  node_weight_t* prev=NULL;
  node_weight_t* w=NULL;
  node_weight_t* first=NULL;

  for (i=0,p=0;i<len;i++) {
    nodeDef = all_nodes+i;
    if (nodeDef->deposit<c->minDeposit)  continue;
    weightDef = weights +i;
    if (weightDef->blacklistedUntil > now )  continue;
    w = malloc(sizeof(node_weight_t));
    if (!first) first = w;
    w->node = nodeDef;
    w->weight = weightDef;
    w->next = NULL;
    w->s = s;
    w->w = weightDef->weight * nodeDef->capacity * (500 / ( weightDef->response_count ? ( weightDef -> total_response_time / weightDef->response_count ) : 500));
    s+=w->w;
    p++;
    if (prev) prev->next=w;
    prev = w;
  } 
  *total_weight=s;
  *total_found=p;
  return first;
}

static int in3_get_nodes(in3* c, in3_ctx_t* ctx, node_weight_t** nodes) {

  // get all nodes from the nodelist
  time_t now = time(0);
  in3_node_t* all_nodes;
  in3_node_weight_t* weights;
  float total_weight;


  int all_nodes_len, res, total_found,i,l;
  res = in3_client_get_node_list(c, c->chainId, false, &all_nodes, &all_nodes_len, &weights);
  if (res<0) 
     return ctx_set_error(ctx,"could not find the chain",res);


  // filter out nodes
  node_weight_t* found = fill_weight_list(c,all_nodes,weights,all_nodes_len,now,&total_weight,&total_found);

  if (total_found==0) {
    // no node available, so we should check if we can retry some blacklisted
    int blacklisted=0;
    for (i=0;i<all_nodes_len;i++) {
      if (weights[i].blacklistedUntil>now) blacklisted++;
    }

    // if morethan 50% of the nodes are blacklisted, we remove the mark and try again
    if (blacklisted>all_nodes_len/2) {
      for (i=0;i<all_nodes_len;i++) 
         weights[i].blacklistedUntil=0;
      found = fill_weight_list(c,all_nodes,weights,all_nodes_len,now,&total_weight,&total_found);
    }

    if (total_found==0) 
      return ctx_set_error(ctx,"No nodes found that match the criteria", IN3_ERR_NO_NODES_FOUND);
  }

  l = total_found < c->requestCount ? total_found : c->requestCount;
  if (total_found==l) {
    *nodes = found;
    return 0;
  }

  node_weight_t* last=NULL;
  node_weight_t* first=NULL;
  node_weight_t* wn=NULL;
  float r;
  int added=0;
  node_weight_t* w=NULL;

  // we want ot make sure this loop is run only max 10xthe number of requested nodes
  for (i=0;added<l && i<l*10;i++) {
    // pick a random number
    r = ( total_weight * (float)rand())/RAND_MAX;

    // find the first node matching it.
    w = found;
    while (w) {
      if (w->s<=r && w->s+w->w>=r) break;
      w = w->next;
    }

    if (w) {
      // check if we already added it,
      wn = first;
      while (wn) {
        if (wn->node==w->node) break;
        wn = wn->next;
      }

      if (!wn) {
        added++;
        wn = calloc(1,sizeof(node_weight_t));
        wn->s = w->s;
        wn->w = w->w;
        wn->weight = w->weight;
        wn->node = w->node;

        if (last) last->next=wn;
        if (!first) first = wn;
      }

    }
  }

  *nodes = first;
  free_ctx_nodes(found);

  // select them based on random
  return res;
}

static int configure_request(in3* c, in3_ctx_t* ctx, in3_request_config_t* conf, jsmntok_t* req ) {
   int i;
   conf->chainId = c->chainId;
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
       int res = in3_get_nodes(c,ctx, &sig_nodes);
       if (res<0) 
         return ctx_set_error(ctx,"Could not find any nodes for requesting signatures",IN3_ERR_NO_NODES_FOUND);
       int node_count = ctx_nodes_len(sig_nodes);
       conf->signaturesCount = node_count;
       conf->signatures = malloc(sizeof(bytes_t)*node_count);
       node_weight_t* w= sig_nodes;
       for (i=0;i<node_count;i++) {
         conf->signatures[i].len = w->node->address->len;
         conf->signatures[i].data = w->node->address->data;
       }
       free_ctx_nodes(sig_nodes);
     }
   }
   return 0;
}

static int verify_response( in3* c, in3_ctx_t* ctx, jsmntok_t* request, in3_request_config_t* conf, jsmntok_t* response) {
  // TODO verify
  return 0;
}

static int in3_client_send_intern( in3* c, in3_ctx_t* ctx) {
  // find the nodes to send the request to
  int i,n,nodes_count;
  int res = in3_get_nodes(c,ctx, &ctx->nodes);
  if (res<0)
    return ctx_set_error(ctx, "could not find any node",res);
  nodes_count = ctx_nodes_len(ctx->nodes);

  // configure the requests
  for (i=0;i<ctx->len;i++) {
    res = configure_request(c,ctx,ctx->requests_configs+i,ctx->requests[i]);
    if (res<0) 
       return ctx_set_error(ctx, "error configuring the config for request",res);
  }
  // now send the request
  if (!c->transport)
     return ctx_set_error(ctx, "no transport set",IN3_ERR_CONFIG_ERROR);


  // prepare the payload
  int payload_buffer_size = strlen(ctx->request_data)*4;
  char* payload = malloc(payload_buffer_size);
  char** urls = malloc(sizeof(char*)* nodes_count);
  node_weight_t* w = ctx->nodes;
  for (n=0;n<nodes_count;n++) {
    printf("nodeUrl %i:%s",n,w->node->url);
    urls[n]=w->node->url;
    w=w->next;
  }
  res = ctx_create_payload(ctx, payload,payload_buffer_size);
  if (res<0)  {
     free(payload);
     free(urls);
    return ctx_set_error(ctx,"could not generate the payload",IN3_ERR_CONFIG_ERROR);
  }

  in3_response_t* response = c->transport(urls,payload,nodes_count);
  free(payload);
  free(urls);
  if (!response) 
      return ctx_set_error(ctx, "no response from transport",IN3_ERR_CONFIG_ERROR);

  // verify responses
  w = ctx->nodes;
  for (n=0;n<nodes_count;n++) {
    if (response[n].error || !response[n].result) {
      // blacklist the node
      w->weight->blacklistedUntil = time(0) + 3600000;
      w->weight=0;
    }
    else {
      // we need to clean up the prev ios responses if set
      if (ctx->responses) free(ctx->responses);
      if (ctx->tok_res)   free(ctx->tok_res);

      // parse the result
      res = ctx_parse_response(ctx,response[n].result);
      if (res<0) {
        // blacklist!
        w->weight->blacklistedUntil = time(0) + 3600000;
        w->weight=0;
      }
      else {
        // check each request
        for (i=0;i<ctx->len;i++) {
          if (verify_response(c,ctx,ctx->requests[i], ctx->requests_configs+i, ctx->responses[i])<0) {
            // blacklist!
            w->weight->blacklistedUntil = time(0) + 3600000;
            w->weight=0;
            break;
          }
        } 
      }
    }
    if (w->weight>0) 
      // this reponse was successfully verified, so let us keep it.
      break;
    w=w->next;
  }

  // clean up responses
  for (i=0;i<nodes_count;i++) {
    if (response[i].error) free(response[i].error);
    if (response[i].result && response[i].result!=ctx->response_data) free(response[i].result);
  }
  free(response);

  if (!w) {
    // this means all of the responses failed or could not be verified
    if (ctx->attempt< c->max_attempts) {
      ctx->attempt++;
      // clean up old results
      if (ctx->response_data) free(ctx->response_data);
      if (ctx->responses) free(ctx->responses);
      if (ctx->tok_res)   free(ctx->tok_res);
      if (ctx->requests_configs ) {
          for (i=0;i<ctx->len;i++) {
              if (ctx->requests_configs[i].signaturesCount) 
                free(ctx->requests_configs[i].signatures);
          }
      }
      ctx->response_data = NULL;
      ctx->responses = NULL;
      ctx->tok_res = NULL;

      // now try again
      return  in3_client_send_intern(c,ctx);
    }
    else 
      // we give up
      return ctx_set_error(ctx, "reaching max_attempts and giving up",IN3_ERR_MAX_ATTEMPTS);
  
  }
  else 
    // we have a result
    return 0;
}


int in3_client_send( in3* c, char* req, char* result, int buf_size) {
  int res=0, len,p,i;
  in3_ctx_t* ctx = new_ctx(req); 

  if (ctx->error) 
    res=-1;
  else if (ctx->tok_req->type==JSMN_ARRAY) {
    res = in3_client_send_intern(c,ctx);
    // create the results if it was succesful
    if (res>=0) {
      result[0]='[';
      for (p=1,i=0,len=0;i<ctx->len;i++) {
        if (i>0) result[p++]=',';
        len = ctx->responses[i]->end-ctx->responses[i]->start;
        if (p+len>buf_size) {
          res = IN3_ERR_BUFFER_TOO_SMALL;
          break;
        }
        p+=ctx_cpy_string(ctx->response_data,ctx->responses[i],result+p);
      }
      result[p++]=']';
      result[p]=0;
    }
  }
  else if (ctx->tok_req->type==JSMN_OBJECT) {
    res = in3_client_send_intern(c,ctx);
    if (res>=0) 
      result[ctx_cpy_string(ctx->response_data, ctx->responses[0],result)]=0;
  }
  else
    res = 0;

  printf("\n error: %s\n",ctx->error);

  free_ctx(ctx);


  return res;

}



int in3_client_get_node_list(in3* c, u_int64_t chain_id, bool update,  in3_node_t** nodeList, int* nodeListLength, in3_node_weight_t** weights ) {
  int i;
  in3_chain_t* chain;
  for (i=0;i<c->serversCount;i++) {
    chain = c->servers + i;
    if (chain->chainId == chain_id) {
      if (chain->needsUpdate || update) {
        chain->needsUpdate=false;
        // now update the nodeList

/*

        
      // create a random seed which ensures the deterministic nature of even a partly list.
      const seed = '0x' + keccak256('0x' + Math.round(Math.random() * Number.MAX_SAFE_INTEGER).toString(16)).toString('hex')

      const nlResponse = await this.sendRPC(
        'in3_nodeList',
        [this.defConfig.nodeLimit || 0, seed, servers.initAddresses || []],
        chain, conf)
      const nl = nlResponse.result as ServerList

*/

      }
      *nodeListLength = chain->nodeListLength;
      *nodeList = chain->nodeList;
      *weights = chain->weights;
      return 0;
    }
  }

  return IN3_ERR_CHAIN_NOT_FOUND;
}
