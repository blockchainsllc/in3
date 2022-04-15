#include "../../core/client/keys.h"
#include "../../core/client/request_internal.h"
#include "../recorder/recorder.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>

void sb_to_yaml(sb_t* sb, d_token_t* t, int level, bool as_array_item);

typedef struct {
  in3_plugin_act_fn transport;
  FILE*             f;
  in3_plugin_act_fn cache;
  sb_t              mocks;
  sb_t              config;
  int               mock_count;
} recorder_t;

static recorder_t rec = {0};

static in3_ret_t recorder_transport_out(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  UNUSED_VAR(plugin_data);
  in3_http_request_t* req = plugin_ctx;
  in3_ret_t           res = rec.transport(NULL, action, plugin_ctx);
  if (action != PLGN_ACT_TRANSPORT_CLEAN) {
    if (!rec.mock_count) sb_add_chars(&rec.mocks, "\n      mockedResponses:");

    rec.mock_count++;
    sb_printx(&rec.mocks, "\n        - *mock_%u", rec.mock_count);

    fprintf(rec.f, "\n\n  - &mock_%i\n    req:\n      method: %s\n      url: %s", rec.mock_count, req->method, req->urls[0]);
    bool is_array = true;
    if (req->payload_len) {
      json_ctx_t* ctx = parse_json(req->payload);
      is_array        = d_type(ctx->result) == T_ARRAY && d_len(ctx->result) != 1;
      fprintf(rec.f, "\n      body:");
      sb_t sb = {0};
      sb_to_yaml(&sb, is_array ? ctx->result : d_get_at(ctx->result, 0), 4, false);
      fprintf(rec.f, "%s", sb.data);
      json_free(ctx);
      _free(sb.data);
    }
    in3_response_t* r = req->req->raw_response;
    if (r->time) {
      char*       data = r->data.data ? r->data.data : "";
      json_ctx_t* ctx  = parse_json(data);
      if (ctx) {
        sb_t sb = {0};
        sb_to_yaml(&sb, is_array ? ctx->result : d_get_at(ctx->result, 0), 3, false);
        fprintf(rec.f, "\n    res:%s", sb.data);
        _free(sb.data);
        json_free(ctx);
      }
      else
        fprintf(rec.f, "\n    res: '%s'", data);
    }
  }
  return res;
}

in3_ret_t storage_out(void* data, in3_plugin_act_t action, void* arg) {
  UNUSED_VAR(data);
  UNUSED_VAR(action);
  UNUSED_VAR(arg);
  return IN3_EIGNORE;
}

static in3_plugin_t* get_plugin(in3_t* c, in3_plugin_act_t action) {
  for (in3_plugin_t* p = c->plugins; p; p = p->next) {
    if (p->acts & action) return p;
  }
  return NULL;
}

void recorder_write_start(in3_t* c, char* file, int argc, char* argv[]) {
  UNUSED_VAR(argc);
  UNUSED_VAR(argv);
  in3_plugin_t* p = get_plugin(c, PLGN_ACT_TRANSPORT_SEND);
  rec.transport   = p ? p->action_fn : NULL;
  rec.f           = fopen(file, "w");
  if (p) p->action_fn = recorder_transport_out;
  p = get_plugin(c, PLGN_ACT_CACHE_GET);
  if (p) {
    rec.cache    = p->action_fn;
    p->action_fn = storage_out;
  }
}

void recorder_update_cmd(char* file, int* argc, char** argv[]) {
  UNUSED_VAR(file);
  UNUSED_VAR(argc);
  UNUSED_VAR(argv);
}

void recorder_print(int err, const char* fmt, ...) {
  UNUSED_VAR(err);
  UNUSED_VAR(fmt);
  va_list args;
  va_start(args, fmt);
  vfprintf(err ? stderr : stdout, fmt, args);
  va_end(args);
}

void recorder_exit(int code) {
  if (rec.f) {
    fclose(rec.f);
  }
  _free(rec.config.data);
  _free(rec.mocks.data);
  exit(code);
}

void recorder_request(char* req) {
  if (!rec.f) return;

  fprintf(rec.f, "_defines:\n");
  if (rec.config.len) {
    sb_t sb = {0};
    fprintf(rec.f, "  - &config");
    rec.config.data[0] = '{';
    sb_add_char(&rec.config, '}');
    json_ctx_t* ctx = parse_json(rec.config.data);
    sb_to_yaml(&sb, ctx->result, 2, false);
    json_free(ctx);
    fprintf(rec.f, "%s", sb.data);
    _free(sb.data);
  }
  json_ctx_t* ctx = parse_json(req);
  char*       m   = d_get_string(ctx->result, K_METHOD);
  char*       api = strchr(m, '_');
  if (!api)
    api = "api";
  else {
    char* t = alloca(api - m + 1);
    memcpy(t, m, api - m);
    t[api - m] = 0;
    api        = t;
  }

  sb_printx(&rec.mocks, "\n\n%s:\n  %s:\n    - extra: run %s\n      config: *config\n      input:", api, m, m);
  sb_to_yaml(&rec.mocks, d_get(ctx->result, K_PARAMS), 4, false);
  json_free(ctx);
}

void recorder_response(char* req) {
  if (!rec.f) return;
  fprintf(rec.f, "%s\n      expected_output:", rec.mocks.data);
  sb_t        sb  = {0};
  json_ctx_t* ctx = parse_json(req);
  sb_to_yaml(&sb, ctx->result, 4, false);
  json_free(ctx);
  fprintf(rec.f, "%s", sb.data);
  _free(sb.data);
}
void recorder_error(char* req) {
  if (!rec.f) return;
  fprintf(rec.f, "%s\n      expected_failure: '%s'", rec.mocks.data, req);
}
void recorder_configure(char* conf) {
  sb_add_char(&rec.config, rec.config.len ? ',' : ' ');
  sb_add_range(&rec.config, conf, 1, strlen(conf) - 2);
}
void recorder_read_start(in3_t* c, char* file) {
  UNUSED_VAR(c);
  UNUSED_VAR(file);
  recorder_print(true, "-fi not supported for testcase\n");
  exit(-1);
}
