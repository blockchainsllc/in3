#include "req_exec.h"
#include "../../tools/recorder/recorder.h"
#include "../http-server/http_server.h"
#include "helper.h"
req_exec_t* get_req_exec() {
  static req_exec_t val = {0};
  return &val;
}

static void execute(in3_t* c, FILE* f) {
  if (feof(f)) die("no data");
  sb_t* sb    = sb_new(NULL);
  char  first = 0, stop = 0;
  int   level = 0, d = 0;
  while (1) {
    d = fgetc(f);
    if (d == EOF) {
      if (first)
        die("Invalid json-data from stdin");
      else
        recorder_exit(EXIT_SUCCESS);
    }
    if (first == 0) {
      if (d == '{')
        stop = '}';
      else if (d == '[')
        stop = ']';
      else
        continue;
      first = d;
    }

    sb_add_char(sb, (char) d);
    if (d == first) level++;
    if (d == stop) level--;
    if (level == 0) {
      // time to execute
      in3_req_t* ctx = req_new(c, sb->data);
      if (ctx->error)
        recorder_print(0, "{\"jsonrpc\":\"2.0\",\"id\":%i,\"error\":{\"code\":%i,\"message\":\"%s\"}\n", 1, ctx->verification_state, ctx->error);
      else {
        in3_ret_t ret = in3_send_req(ctx);
        uint32_t  id  = d_get_int(ctx->requests[0], K_ID);
        if (ctx->error) {
          for (char* x = ctx->error; *x; x++) {
            if (*x == '\n') *x = ' ';
          }
        }

        if (ret == IN3_OK) {
          if (c->flags & FLAGS_KEEP_IN3) {
            str_range_t rr  = d_to_json(ctx->responses[0]);
            rr.data[rr.len] = 0;
            recorder_print(0, "%s\n", rr.data);
          }
          else {
            d_token_t* result = d_get(ctx->responses[0], K_RESULT);
            d_token_t* error  = d_get(ctx->responses[0], K_ERROR);
            char*      r      = d_create_json(ctx->response_context, result ? result : error);
            if (result)
              recorder_print(0, "{\"jsonrpc\":\"2.0\",\"id\":%i,\"result\":%s}\n", id, r);
            else
              recorder_print(0, "{\"jsonrpc\":\"2.0\",\"id\":%i,\"error\":%s}\n", id, r);
            _free(r);
          }
        }
        else
          recorder_print(0, "{\"jsonrpc\":\"2.0\",\"id\":%i,\"error\":{\"code\":%i,\"message\":\"%s\"}}\n", id, ctx->verification_state, ctx->error == NULL ? "Unknown error" : ctx->error);
      }
      fflush(stdout);
      req_free(ctx);
      first   = 0;
      sb->len = 0;
    }
  }
}

void check_server(in3_t* c) {
  if (get_req_exec()->port) {
#ifdef IN3_SERVER
    // start server
    http_run_server(get_req_exec()->port, c, get_req_exec()->allowed_methods);
    recorder_exit(0);
#else
    die("You need to compile in3 with -DIN3_SERVER=true to start the server.");
#endif
  }
  else {
    in3_log_info("in3 " IN3_VERSION " - reading json-rpc from stdin. (exit with ctrl C)\n________________________________________________\n");
    execute(c, stdin);
    recorder_exit(0);
  }
}