/** @file 
 * simple commandline-util sending in3-requests.
 * */

#include <abi.h>
#include <client/cache.h>
#include <client/client.h>
#include <eth_api.h>
#include <eth_full.h>
#include <evm.h>
#include <in3_curl.h>
#include <in3_storage.h>
#include <inttypes.h>
#include <math.h>
#include <signer.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/debug.h>
#include <util/utils.h>

#include <util/data.h>

uint64_t getChainId(char* name) {
  if (strcmp(name, "mainnet") == 0) return 0x01L;
  if (strcmp(name, "kovan") == 0) return 0x2aL;
  if (strcmp(name, "tobalaba") == 0) return 0x44dL;
  if (strcmp(name, "evan") == 0) return 0x4b1L;
  if (strcmp(name, "evan") == 0) return 0x4b1L;
  if (strcmp(name, "ipfs") == 0) return 0x7d0;
  return atoi(name);
}

call_request_t* prepare_tx(char* fn_sig, char* to, char* args, char* block_number, uint64_t gas, char* value) {
  call_request_t* req = parseSignature(fn_sig);
  if (req->in_data->type == A_TUPLE) {
    json_ctx_t* in_data = parse_json(args);
    if (set_data(req, in_data->result, req->in_data) < 0) { printf("Error: could not set the data"); }
    free_json(in_data);
  }
  sb_t* params = sb_new("");
  sb_add_chars(params, "[{\"to\":\"");
  sb_add_chars(params, to);
  sb_add_chars(params, "\", \"data\":");
  sb_add_bytes(params, "", &req->call_data->b, 1, false);
  if (block_number) {
    sb_add_chars(params, "},\"");
    sb_add_chars(params, block_number);
    sb_add_chars(params, "\"]");
  } else {
    if (value) {
      sb_add_chars(params, ", \"value\":\"");
      sb_add_chars(params, value);
      sb_add_chars(params, "\"");
    }
    sb_add_chars(params, ", \"gasLimit\":");
    uint8_t gasdata[8];
    bytes_t g_bytes = bytes(gasdata, 8);
    long_to_bytes(gas ? gas : 100000, gasdata);
    b_optimize_len(&g_bytes);
    sb_add_bytes(params, "", &g_bytes, 1, false);
    sb_add_chars(params, "}]");
  }
  strcpy(args, params->data);
  sb_free(params);
  return req;
}

void print_val(d_token_t* t) {
  switch (d_type(t)) {
    case T_ARRAY:
    case T_OBJECT:
      for (d_iterator_t it = d_iter(t); it.left; d_iter_next(&it))
        print_val(it.token);
      break;
    case T_BOOLEAN:
      printf("%s\n", d_int(t) ? "true" : "false");
      break;
    case T_INTEGER:
      printf("%i\n", d_int(t));
      break;
    case T_BYTES:
      if (t->len < 9)
        printf("%" PRId64 "\n", d_long(t));
      else {
        printf("0x");
        for (int i = 0; i < t->len; i++) printf("%02x", t->data[i]);
        printf("\n");
      }
      break;
    case T_NULL:
      printf("NULL\n");
      break;
    case T_STRING:
      printf("%s\n", d_string(t));
      break;
  }
}
int main(int argc, char* argv[]) {
  //  newp();
  int i;
  if (argc < 2) {
    fprintf(stdout, "Usage: %s <options> method params ... \n  -p -proof    none|standard|full\n  -s -signs    number of signatures\n  -c -chain    mainnet|kovan|evan|tobalaba|ipfs\n", argv[0]);
    return 1;
  }

  char* method = NULL;
  char  params[5000];
  params[0] = '[';
  int p     = 1;

  in3_storage_handler_t storage_handler;
  storage_handler.get_item = storage_get_item;
  storage_handler.set_item = storage_set_item;

  in3_register_eth_full();

  in3_t* c        = in3_new();
  c->transport    = send_curl;
  c->requestCount = 1;
  c->cacheStorage = &storage_handler;
  in3_cache_init(c);
  bytes32_t       pk;
  bool            force_hex    = false;
  char*           sig          = NULL;
  char*           to           = NULL;
  char*           block_number = "latest";
  call_request_t* req          = NULL;
  bool            json         = false;
  uint64_t        gas_limit    = 100000;
  char*           value        = NULL;
  bool            wait         = false;

  if (getenv("IN3_PK")) {
    hex2byte_arr(getenv("IN3_PK"), -1, pk, 32);
    eth_set_pk_signer(c, pk);
  }

  // fill from args
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-pk") == 0) {
      hex2byte_arr(argv[++i], -1, pk, 32);
      eth_set_pk_signer(c, pk);
    } else if (strcmp(argv[i], "-chain") == 0 || strcmp(argv[i], "-c") == 0)
      c->chainId = getChainId(argv[++i]);
    else if (strcmp(argv[i], "-block") == 0 || strcmp(argv[i], "-b") == 0)
      block_number = argv[++i];
    else if (strcmp(argv[i], "-to") == 0)
      to = argv[++i];
    else if (strcmp(argv[i], "-gas") == 0)
      gas_limit = atoll(argv[++i]);
    else if (strcmp(argv[i], "-value") == 0)
      value = argv[++i];
    else if (strcmp(argv[i], "-hex") == 0)
      force_hex = true;
    else if (strcmp(argv[i], "-wait") == 0)
      wait = true;
    else if (strcmp(argv[i], "-json") == 0)
      json = true;
    else if (strcmp(argv[i], "-debug") == 0)
      c->evm_flags = EVM_PROP_DEBUG;
    else if (strcmp(argv[i], "-signs") == 0 || strcmp(argv[i], "-s") == 0)
      c->signatureCount = atoi(argv[++i]);
    else if (strcmp(argv[i], "-proof") == 0 || strcmp(argv[i], "-p") == 0) {
      if (strcmp(argv[i + 1], "none") == 0)
        c->proof = PROOF_NONE;
      else if (strcmp(argv[i + 1], "standard") == 0)
        c->proof = PROOF_STANDARD;
      else if (strcmp(argv[i + 1], "full") == 0)
        c->proof = PROOF_FULL;
      else {
        printf("Invalid Argument for proof: %s\n", argv[i + 1]);
        return 1;
      }
      i++;
    } else {
      if (method == NULL)
        method = argv[i];
      else if (to == NULL && (strcmp(method, "call") == 0 || strcmp(method, "send") == 0))
        to = argv[i];
      else if (sig == NULL && (strcmp(method, "call") == 0 || strcmp(method, "send") == 0))
        sig = argv[i];
      else {
        if (p > 1) params[p++] = ',';
        if (argv[i][0] == '{' || strcmp(argv[i], "true") == 0 || strcmp(argv[i], "false") == 0 || (*argv[i] >= '0' && *argv[i] <= '9' && *(argv[i] + 1) != 'x'))
          p += sprintf(params + p, "%s", argv[i]);
        else
          p += sprintf(params + p, "\"%s\"", argv[i]);
      }
    }
  }
  params[p++] = ']';
  params[p]   = 0;

  char* result;
  char* error;

  if (strcmp(method, "call") == 0) {
    req    = prepare_tx(sig, to, params, block_number, 0, NULL);
    method = "eth_call";
    //    printf(" new params %s\n", params);
  } else if (strcmp(method, "send") == 0) {
    prepare_tx(sig, to, params, NULL, gas_limit, value);
    method = "eth_sendTransaction";
    //    printf(" new params %s\n", params);
  }

  // send the request
  in3_client_rpc(c, method, params, &result, &error);
  // if we need to wait
  if (!error && result && wait && strcmp(method, "eth_sendTransaction") == 0) {
    bytes32_t txHash;
    hex2byte_arr(result + 3, 64, txHash, 32);
    result = eth_wait_for_receipt(c, txHash);
  }
  in3_free(c);

  if (error) {
    fprintf(stderr, "Error: %s\n", error);
    free(error);
    return 1;
  } else {

    // if the result is a string, we remove the quotes
    if (result[0] == '"' && result[strlen(result) - 1] == '"') {
      memmove(result, result + 1, strlen(result) + 1);
      result[strlen(result) - 1] = 0;
    }

    // if the request was a eth_call, we decode the result
    if (req) {
      int l = strlen(result) / 2 - 1;
      if (l) {
        uint8_t tmp[l + 1];
        l               = hex2byte_arr(result, -1, tmp, l + 1);
        json_ctx_t* res = req_parse_result(req, bytes(tmp, l));
        if (json)
          printf("%s\n", d_create_json(res->result));
        else
          print_val(res->result);
        req_free(req);
      }
      // if not we simply print the result
    } else {
      if (!force_hex && result[0] == '0' && result[1] == 'x' && strlen(result) <= 18) {
        printf("%" PRIu64 "\n", c_to_long(result, strlen(result)));
      } else
        printf("%s\n", result);
    }
    free(result);
  }
  return 0;
}