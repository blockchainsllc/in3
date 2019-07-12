#include "../../core/client/client.h"
#include "../../verifier/eth1/full/eth_full.h"
#include <emscripten.h>
#include <emscripten/fetch.h>
#include <stdio.h>
#include <string.h>

int fetch(char** urls, int urls_len, char* payload, in3_response_t* result) {

  const char* headers[3];
  headers[0] = "Content-Type";
  headers[1] = "application/json";
  headers[2] = NULL;
  int res    = 0;

  for (int i = 0; i < urls_len; i++) {
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "POST");
    attr.requestData          = payload;
    attr.attributes           = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;
    attr.requestHeaders       = headers;
    emscripten_fetch_t* fetch = emscripten_fetch(&attr, urls[i]); // Blocks here until the operation is complete.
    if (fetch->status == 200)
      sb_add_range(&result[i].result, fetch->data, 0, fetch->numBytes);
    else {
      sb_add_chars(&result[i].error, "Error fetching the RPC-call");
      res = -1;
    }
    emscripten_fetch_close(fetch);
  }
  return res;
}

in3_t* EMSCRIPTEN_KEEPALIVE in3_create() {
  // register a chain-verifier for full Ethereum-Support
  in3_register_eth_full();

  in3_t* c     = in3_new();
  c->transport = fetch;
  printf("init in3\n");
  return c;
}
static char* last_error;

char* EMSCRIPTEN_KEEPALIVE in3_last_error() {
  return last_error;
}

/* sends a request and stores the result in the provided buffer */
char* EMSCRIPTEN_KEEPALIVE in3_send(in3_t* c, char* method, char* params) {
  if (last_error) free(last_error);
  last_error   = NULL;
  char *result = NULL, *error = NULL;
  int   res = in3_client_rpc(c, method, params, &result, &error);
  if (error) {
    last_error = error;
    return NULL;
  } else if (res < 0 || !result) {
    char* msg  = "unknown error while execxuting in3-request";
    last_error = malloc(strlen(msg) + 1);
    strcpy(last_error, msg);
    return NULL;
  }

  return result;
}

/* frees the references of the client */
void EMSCRIPTEN_KEEPALIVE in3_dispose(in3_t* a) {
  in3_free(a);
}

int main(int argc, char* argv[]) {
  in3_t* c   = in3_create();
  char*  res = in3_send(c, "eth_blockNumber", "[]");
  if (!res) res = in3_last_error();
  printf("lastBlock : %s\n", res);
  in3_dispose(c);
}