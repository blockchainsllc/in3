#include <stdlib.h>  
#include <string.h>
#include <stdio.h>
#include <stdint.h>  
#include "client.h"  
#include "../jsmn/jsmnutil.h"
#include "../util/utils.h"
#include <errno.h> 
#include "context.h"




static int in3_client_send_intern( in3* c, in3_ctx_t* ctx) {
  int i,l,p=1;
  jsmntok_t* t;

  char* buf = malloc(1000);
  buf[0]='[';

  // create response
  for (i=0;i< ctx->len;i++) {
    if (i>0) buf[p++]=',';
    memcpy(buf+p,"{\"result\":\"",11);
    p+=11;
    if ((t=ctx_get_token(ctx->request_data, ctx->requests[i],"method")))
      p+=ctx_cpy_string(ctx->request_data,t,buf+p);
    else {
      memcpy(buf+p,"none",4);
      p+=4;
    }
    buf[p++]='\"';
    buf[p++]='}';
  }
  buf[p++]=']';
  buf[p++]=0;

  return ctx_parse_response(ctx,buf);
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

  free_ctx(ctx);


  return res;

}



int get_node_list(in3* c, u_int64_t chain_id, bool update,  in3_node_t** nodeList, u_int32_t* nodeListLength ) {
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
      return 0;
    }
  }

  return IN3_ERR_CHAIN_NOT_FOUND;
}
