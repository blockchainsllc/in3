#include "transport.h"
#include "../../src/core/client/keys.h"
#include "../../src/core/util/data.h"
#include "../../src/nodeselect/full/cache.h"
#include "../../src/nodeselect/full/nodelist.h"
#include "../test_utils.h"
#include <stdio.h>
#include <unistd.h>
#define MOCK_PATH TESTDATA_DIR "/mock/%s.json"

static void clean_json_str(char* s) {
  const char* d = s;
  do {
    while (*d == ' ' || *d == '\x09' || *d == '\t' | *d == '\n') {
      ++d;
    }

  } while ((*s++ = *d++));
}

typedef struct response_s {
  char*              request_method;
  char*              request_params;
  char*              response;
  struct response_s* next;
} response_t;

static response_t* response_buffer = NULL;
static response_t* responses;
char*              read_json_response_buffer(char* path) {

  size_t pl          = strlen(path);
  char*  path_native = alloca(pl + 1);
#ifdef WIN32
  for (int i = 0; i < pl + 1; i++) path_native[i] = path[i] == '/' ? '\\' : path[i];
#else
  memcpy(path_native, path, pl + 1);
#endif
  char* resp_buf;
  long  length;
  FILE* f = fopen(path_native, "r");
  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    resp_buf = _malloc(length + 1);
    fread(resp_buf, 1, length, f);
    resp_buf[length] = 0;
    fclose(f);
    return resp_buf;
  }
  else {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      printf("Current working dir: %s\n", cwd);
    }
    else {
      perror("getcwd() error");
    }
    printf("Error coudl not find the testdata %s\n", path_native);
    return NULL;
  }
}

void add_response(char* request_method, char* request_params, char* result, char* error, char* in3) {
  response_t* n     = calloc(1, sizeof(response_t));
  n->next           = responses;
  n->request_method = request_method;
  n->request_params = request_params;
  n->response       = malloc(40 + ((result || error) ? strlen(result ? result : error) : 0) + (in3 ? strlen(in3) + 10 : 0));
  if (in3)
    sprintf(n->response, "[{\"id\":1,\"jsonrpc\":\"2.0\",\"%s\":%s,\"in3\":%s}]", result ? "result" : "error", result ? result : error, in3);
  else
    sprintf(n->response, "[{\"id\":1,\"jsonrpc\":\"2.0\",\"%s\":%s}]", result ? "result" : "error", result ? result : error);

  responses = n;
}

static void clean_last_response() {
  if (response_buffer) {
    _free(response_buffer->request_method);
    _free(response_buffer->request_params);
    _free(response_buffer->response);
    _free(response_buffer);
    response_buffer = NULL;
  }
}
/* add response - request mock from json*/
int add_response_test(char* test, char* needed_params) {
  clean_last_response();
  char path[270];
  sprintf(path, MOCK_PATH, test);
  char*       buffer = read_json_response_buffer(path);
  json_ctx_t* mock   = parse_json(buffer);
  str_range_t res;
  d_token_t*  req;
  char*       params = NULL;
  if (d_type(mock->result) == T_OBJECT) {
    res    = d_to_json(d_get_at(d_get(mock->result, key("response")), 0));
    req    = d_get_at(d_get(mock->result, key("request")), 0);
    params = d_create_json(mock, d_get(req, key("params")));
  }
  else if (d_type(mock->result) == T_ARRAY) {
    for_children_of(iter, mock->result) {
      res    = d_to_json(d_get_at(d_get(iter.token, key("response")), 0));
      req    = d_get_at(d_get(iter.token, key("request")), 0);
      params = d_create_json(mock, d_get(req, key("params")));
      clean_json_str(params);
      if (strcmp(params, needed_params) != 0) {
        _free(params);
        params = NULL;
      }
      else
        break;
    }
  }

  if (params) {
    clean_json_str(params);

    response_buffer                 = _calloc(1, sizeof(response_t));
    response_buffer->request_method = _strdupn(d_get_string(req, key("method")), -1);
    response_buffer->request_params = params;
    response_buffer->response       = _strdupn(res.data, res.len);
  }

  json_free(mock);
  _free(buffer);
  return params ? 0 : -1;
}
in3_ret_t test_transport(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  UNUSED_VAR(plugin_data);
  UNUSED_VAR(action);

  in3_http_request_t* req = plugin_ctx;
  TEST_ASSERT_NOT_NULL_MESSAGE(responses, "no request registered");
  json_ctx_t* r = parse_json(req->payload);
  TEST_ASSERT_NOT_NULL_MESSAGE(r, "payload not parseable");
  d_token_t*  request = d_type(r->result) == T_ARRAY ? d_get_at(r->result, 0) : r->result;
  char*       method  = d_get_string(request, K_METHOD);
  str_range_t params  = d_to_json(d_get(request, key("params")));
  char*       p       = alloca(params.len + 1);
  strncpy(p, params.data, params.len);
  p[params.len] = 0;
  clean_json_str(p);

  response_t* resp = responses;
  for (int i = 0; i < req->urls_len; ++i) {
    TEST_ASSERT_EQUAL_STRING(resp->request_method, method);
    TEST_ASSERT_EQUAL_STRING(resp->request_params, p);
    in3_ctx_add_response(req->req, i, false, resp->response, -1, 0);
    _free(resp->response);
    responses = resp->next;
    _free(resp);
    resp = responses;
  }
  json_free(r);
  responses = resp;
  return IN3_OK;
}

in3_ret_t mock_transport(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  UNUSED_VAR(plugin_data);
  UNUSED_VAR(action);

  in3_http_request_t* req      = plugin_ctx;
  json_ctx_t*         r        = parse_json(req->payload);
  d_token_t*          request  = d_type(r->result) == T_ARRAY ? d_get_at(r->result, 0) : r->result;
  char*               method   = d_get_string(request, K_METHOD);
  str_range_t         params   = d_to_json(d_get(request, K_PARAMS));
  char*               p        = alloca(params.len + 1);
  sb_t                filename = {0};
  sb_add_chars(&filename, method);
  for_children_of(iter, d_get(request, K_PARAMS)) {
    d_bytes(iter.token);
    switch (d_type(iter.token)) {
      case T_BOOLEAN:
      case T_INTEGER:
        sb_printx(&filename, "_%i", d_int(iter.token));
        break;
      case T_STRING:
        if (d_len(iter.token) < 10)
          sb_printx(&filename, "_%s", d_string(iter.token));
        break;

      default:
        break;
    }
  }

  strncpy(p, params.data, params.len);
  p[params.len] = 0;
  TEST_ASSERT_EQUAL_MESSAGE(0, add_response_test(filename.data, p), "response not found");
  TEST_ASSERT_NOT_NULL_MESSAGE(response_buffer, "no request registered");
  TEST_ASSERT_NOT_NULL_MESSAGE(r, "payload not parseable");
  clean_json_str(p);
  _free(filename.data);

  TEST_ASSERT_EQUAL_STRING(response_buffer->request_method, method);
  TEST_ASSERT_EQUAL_STRING(response_buffer->request_params, p);
  json_free(r);

  sb_add_chars(&req->req->raw_response->data, response_buffer->response);
  req->req->raw_response->state = IN3_OK;
  clean_last_response();
  return IN3_OK;
}
