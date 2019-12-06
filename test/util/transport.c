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
#define MOCK_PATH "../test/testdata/mock/%s.json"
typedef struct response_s {
  char*              request_method;
  char*              request_params;
  char*              response;
  struct response_s* next;
} response_t;

static response_t* responses = NULL;

char* read_json_response_buffer(char* path) {
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
  response_t* r = responses;
  while (r) {
    if (r->next)
      r = r->next;
    else
      break;
  }

  response_t* n     = calloc(1, sizeof(response_t));
  n->request_method = request_method;
  n->request_params = request_params;
  n->response       = malloc(40 + strlen(result ? result : error) + (in3 ? strlen(in3) + 10 : 0));
  if (in3)
    sprintf(n->response, "[{\"id\":1,\"jsonrpc\":\"2.0\",\"%s\":%s,\"in3\":%s}]", result ? "result" : "error", result ? result : error, in3);
  else
    sprintf(n->response, "[{\"id\":1,\"jsonrpc\":\"2.0\",\"%s\":%s}]", result ? "result" : "error", result ? result : error);

  if (r)
    r->next = n;
  else
    responses = n;
}
/* add response - request mock from json*/
void add_response_test(char* test) {
  char path[70];
  sprintf(path, MOCK_PATH, test);
  char*       buffer = read_json_response_buffer(path);
  json_ctx_t* mock   = parse_json(buffer);
  str_range_t res    = d_to_json(d_get_at(d_get(mock->result, key("response")), 0));
  d_token_t*  req    = d_get_at(d_get(mock->result, key("request")), 0);
  char*       params =  d_create_json(d_get(req, key("params")));
  clean_json_str(params);
  char* method = d_get_string(req, "method");
  response_t* r = responses;
  while (r) {
    if (r->next)
      r = r->next;
    else
      break;
  }
  response_t* n     = _calloc(1, sizeof(response_t));
  n->request_method = method;
  n->request_params = params;
  n->response       = _malloc(40 + res.len);
  sprintf(n->response, "%s", res.data);

  if (r)
    r->next = n;
  else
    responses = n;
}


in3_ret_t test_transport(in3_request_t* req) {
  TEST_ASSERT_NOT_NULL_MESSAGE(responses, "no request registered");
  json_ctx_t* r = parse_json(req->payload);
  TEST_ASSERT_NOT_NULL_MESSAGE(r, "payload not parseable");
  d_token_t* request = d_type(r->result) == T_ARRAY ? r->result + 1 : r->result;
  char * method = d_get_string(request, "method");
  str_range_t params  = d_to_json(d_get(request, key("params")));
  char        p[params.len + 1];
  strncpy(p, params.data, params.len);
  p[params.len] = 0;
  clean_json_str(p);

  TEST_ASSERT_EQUAL_STRING(responses->request_method, method);
  TEST_ASSERT_EQUAL_STRING(responses->request_params, p);
  free_json(r);

  sb_add_chars(&req->results->result, responses->response);
  response_t* next = responses->next;
  _free(responses->response);
  //_free(responses);
  responses = next;
  return IN3_OK;
}

static in3_ret_t setup_transport(in3_request_t* req, char* path) {
  // now parse the json
  char*       response_buffer = read_json_response_buffer(path);
  json_ctx_t* res             = parse_json(response_buffer);
  str_range_t json            = d_to_json(d_get_at(d_get(res->result, key("response")), 0));
  sb_add_range(&req->results->result, json.data, 0, json.len);
  free_json(res);
  _free(response_buffer);
  return IN3_OK;
}
