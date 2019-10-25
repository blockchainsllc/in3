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

typedef struct response_s {
  char*              request;
  char*              response;
  struct response_s* next;
} response_t;

response_t* responses = NULL;

void add_response(char* request, char* result, char* error, char* in3) {
  response_t* r = responses;
  while (r) {
    if (r->next)
      r = r->next;
    else
      break;
  }

  response_t* n = calloc(1, sizeof(response_t));
  n->request    = request;
  n->response   = malloc(40 + strlen(result ? result : error) + (in3 ? strlen(in3) + 10 : 0));
  if (in3)
    sprintf(n->response, "[{\"id\":1,\"jsonrpc\":\"2.0\",\"%s\":%s,\"in3\":%s}]", result ? "result" : "error", result ? result : error, in3);
  else
    sprintf(n->response, "[{\"id\":1,\"jsonrpc\":\"2.0\",\"%s\":%s}]", result ? "result" : "error", result ? result : error);

  if (r)
    r->next = n;
  else
    responses = n;
}

in3_ret_t test_transport(char** urls, int urls_len, char* payload, in3_response_t* result) {
  TEST_ASSERT_NOT_NULL(responses);
  TEST_ASSERT_EQUAL_STRING(responses->request, payload);
  sb_add_chars(&result->result, responses->response);
  response_t* next = responses->next;
  free(responses->response);
  free(responses);
  responses = next;
  return IN3_OK;
}
