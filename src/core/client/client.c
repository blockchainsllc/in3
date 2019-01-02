#include "client.h"
#include "../util/data.h"
#include "context.h"
#include "keys.h"
#include "send.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int in3_client_rpc(in3_t* c, char* method, char* params, char** result, char** error) {
  int  res = 0;
  char req[10000];
  sprintf(req, "{\"method\":\"%s\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":%s}", method, params);

  in3_ctx_t*  ctx = new_ctx(c, req);
  str_range_t s;
  if (result) result[0] = 0;
  error[0] = 0;

  if (ctx->error) {
    if (error != NULL) {
      *error = _malloc(strlen(ctx->error) + 1);
      strcpy(*error, ctx->error);
    }
    res = -1;
  } else {
    res = in3_send_ctx(ctx);
    if (res >= 0) {

      d_token_t* r = d_get(ctx->responses[0], K_RESULT);
      if (r) {
        if (result)
          *result = d_create_json(r);
      } else if ((r = d_get(ctx->responses[0], K_ERROR))) {
        if (d_type(r) == T_OBJECT) {
          s      = d_to_json(r);
          *error = _malloc(s.len + 1);
          strncpy(*error, s.data, s.len);
        } else {
          *error = _malloc(d_len(r) + 1);
          strncpy(*error, d_string(r), d_len(r));
        }
      } else if (ctx->error) {
        *error = _malloc(strlen(ctx->error) + 1);
        strcpy(*error, ctx->error);
      } else {
        *error = _malloc(50);
        strcpy(*error, "No Result and also no error");
      }

    } else if (ctx->error) {
      *error = _malloc(strlen(ctx->error) + 1);
      strcpy(*error, ctx->error);
    } else {
      *error = _malloc(50);
      strcpy(*error, "Error sending the request");
    }
  }
  free_ctx(ctx);
  return res;
}
