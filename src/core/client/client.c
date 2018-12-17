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
#include "cache.h"
#include <time.h>
#include "../util/stringbuilder.h"
#include "verifier.h"


static int in3_client_send_intern( in3_ctx_t* ctx);
static void free_nodeList(in3_node_t* nodeList, int count) {
  int i;

     // clean chain..
   for (i=0;i<count;i++) {
     if (nodeList[i].url) free(nodeList[i].url);
     if (nodeList[i].address) b_free(nodeList[i].address);
   }
   free(nodeList);
}

int in3_client_fill_chain(in3_chain_t* chain, in3_ctx_t* ctx,jsmntok_t* result) {
  jsmntok_t* t;
  char* res=ctx->response_data;
  int i,r=0;
  jsmntok_t* nodes = ctx_get_token(res, result,"nodes");
  if (!nodes || nodes->type!=JSMN_ARRAY) return ctx_set_error(ctx, "No Nodes in the result", -1); 

  if (!(t=ctx_get_token(res, result,"lastBlockNumber"))) return ctx_set_error(ctx, "LastBlockNumer is missing", -1);
  chain->lastBlock = ctx_to_long(res,t,chain->lastBlock);

  
   in3_node_t* newList = calloc(nodes->size, sizeof(in3_node_t));

   // set new values
   for (i=0;i<nodes->size;i++) {
     in3_node_t* n = newList+i;
     jsmntok_t* node = ctx_get_array_token(nodes,i);
     if (!node) 
        r = ctx_set_error(ctx, "node missing", -1); 
     else {
        
        n->capacity = ctx_to_int(res,ctx_get_token(res,node,"capacity"),1); 
        n->index = ctx_to_int(res,ctx_get_token(res,node,"index"),i); 
        n->deposit = ctx_to_long(res,ctx_get_token(res,node,"deposit"),0); 
        n->props = ctx_to_long(res,ctx_get_token(res,node,"props"),65535); 
        

        t=ctx_get_token(res,node,"url");
        if (t) {
          n->url = malloc(t->end - t->start +1);
          ctx_cpy_string(res,t,n->url);
        }
        else 
          r = ctx_set_error(ctx,"missing url in nodelist",-1);

        t=ctx_get_token(res,node,"address");
        if (t) 
          n->address = ctx_to_bytes(res,t,20);
        else
          r = ctx_set_error(ctx,"missing address in nodelist",-1);
     }
     if (r!=0) break;
   }

   if (r==0) {
       free_nodeList(chain->nodeList, chain->nodeListLength);
       chain->nodeList = newList;
       chain->nodeListLength = nodes->size;

       free(chain->weights);
       chain->weights=  calloc(nodes->size,sizeof(in3_node_weight_t));
       for (i=0;i<nodes->size;i++)
         chain->weights[i].weight = 1;
   }
   else 
      free_nodeList(newList, nodes->size);

  return r;
}

static int update_nodelist(in3* c,in3_chain_t* chain, in3_ctx_t* parent_ctx) {
  int res=0;

  // create random seed
  char seed[67];
  sprintf(seed,"0x%08x%08x%08x%08x%08x%08x%08x%08x",rand(),rand(),rand(),rand(),rand(),rand(),rand(),rand());

  // create request
  char req[10000];
  sprintf(req,"{\"method\":\"in3_nodeList\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":[%i,\"%s\",[]]}",c->nodeLimit, seed);

  uint16_t old_sig_count =  c->signatureCount;
  c->signatureCount=0;

  // new client
  in3_ctx_t* ctx = new_ctx(c,req); 
  if (ctx->error) 
    res = ctx_set_error(parent_ctx,ctx->error,-1);
  else  {
    res = in3_client_send_intern(ctx);
    if (res>=0) {
      jsmntok_t* r = ctx_get_token(ctx->response_data, ctx->responses[0],"result");
      if (r) {
        // we have a result....
        res = in3_client_fill_chain(chain,ctx,r);
        if (res<0)
          res = ctx_set_error(parent_ctx,"Error updating node_list",ctx_set_error(parent_ctx,ctx->error,res));
      }
      else if (ctx->error) 
        res = ctx_set_error(parent_ctx,"Error updating node_list",ctx_set_error(parent_ctx,ctx->error,-1));
      else if ((r = ctx_get_token(ctx->response_data,ctx->responses[0],"error"))) {
        ctx_cpy_string(ctx->response_data,r,req);
        res = ctx_set_error(parent_ctx,"Error updating node_list",ctx_set_error(parent_ctx,req,-1));
      }
      else
        res = ctx_set_error(parent_ctx,"Error updating node_list without any result",-1);
    }
    else if (ctx->error)
      res = ctx_set_error(parent_ctx,"Error updating node_list",ctx_set_error(parent_ctx,ctx->error,-1));
    else
      res = ctx_set_error(parent_ctx,"Error updating node_list without any result",-1);
  }

  if (!res && c->cacheStorage) 
     in3_cache_store_nodelist(ctx,chain);
  free_ctx(ctx);
  c->signatureCount=old_sig_count;
  return res;
}

static int get_node_list(in3_ctx_t* ctx, uint64_t chain_id, bool update,  in3_node_t** nodeList, int* nodeListLength, in3_node_weight_t** weights) {
  int i, res=IN3_ERR_CHAIN_NOT_FOUND;
  in3_chain_t* chain;
  in3* c = ctx->client;
  for (i=0;i< c->serversCount;i++) {
    chain = c->servers + i;
    if (chain->chainId == chain_id) {
      if (chain->needsUpdate || update) {
        chain->needsUpdate=false;

        // now update the nodeList
        res=update_nodelist(c,chain, ctx);
        if (res<0) break;

      }
      *nodeListLength = chain->nodeListLength;
      *nodeList = chain->nodeList;
      *weights = chain->weights;
      return 0;
    }
  }

  return res;
}


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

static int in3_get_nodes(in3_ctx_t* ctx, node_weight_t** nodes) {

  // get all nodes from the nodelist
  time_t now  = time(0);
  in3_node_t*        all_nodes;
  in3_node_weight_t* weights;
  float total_weight;
  int all_nodes_len, res, total_found,i,l;

  res = get_node_list(ctx,ctx->client->chainId, false, &all_nodes, &all_nodes_len, &weights);
  if (res<0) 
     return ctx_set_error(ctx,"could not find the chain",res);


  // filter out nodes
  node_weight_t* found = fill_weight_list(ctx->client,all_nodes,weights,all_nodes_len,now,&total_weight,&total_found);

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
      found = fill_weight_list(ctx->client,all_nodes,weights,all_nodes_len,now,&total_weight,&total_found);
    }

    if (total_found==0) 
      return ctx_set_error(ctx,"No nodes found that match the criteria", IN3_ERR_NO_NODES_FOUND);
  }

  l = total_found < ctx->client->requestCount ? total_found : ctx->client->requestCount;
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
    r = total_weight *  ((float)(rand() % 10000)) / 10000.0;

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

static int configure_request(in3_ctx_t* ctx, in3_request_config_t* conf, jsmntok_t* req ) {
   int i;
   in3* c         = ctx->client;

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
       int res = in3_get_nodes(ctx, &sig_nodes);
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

static int verify_response( in3_ctx_t* ctx, jsmntok_t* request, in3_request_config_t* conf, jsmntok_t* response) {
  // TODO verify
  return 0;
}

static int send_request(in3_ctx_t* ctx, int nodes_count,in3_response_t** response_result ) {
  int n,res;

  *response_result = NULL;
  // prepare the payload
  sb_t* payload = sb_new(NULL);

  // create url-array
  char** urls = malloc(sizeof(char*)* nodes_count);
  node_weight_t* w = ctx->nodes;
  for (n=0;n<nodes_count;n++) {
    urls[n]=w->node->url;
    w=w->next;
  }

  res = ctx_create_payload(ctx, payload);
  if (res<0)  {
     sb_free(payload);
     free(urls);
    return ctx_set_error(ctx,"could not generate the payload",IN3_ERR_CONFIG_ERROR);
  }

  // prepare response-object
  in3_response_t* response = malloc( sizeof(in3_response_t)*nodes_count );
  for (n=0;n<nodes_count;n++) {
    sb_init(&response[n].error);
    sb_init(&response[n].result);
  }

  // send requets 
  res = ctx->client->transport(urls,nodes_count, payload->data,response);

  // free resources
  sb_free(payload);
  free(urls);

  if (res<0) {
     for (n=0;n<nodes_count;n++) {
       free(response[n].error.data);
       free(response[n].result.data);
     }
     free(response);
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
      w->weight->blacklistedUntil = time(0) + 3600000;
      w->weight=0;
    }
    else {
      // we need to clean up the prev ios responses if set
      if (ctx->responses) free(ctx->responses);
      if (ctx->tok_res)   free(ctx->tok_res);

      // parse the result
      res = ctx_parse_response(ctx,response[n].result.data);
      if (res<0) {
        // blacklist!
        w->weight->blacklistedUntil = time(0) + 3600000;
        w->weight=0;
      }
      else {
//        printf("res:%s",ctx->response_data);
        // check each request
        for (i=0;i<ctx->len;i++) {
          vc.request=ctx->requests[i];
          vc.result = ctx_get_token(ctx->response_data, ctx->responses[i],"result");
          if ((vc.proof =ctx_get_token(ctx->response_data,  ctx->responses[i], "in3")))
            vc.proof=ctx_get_token(ctx->response_data,  vc.proof, "proof");
          vc.config = ctx->requests_configs +i;

          if (verifier && verifier->verify(&vc)) {
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
      return true;

    w=w->next;
  }
  return false;
    
}

static int in3_client_send_intern( in3_ctx_t* ctx) {
  // find the nodes to send the request to
  int i,n,nodes_count;

  in3_response_t* response;
  int res = in3_get_nodes(ctx, &ctx->nodes);
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
    free(response[i].error.data);
    if (response[i].result.data!=ctx->response_data) free(response[i].result.data);
  }
  free(response);

  if (!is_valid) {
    // this means all of the responses failed or could not be verified
    if (ctx->attempt< ctx->client->max_attempts) {
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
      return  in3_client_send_intern(ctx);
    }
    else 
      // we give up
      return ctx_set_error(ctx, "reaching max_attempts and giving up",IN3_ERR_MAX_ATTEMPTS);
  }
  else 
    // we have a result
    return 0;
}


int in3_client_send( in3* c, char* req, char* result, int buf_size, char* error) {
  int res=0, len,p,i;
  in3_ctx_t* ctx = new_ctx(c,req); 
  result[0]=0;
  error[0]=0;

  if (ctx->error) {
    if (error!=NULL) strcpy(error,ctx->error);
    res=-1;
  }
  else if (ctx->tok_req->type==JSMN_ARRAY) {
    res = in3_client_send_intern(ctx);
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
    res = in3_client_send_intern(ctx);
    if (res>=0) 
      result[ctx_cpy_string(ctx->response_data, ctx->responses[0],result)]=0;
  }
  else
    res = 0;

//  printf("\n error: %s\n",ctx->error);

  free_ctx(ctx);


  return res;

}

int in3_client_rpc(in3* c, char* method, char* params ,char* result, int buf_size, char* error) {
  int res=0, len,p,i;
  char req[10000];
  sprintf(req,"{\"method\":\"%s\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":%s}",method,params);


  in3_ctx_t* ctx = new_ctx(c, req); 
  result[0]=0;
  error[0]=0;

  if (ctx->error) {
    if (error!=NULL) strcpy(error,ctx->error);
    res=-1;
  }
  else  {
    res = in3_client_send_intern(ctx);
    if (res>=0) {

      jsmntok_t* r = ctx_get_token(ctx->response_data, ctx->responses[0],"result");
      if (r)
        ctx_cpy_string(ctx->response_data,r,result);
      else if ((r = ctx_get_token(ctx->response_data,ctx->responses[0],"error")))
        ctx_cpy_string(ctx->response_data,r,error);
      else if (ctx->error)
        strcpy(error,ctx->error);
      else
        strcpy(error,"No Result and also no error");

    }
    else if (ctx->error)
      strcpy(error,ctx->error);
    else
      strcpy(error,"Error sending the request");
  }
  free_ctx(ctx);
  return res;
}


