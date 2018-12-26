#include "client.h"
#include "nodelist.h"
#include "context.h"
#include "cache.h"
#include <time.h>
#include <stdio.h>
#include "send.h"
#include "../util/mem.h"
#include "keys.h"
#include "../util/data.h"

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

static int in3_client_fill_chain(in3_chain_t* chain, in3_ctx_t* ctx, d_token_t* result) {
  int i,res = 0,len;

  // read the nodes
  d_token_t* t, *nodes = d_get(result,K_NODES), *node=NULL;
  if (!nodes || d_type(nodes)!=T_ARRAY) 
    return ctx_set_error(ctx, "No Nodes in the result", -1); 

  if (!(t=d_get(result,K_LAST_BLOCK_NUMBER))) 
    return ctx_set_error(ctx, "LastBlockNumer is missing", -1);

  // update last blockNumber
  chain->lastBlock = d_long(t);

  // new nodelist
  in3_node_t* newList = _calloc((len = d_len( nodes )), sizeof(in3_node_t));

   // set new values
  for (i=0;i<len;i++) {
    in3_node_t* n   = newList+i;
    node = node ? d_next(node) : d_get_at(nodes,i);
    if (!node) {
       res = ctx_set_error(ctx, "node missing", -1); 
       break;
    }
    
    n->capacity = d_get_intkd(node,K_CAPACITY,1); 
    n->index    = d_get_intkd(node,K_INDEX,i);
    n->deposit  = d_get_longk(node,K_DEPOSIT); 
    n->props    = d_get_longkd(node,K_PROPS,65535); 
    n->url      = d_get_stringk(node,K_URL);
    
    if (n->url)
      n->url = _strdup(n->url,-1);
    else  {
        res = ctx_set_error(ctx,"missing url in nodelist",-1);
        break;
    }

    n->address  = d_get_bytesk( node, K_ADDRESS);
    if (n->address)
      n->address = b_dup(n->address);
    else  {
        res = ctx_set_error(ctx,"missing address in nodelist",-1);
        break;
    }
  }

  if (res==0) {
       // successfull, so we can update the chain.
       free_nodeList(chain->nodeList, chain->nodeListLength);
       chain->nodeList       = newList;
       chain->nodeListLength = len;

      _free(chain->weights);
       chain->weights = _calloc(len,sizeof(in3_node_weight_t));
       for (i=0;i<len;i++)
         chain->weights[i].weight = 1;
   }
   else 
      free_nodeList(newList, len);

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
  in3_proof_t old_proof = c->proof;
  c->signatureCount=0;
  c->proof = PROOF_NONE;

  // new client
  in3_ctx_t* ctx = new_ctx(c,req); 
  if (ctx->error) 
    res = ctx_set_error(parent_ctx,ctx->error,-1);
  else  {
    res = in3_send_ctx(ctx);
    if (res>=0) {
      d_token_t* r = d_get(ctx->responses[0],K_RESULT);
      if (r) {
        // we have a result....
        res = in3_client_fill_chain(chain,ctx,r);
        if (res<0)
          res = ctx_set_error(parent_ctx,"Error updating node_list",ctx_set_error(parent_ctx,ctx->error,res));
      }
      else if (ctx->error) 
        res = ctx_set_error(parent_ctx,"Error updating node_list",ctx_set_error(parent_ctx,ctx->error,-1));
      else if ((r = d_get(ctx->responses[0],K_ERROR))) {
        if (d_type(r)==T_OBJECT) {
          str_range_t s = d_to_json(r);
          strncpy(req,s.data,s.len);
          res = ctx_set_error(parent_ctx,"Error updating node_list",ctx_set_error(parent_ctx,req,-1));
        }
        else
          res = ctx_set_error(parent_ctx,"Error updating node_list",ctx_set_error(parent_ctx,d_string(r),-1));
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
  c->proof = old_proof;
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

