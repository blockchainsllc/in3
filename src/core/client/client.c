#include "client.h"
#include "../util/data.h"
#include "context.h"
#include "keys.h"
#include "send.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef __ZEPHYR__
  #define printX    printk
  #define fprintX   fprintf   // (kg): fprintk caused link-problems!
  #define snprintX  snprintk
#else
  #define printX    printf
  #define fprintX   fprintf
  #define snprintX  snprintf
#endif


in3_ctx_t* in3_client_rpc_ctx(in3_t* c, char* method, char* params) {
  // generate the rpc-request
  char req[strlen(method) + strlen(params) + 200];
  // sprintf(req, "{\"method\":\"%s\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":%s}", method, params);
  snprintX(req, sizeof(req), "{\"method\":\"%s\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":%s}", method, params);

  // create a new context by parsing the request
  in3_ctx_t* ctx = new_ctx(c, req);

  // this happens if the request is not parseable (JSON-error in params)
  if (ctx->error) return ctx;

  // execute it
  if (in3_send_ctx(ctx) == 0) {
    // the request was succesfull, so we delete interim errors (which can happen in case in3 had to retry)
    if (ctx->error) _free(ctx->error);
    ctx->error = NULL;
  }

  // return context and hope the calle will clean it.
  return ctx;
}

int in3_client_rpc(in3_t* c, char* method, char* params, char** result, char** error) {
  int res = 0;
  // prepare request
  char req[strlen(method) + strlen(params) + 200];
  // sprintf(req, "{\"method\":\"%s\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":%s}", method, params);
  snprintX(req, sizeof(req), "{\"method\":\"%s\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":%s}", method, params);



  // parse it
  in3_ctx_t*  ctx = new_ctx(c, req);
  str_range_t s;

  // make sure result & error are clean
  if (result) result[0] = 0;
  if (error) *error = NULL;

  // check parse-errors
  if (ctx->error) {
    if (error != NULL) {
      *error = _malloc(strlen(ctx->error) + 1);
      strcpy(*error, ctx->error);
    }
    res = -1;
  } else {
    // so far everything is good, so we send the request
    res = in3_send_ctx(ctx);
    if (res >= 0) {

      // looks good, so we get the result
      d_token_t* r = d_get(ctx->responses[0], K_RESULT);
      if (r) {
        // we have a result and copy it
        if (result) *result = d_create_json(r);
      } else if ((r = d_get(ctx->responses[0], K_ERROR))) {
        // the response was correct but contains a error-object, which we convert into a string
        if (d_type(r) == T_OBJECT) {
          s = d_to_json(r);
          if (error != NULL) {
            *error = _malloc(s.len + 1);
            strncpy(*error, s.data, s.len);
            (*error)[s.len ] = '\0';
          }
        } else {
          if (error != NULL) {
            *error = _malloc(d_len(r) + 1);
            strncpy(*error, d_string(r), d_len(r));
            (*error)[d_len(r) ] = '\0';
          }
        }
      } else if (ctx->error) {
        // we don't have a result, but an error, so we copy this
        if (error != NULL) {
          *error = _malloc(strlen(ctx->error) + 1);
          strcpy(*error, ctx->error);
        }
      } else {
        // should not happen
        if (error != NULL) {
          *error = _malloc(50);
          strcpy(*error, "No Result and also no error");
        }
      }

    } else if (ctx->error) {
      // there was an error, copy it
      if (error != NULL) {
        *error = _malloc(strlen(ctx->error) + 1);
        strcpy(*error, ctx->error);
      }
    } else {
      // something went wrong, but no error
      if (error != NULL) {
        *error = _malloc(50);
        strcpy(*error, "Error sending the request");
      }
    }
  }
  free_ctx(ctx);

  // if we have an error, we always return -1
  return *error ? -1 : res;
}
