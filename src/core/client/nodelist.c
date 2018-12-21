#include "client.h"
#include "nodelist.h"
#include "context.h"
#include "cache.h"
#include <time.h>
#include <stdio.h>
#include "send.h"
#include "../util/mem.h"

#ifdef __TEST__

#endif

static void free_nodeList(in3_node_t* nodeList, int count) {
   int i;

     // clean chain..
   for (i=0;i<count;i++) {
     if (nodeList[i].url)       _free(nodeList[i].url);
     if (nodeList[i].address) b_free(nodeList[i].address);
   }
   _free(nodeList);
}

static int in3_client_fill_chain(in3_chain_t* chain, in3_ctx_t* ctx,jsmntok_t* result) {
  char* response = ctx->response_data;
  int i,res = 0;

  // read the nodes
  jsmntok_t* t, *nodes = ctx_get_token(response, result,"nodes");
  if (!nodes || nodes->type!=JSMN_ARRAY) 
    return ctx_set_error(ctx, "No Nodes in the result", -1); 

  if (!(t=ctx_get_token(response, result,"lastBlockNumber"))) 
    return ctx_set_error(ctx, "LastBlockNumer is missing", -1);

  // update last blockNumber
  chain->lastBlock = ctx_to_long(response,t,chain->lastBlock);

  // new nodelist
  in3_node_t* newList = _calloc(nodes->size, sizeof(in3_node_t));

   // set new values
  for (i=0;i<nodes->size;i++) {
    in3_node_t* n   = newList+i;
    jsmntok_t* node = ctx_get_array_token(nodes,i);
    if (!node) {
       res = ctx_set_error(ctx, "node missing", -1); 
       break;
    }
    
    n->capacity = ctx_to_int(response,ctx_get_token(response,node,"capacity"),1); 
    n->index    = ctx_to_int(response,ctx_get_token(response,node,"index"),i); 
    n->deposit  = ctx_to_long(response,ctx_get_token(response,node,"deposit"),0); 
    n->props    = ctx_to_long(response,ctx_get_token(response,node,"props"),65535); 
    
    t=ctx_get_token(response,node,"url");
    if (t) {
        n->url = _malloc(t->end - t->start +1);
        ctx_cpy_string(response,t,n->url);
    }
    else  {
        res = ctx_set_error(ctx,"missing url in nodelist",-1);
        break;
    }

    t=ctx_get_token(response,node,"address");
    if (t) 
        n->address = ctx_to_bytes(response,t,20);
    else {
        res = ctx_set_error(ctx,"missing address in nodelist",-1);
        break;
    }
  }

  if (res==0) {
       // successfull, so we can update the chain.
       free_nodeList(chain->nodeList, chain->nodeListLength);
       chain->nodeList       = newList;
       chain->nodeListLength = nodes->size;

      _free(chain->weights);
       chain->weights = _calloc(nodes->size,sizeof(in3_node_weight_t));
       for (i=0;i<nodes->size;i++)
         chain->weights[i].weight = 1;
   }
   else 
      free_nodeList(newList, nodes->size);

  return res;
}

static int update_nodelist(in3_t* c, in3_chain_t* chain, in3_ctx_t* parent_ctx) {
  int res=0;

  // create random seed
  char seed[67];
  sprintf(seed,"0x%08x%08x%08x%08x%08x%08x%08x%08x",_rand(),_rand(),_rand(),_rand(),_rand(),_rand(),_rand(),_rand());

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
    res = in3_send_ctx(ctx);
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

  if (res>=0 && c->cacheStorage) 
     in3_cache_store_nodelist(ctx,chain);
  free_ctx(ctx);
  c->signatureCount = old_sig_count;
  return res;
}

node_weight_t*  in3_node_list_fill_weight(in3_t* c, in3_node_t* all_nodes, in3_node_weight_t* weights, int len, _time_t now, float* total_weight, int* total_found) {
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
    w = _malloc(sizeof(node_weight_t));
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


int in3_node_list_get(in3_ctx_t* ctx, uint64_t chain_id, bool update,  in3_node_t** nodeList, int* nodeListLength, in3_node_weight_t** weights) {
  int i, res=IN3_ERR_CHAIN_NOT_FOUND;
  in3_chain_t* chain;
  in3_t* c = ctx->client;
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


int in3_node_list_pick_nodes(in3_ctx_t* ctx, node_weight_t** nodes) {

  // get all nodes from the nodelist
  _time_t now  = _time();
  in3_node_t*        all_nodes;
  in3_node_weight_t* weights;
  float total_weight;
  int all_nodes_len, res, total_found,i,l;

  res = in3_node_list_get(ctx,ctx->client->chainId, false, &all_nodes, &all_nodes_len, &weights);
  if (res<0) 
     return ctx_set_error(ctx,"could not find the chain",res);


  // filter out nodes
  node_weight_t* found = in3_node_list_fill_weight(ctx->client,all_nodes,weights,all_nodes_len,now,&total_weight,&total_found);

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
      found = in3_node_list_fill_weight(ctx->client,all_nodes,weights,all_nodes_len,now,&total_weight,&total_found);
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
    r = total_weight *  ((float)(_rand() % 10000)) / 10000.0;

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
        wn = _calloc(1,sizeof(node_weight_t));
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

/** removes all nodes and their weights from the nodelist */
int in3_nodelist_clear(in3_chain_t* chain) {
    int i;
    for (i=0;i<chain->nodeListLength;i++) {
        if (chain->nodeList[i].url)      
          _free(chain->nodeList[i].url);
        if (chain->nodeList[i].address) 
          b_free(chain->nodeList[i].address);
    }
   _free(chain->nodeList);
   _free(chain->weights);
    return 0;
}

