#include "transport.h"
#include "../../src/core/client/cache.h"
#include "../../src/core/client/context.h"
#include "../../src/core/client/nodelist.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/log.h"
#include "../../src/verifier/eth1/basic/eth_basic.h"
#include "../../src/verifier/eth1/basic/signer.h"
#include "../test_utils.h"
#include <stdio.h>
#include <unistd.h> 
#define MOCK_PATH "../test/testdata/mock/%s.json"
static char *response_buffer;
typedef struct response_s {
  char*              request_method;
  char*              request_params;
  char*              response;
  struct response_s* next;
} response_t;

response_t* responses = NULL;

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

// void add_response(char* request_method, char* request) {
//   json_ctx_t* res  = parse_json(request);
//   str_range_t json = d_to_json(d_get_at(res->result, 0));
//   response_t* r = responses;
//   while (r) {
//     if (r->next)
//       r = r->next;
//     else
//       break;
//   }

//   response_t* n     = calloc(1, sizeof(response_t));
//   n->request_method = request_method;
//   n->request_params = request_params;
//   n->response       = malloc(40 + strlen(result ? result : error) + (in3 ? strlen(in3) + 10 : 0));
//   if (in3)
//     sprintf(n->response, "[{\"id\":1,\"jsonrpc\":\"2.0\",\"%s\":%s,\"in3\":%s}]", result ? "result" : "error", result ? result : error, in3);
//   else
//     sprintf(n->response, "[{\"id\":1,\"jsonrpc\":\"2.0\",\"%s\":%s}]", result ? "result" : "error", result ? result : error);

//   if (r)
//     r->next = n;
//   else
//     responses = n;
// }

#ifdef CURL_ENABLE
in3_ret_t curl_transport(in3_request_t* req) {
  return send_curl_blocking((const char**) req->urls, req->urls_len, req->payload, req->results);
}
#endif

in3_ret_t test_transport(in3_request_t* req) {
  //  char** urls, int urls_len, char* payload, in3_response_t* result
  TEST_ASSERT_NOT_NULL_MESSAGE(responses, "no request registered");
  json_ctx_t* r = parse_json(req->payload);
  TEST_ASSERT_NOT_NULL_MESSAGE(r, "payload not parseable");
  d_token_t*  request = d_type(r->result) == T_ARRAY ? r->result + 1 : r->result;
  str_range_t params  = d_to_json(d_get(request, key("params")));
  char        p[params.len + 1];
  strncpy(p, params.data, params.len);
  p[params.len] = 0;
  TEST_ASSERT_EQUAL_STRING(responses->request_method, d_get_string(request, "method"));
  TEST_ASSERT_EQUAL_STRING(responses->request_params, p);
  free_json(r);

  sb_add_chars(&req->results->result, responses->response);
  response_t* next = responses->next;
  free(responses->response);
  free(responses);
  responses = next;
  return IN3_OK;
}
static void read_json_response_buffer(char* path) {
  if(response_buffer != NULL){
      _free(response_buffer);
      response_buffer = NULL;
  }
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
  } else {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      printf("Current working dir: %s\n", cwd);
    } else {
      perror("getcwd() error");
    }
    printf("Error coudl not find the testdata %s\n", path);
  }
}

static in3_ret_t setup_transport(in3_request_t* req, char * path) {
  // now parse the json
  read_json_response_buffer(path);
  json_ctx_t* res  = parse_json(response_buffer);
  str_range_t json = d_to_json(d_get_at(d_get(res->result,key("response")),0));
  sb_add_range(&req->results->result, json.data, 0, json.len);
  free_json(res);
  return IN3_OK;
}
in3_ret_t mock_transport(in3_request_t* req) {
  char path[70];
  sprintf(path, MOCK_PATH, "in3_nodeList");
  in3_log_debug("Req : %s \n", path);
  for (int i = 0; i < req->urls_len; i++) {
    if (strstr(req->payload, "nodeList") != NULL) {
      in3_log_debug("Returning Node List ...\n");
      sprintf(path, MOCK_PATH, "in3_nodeList");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_call") != NULL) {
      in3_log_debug("Returning Call Response ...\n");
      sprintf(path, MOCK_PATH, "eth_call");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_getCode") != NULL) {
      in3_log_debug("Returning getCode Response ...\n");
      sprintf(path, MOCK_PATH, "eth_getCode");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_getBlockByHash") != NULL) {
      in3_log_debug("Returning block by hash  Response ...\n");
      sprintf(path, MOCK_PATH, "eth_getBlockByHash");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_getBlockByNumber") != NULL) {
      in3_log_debug("Returning block by number Response ...\n");
      sprintf(path, MOCK_PATH, "eth_getBlockByNumber");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_blockNumber") != NULL) {
      in3_log_debug("Returning block by number Response ...\n");
      sprintf(path, MOCK_PATH, "eth_blockNumber");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_getBalance") != NULL) {
      in3_log_debug("Returning eth_getBalance Response ...\n");
      sprintf(path, MOCK_PATH, "eth_getBalance");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_getTransactionByHash") != NULL) {
      in3_log_debug("Returning eth_getTransactionByHash Response ...\n");
      sprintf(path, MOCK_PATH, "eth_getTransactionByHash");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_getBlockTransactionCountByHash") != NULL) {
      in3_log_debug("Returning eth_getBlockTransactionCountByHash Response ...\n");
      sprintf(path, MOCK_PATH, "eth_getBlockTransactionCountByHash");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_getBlockTransactionCountByNumber") != NULL) {
      in3_log_debug("Returning eth_getBlockTransactionCountByNumber Response ...\n");
      sprintf(path, MOCK_PATH, "eth_getBlockTransactionCountByNumber");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_getLogs") != NULL) {
      in3_log_debug("Returning eth_getLogs Response ...\n");
      sprintf(path, MOCK_PATH, "eth_getLogs");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_chainId") != NULL) {
      in3_log_debug("Returning eth_chainId Response ...\n");
      sprintf(path, MOCK_PATH, "eth_chainId");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_getTransactionReceipt") != NULL) {
      in3_log_debug("Returning eth_getTransactionReceipt Response ...\n");
      sprintf(path, MOCK_PATH, "eth_getTransactionReceipt");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_getTransactionCount") != NULL) {
      in3_log_debug("Returning eth_getTransactionCount Response ...\n");
      sprintf(path, MOCK_PATH, "eth_getTransactionCount");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_sendRawTransaction") != NULL) {
      in3_log_debug("Returning eth_sendRawTransaction Response ...\n");
      sprintf(path, MOCK_PATH, "eth_sendRawTransaction");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_getStorageAt") != NULL) {
      in3_log_debug("Returning eth_getStorageAt Response ...\n");
      sprintf(path, MOCK_PATH, "eth_getStorageAt");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_gasPrice") != NULL) {
      in3_log_debug("Returning eth_gasPrice Response ...\n");
      sprintf(path, MOCK_PATH, "eth_gasPrice");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_getTransactionByBlockHashAndIndex") != NULL) {
      in3_log_debug("Returning eth_getTransactionByBlockHashAndIndex Response ...\n");
      sprintf(path, MOCK_PATH, "eth_getTransactionByBlockHashAndIndex");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_getTransactionByBlockNumberAndIndex") != NULL) {
      in3_log_debug("Returning eth_getTransactionByBlockNumberAndIndex Response ...\n");
      sprintf(path, MOCK_PATH, "eth_getTransactionByBlockNumberAndIndex");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_getUncleByBlockNumberAndIndex") != NULL) {
      in3_log_debug("Returning eth_getUncleByBlockNumberAndIndex Response ...\n");
      sprintf(path, MOCK_PATH, "eth_getUncleByBlockNumberAndIndex");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_getUncleCountByBlockNumber") != NULL) {
      in3_log_debug("Returning eth_getUncleCountByBlockNumber Response ...\n");
      sprintf(path, MOCK_PATH, "eth_getUncleCountByBlockNumber");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_getUncleCountByBlockHash") != NULL) {
      in3_log_debug("Returning eth_getUncleCountByBlockHash Response ...\n");
      sprintf(path, MOCK_PATH, "eth_getUncleCountByBlockHash");
      return setup_transport(req, path);
    } else if (strstr(req->payload, "eth_estimateGas") != NULL) {
      in3_log_debug("Returning eth_estimateGas Response ...\n");
      sprintf(path, MOCK_PATH, "eth_estimateGas");
      return setup_transport(req, path);
    } 

  }
  return 0;
}