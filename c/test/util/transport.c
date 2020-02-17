#include "transport.h"
#include "../../src/core/client/cache.h"
#include "../../src/core/client/context.h"
#include "../../src/core/client/nodelist.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/log.h"
#include "../../src/core/util/utils.h"
#include "../../src/verifier/eth1/basic/eth_basic.h"
#include "../../src/verifier/eth1/basic/signer.h"
#include "../test_utils.h"
#include <stdio.h>
#include <unistd.h>
#define MOCK_PATH "../c/test/testdata/mock/%s.json"

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
  char* response_buffer;
  long  length;
  FILE* f = fopen(path, "r");
  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    response_buffer = _malloc(length + 1);
    if (response_buffer) {
      fread(response_buffer, 1, length, f);
      response_buffer[length] = 0;
    }
    fclose(f);
    return response_buffer;
  } else {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      printf("Current working dir: %s\n", cwd);
    } else {
      perror("getcwd() error");
    }
    printf("Error coudl not find the testdata %s\n", path);
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

/* add response - request mock from json*/
void add_response_test(char* test) {
  if (response_buffer) {
    _free(response_buffer);
    response_buffer = NULL;
  }
  char path[70];
  sprintf(path, MOCK_PATH, test);
  char*       buffer = read_json_response_buffer(path);
  json_ctx_t* mock   = parse_json(buffer);
  str_range_t res    = d_to_json(d_get_at(d_get(mock->result, key("response")), 0));
  d_token_t*  req    = d_get_at(d_get(mock->result, key("request")), 0);
  char*       params = d_create_json(d_get(req, key("params")));
  clean_json_str(params);
  char* method                    = d_get_string(req, "method");
  response_buffer                 = _calloc(1, sizeof(response_t));
  response_buffer->request_method = method;
  response_buffer->request_params = params;
  response_buffer->response       = _malloc(40 + res.len);
  sprintf(response_buffer->response, "%s", res.data);
}

in3_ret_t test_transport(in3_request_t* req) {
  TEST_ASSERT_NOT_NULL_MESSAGE(responses, "no request registered");
  json_ctx_t* r = parse_json(req->payload);
  TEST_ASSERT_NOT_NULL_MESSAGE(r, "payload not parseable");
  d_token_t*  request = d_type(r->result) == T_ARRAY ? r->result + 1 : r->result;
  char*       method  = d_get_string(request, "method");
  str_range_t params  = d_to_json(d_get(request, key("params")));
  char*       p       = alloca(params.len + 1);
  strncpy(p, params.data, params.len);
  p[params.len] = 0;
  clean_json_str(p);

  TEST_ASSERT_EQUAL_STRING(responses->request_method, method);
  TEST_ASSERT_EQUAL_STRING(responses->request_params, p);
  json_free(r);

  sb_add_chars(&req->results->result, responses->response);
  response_t* next = responses->next;
  _free(responses->response);
  _free(responses);
  responses = next;
  return IN3_OK;
}

in3_ret_t mock_transport(in3_request_t* req) {
  json_ctx_t* r       = parse_json(req->payload);
  d_token_t*  request = d_type(r->result) == T_ARRAY ? r->result + 1 : r->result;
  char*       method  = d_get_string(request, "method");
  add_response_test(method);
  TEST_ASSERT_NOT_NULL_MESSAGE(response_buffer, "no request registered");
  TEST_ASSERT_NOT_NULL_MESSAGE(r, "payload not parseable");
  str_range_t params = d_to_json(d_get(request, key("params")));
  char*       p      = alloca(params.len + 1);
  strncpy(p, params.data, params.len);
  p[params.len] = 0;
  clean_json_str(p);

  TEST_ASSERT_EQUAL_STRING(response_buffer->request_method, method);
  TEST_ASSERT_EQUAL_STRING(response_buffer->request_params, p);
  json_free(r);

  sb_add_chars(&req->results->result, response_buffer->response);
  return IN3_OK;
}
