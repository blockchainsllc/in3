#include <stdlib.h>  
#include <string.h>
#include <stdio.h>
#include "client.h"  
#include <errno.h> 
#include "context.h"
#include "send.h"


int in3_client_send( in3_t* c, char* req, char* result, int buf_size, char* error) {
  int res=0, len,p,i;
  in3_ctx_t* ctx = new_ctx(c,req); 
  result[0]=0;
  error[0]=0;

  if (ctx->error) {
    if (error!=NULL) strcpy(error,ctx->error);
    res=-1;
  }
  else if (ctx->tok_req->type==JSMN_ARRAY) {
    res = in3_send_ctx(ctx);
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
    res = in3_send_ctx(ctx);
    if (res>=0) 
      result[ctx_cpy_string(ctx->response_data, ctx->responses[0],result)]=0;
  }
  else
    res = 0;

//  printf("\n error: %s\n",ctx->error);

  free_ctx(ctx);


  return res;

}

int in3_client_rpc(in3_t* c, char* method, char* params ,char** result, char** error) {
  int res=0;
  char req[10000];
  sprintf(req,"{\"method\":\"%s\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":%s}",method,params);


  in3_ctx_t* ctx = new_ctx(c, req); 
  result[0]=0;
  error[0]=0;

  if (ctx->error) {
    if (error!=NULL) {
      *error = malloc(strlen(ctx->error)+1);
      strcpy(*error,ctx->error);
    }
    res=-1;
  }
  else  {
    res = in3_send_ctx(ctx);
    if (res>=0) {

      jsmntok_t* r = ctx_get_token(ctx->response_data, ctx->responses[0],"result");
      if (r) {
        *result = malloc(r->end - r->start +1);
        ctx_cpy_string(ctx->response_data,r,*result);
      }
      else if ((r = ctx_get_token(ctx->response_data,ctx->responses[0],"error"))) {
        *error = malloc(r->end - r->start +1);
        ctx_cpy_string(ctx->response_data,r,*error);
      }
      else if (ctx->error) {
        *error = malloc(strlen(ctx->error)+1);
        strcpy(*error,ctx->error);
      }
      else {
        *error = malloc(50);
        strcpy(*error,"No Result and also no error");
      }

    }
    else if (ctx->error) {
      *error = malloc(strlen(ctx->error)+1);
      strcpy(*error,ctx->error);
    }
    else {
        *error = malloc(50);
        strcpy(*error,"Error sending the request");
    }
  }
  free_ctx(ctx);
  return res;
}


