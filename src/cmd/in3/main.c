/** @file 
 * simple commandline-util sending in3-requests.
 * */

#include "../../api/eth1/abi.h"
#include "../../api/eth1/eth_api.h"
#include "../../core/client/cache.h"
#include "../../core/client/client.h"
#include "../../core/util/data.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../core/util/utils.h"
#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/secp256k1.h"
#include "../../transport/curl/in3_curl.h"
#include "../../verifier/eth1/basic/signer.h"
#include "../../verifier/eth1/full/eth_full.h"
#include "../../verifier/eth1/full/evm.h"
#include "in3_storage.h"
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* get_wei(char* val) {
  if (*val == '0' && val[1] == 'x') return val;
  int    l     = strlen(val);
  double value = 0;
  if (l > 3 && val[l - 1] > '9') {
    char unit[4];
    strcpy(unit, val + l - 3);
    double f = 1;
    if (val[l - 4] == 'k')
      f = 1000;
    else if (val[l - 4] == 'M')
      f = 1000000;
    else if (val[l - 4] == 'm')
      f = 0.001;
    val[l - ((f == 1) ? 3 : 4)] = 0;
    value                       = atof(val);
    if (strcmp(unit, "eth") == 0)
      value *= 1000000000000000000l;
    else if (strcmp(unit, "wei") == 0)
      value *= 1000000000000000000l;
    value *= f;
  } else
    value = atof(val);

  value            = floor(value);
  char* res        = _malloc(200);
  res[199]         = 0;
  char*      p     = res + 199;
  const char hex[] = "0123456789abcdef";
  while (value > 0.999999) {
    p--;
    *p    = hex[(int) (fmod(value, 16))];
    value = floor(value / 16);
  }
  res[0] = '0';
  res[1] = 'x';
  if (p == res + 199) {
    res[2] = '0';
    res[3] = 0;
  } else {
    memmove(res + 2, p, res + 199 - p);
    res[res + 201 - p] = 0;
  }
  return res;
}

bytes_t readFile(FILE* f) {
  size_t   allocated = 1024, len = 0, r;
  uint8_t* buffer = _malloc(1025);
  while (1) {
    r = fread(buffer + len, 1, allocated - len, f);
    len += r;
    if (feof(f)) break;
    buffer = _realloc(buffer, allocated * 2 + 1, allocated + 1);
    allocated *= 2;
  }
  buffer[len] = 0;
  return bytes(buffer, len);
}

bytes_t* get_std_in() {
  if (feof(stdin)) return NULL;
  bytes_t content = readFile(stdin);

  char* bin = strstr((char*) content.data, "\nBinary: \n");
  if (bin) {
    bin += 10;
    char* end = strstr(bin, "\n");
    if (end)
      return hex2byte_new_bytes(bin, end - bin);
  }

  bytes_t* res = (content.len > 1 && *content.data == '0' && content.data[1] == 'x') ? hex2byte_new_bytes((char*) content.data + 2, content.len - 2) : hex2byte_new_bytes((char*) content.data, content.len);
  _free(content.data);
  return res;
}

uint64_t getChainId(char* name) {
  if (strcmp(name, "mainnet") == 0) return 0x01L;
  if (strcmp(name, "kovan") == 0) return 0x2aL;
  if (strcmp(name, "tobalaba") == 0) return 0x44dL;
  if (strcmp(name, "evan") == 0) return 0x4b1L;
  if (strcmp(name, "ipfs") == 0) return 0x7d0;
  if (strcmp(name, "local") == 0) return 0xFFFFL;
  if (strcmp(name, "volta") == 0) return 0x12046;
  return atoi(name);
}

void set_chain_id(in3_t* c, char* id) {
  if (strstr(id, "://")) {
    c->chainId                   = 0xFFFFL;
    c->chains[6].nodeList[0].url = id;
  } else
    c->chainId = getChainId(id);
}

call_request_t* prepare_tx(char* fn_sig, char* to, char* args, char* block_number, uint64_t gas, char* value, bytes_t* data) {
  call_request_t* req = fn_sig ? parseSignature(fn_sig) : NULL;
  if (req && req->in_data->type == A_TUPLE) {
    json_ctx_t* in_data = parse_json(args);
    if (set_data(req, in_data->result, req->in_data) < 0) { printf("Error: could not set the data"); }
    free_json(in_data);
  }
  sb_t* params = sb_new("[{");
  if (to) {
    sb_add_chars(params, "\"to\":\"");
    sb_add_chars(params, to);
    sb_add_chars(params, "\" ");
  }
  if (req || data) {
    if (to)
      sb_add_chars(params, ",\"data\":");
    else
      sb_add_chars(params, "\"data\":");
  }
  if (req) {
    if (data) {
      uint8_t* full = _malloc(req->call_data->b.len - 4 + data->len);
      memcpy(full, data->data, data->len);
      memcpy(full + data->len, req->call_data->b.data + 4, req->call_data->b.len - 4);
      bytes_t bb = bytes(full, req->call_data->b.len - 4 + data->len);
      sb_add_bytes(params, "", &bb, 1, false);
      _free(full);
    } else
      sb_add_bytes(params, "", &req->call_data->b, 1, false);
  } else if (data)
    sb_add_bytes(params, "", data, 1, false);
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
        for (int i = 0; i < (int) t->len; i++) printf("%02x", t->data[i]);
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
  char  params[50000];
  params[0] = '[';
  int p     = 1;

  in3_storage_handler_t storage_handler;
  storage_handler.get_item = storage_get_item;
  storage_handler.set_item = storage_set_item;

  in3_register_eth_full();

  in3_t* c        = in3_new();
  c->transport    = send_curl;
  c->requestCount = 1;
  c->use_http     = true;
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
  char*           pwd          = NULL;
  char*           pk_file      = NULL;
  bytes_t*        data         = NULL;

  if (getenv("IN3_PK")) {
    hex2byte_arr(getenv("IN3_PK"), -1, pk, 32);
    eth_set_pk_signer(c, pk);
  }

  if (getenv("IN3_CHAIN"))
    set_chain_id(c, getenv("IN3_CHAIN"));

  // fill from args
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-pk") == 0) {
      if (argv[i + 1][0] == '0' && argv[i + 1][1] == 'x') {
        hex2byte_arr(argv[++i], -1, pk, 32);
        eth_set_pk_signer(c, pk);
      } else
        pk_file = argv[++i];

    } else if (strcmp(argv[i], "-chain") == 0 || strcmp(argv[i], "-c") == 0)
      set_chain_id(c, argv[++i]);
    else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "-data") == 0) {
      char* d = argv[++i];
      if (strcmp(d, "-") == 0)
        data = get_std_in();
      else if (*d == '0' && d[1] == 'x')
        data = hex2byte_new_bytes(d + 2, strlen(d) - 2);
      else {
        FILE* f = fopen(d, "r");
        if (!f) {
          printf("data could not be read from file %s", d);
          return 1;
        }
        bytes_t content = readFile(f);
        data            = hex2byte_new_bytes((char*) content.data + 2, content.len - 2);
        fclose(f);
      }
    } else if (strcmp(argv[i], "-block") == 0 || strcmp(argv[i], "-b") == 0)
      block_number = argv[++i];
    else if (strcmp(argv[i], "-to") == 0)
      to = argv[++i];
    else if (strcmp(argv[i], "-gas") == 0)
      gas_limit = atoll(argv[++i]);
    else if (strcmp(argv[i], "-pwd") == 0)
      pwd = argv[++i];
    else if (strcmp(argv[i], "-value") == 0)
      value = get_wei(argv[++i]);
    else if (strcmp(argv[i], "-hex") == 0)
      force_hex = true;
    else if (strcmp(argv[i], "-wait") == 0 || strcmp(argv[i], "-w") == 0)
      wait = true;
    else if (strcmp(argv[i], "-json") == 0)
      json = true;
    else if (strcmp(argv[i], "-np") == 0)
      c->proof = PROOF_NONE;
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

  char* result = NULL;
  char* error  = NULL;

  if (pk_file) {
    if (!pwd) {
      //TODO ask for password
    }
    FILE* pkf = fopen(pk_file, "r");
    if (!pkf) {
      printf("pk file not found!\n");
      return -1;
    }

    char*  buffer    = _malloc(1024);
    size_t allocated = 1024;
    size_t len       = 0;
    size_t r;

    while (1) {
      r = fread(buffer + len, 1, allocated - len, pkf);
      len += r;
      if (feof(pkf)) break;
      buffer = _realloc(buffer, allocated * 2, allocated);
      allocated *= 2;
    }
    fclose(pkf);
    buffer[len]          = 0;
    json_ctx_t* key_json = parse_json(buffer);
    if (!key_json) {
      printf("invalid json in pk file!\n");
      return -1;
    }

    bytes32_t pk_seed;

    switch (decrypt_key(key_json->result, pwd, pk_seed)) {
      case 0:
        break;
      default:
        printf("Invalid key\n");
        return -1;
    }

    if (!method) {
      char tmp[64];
      bytes_to_hex(pk_seed, 32, tmp);
      printf("%s", tmp);
      return 0;
    }
  }

  if (c->chainId == 0xFFFF) c->proof = PROOF_NONE;

  if (strcmp(method, "call") == 0) {
    req    = prepare_tx(sig, to, params, block_number, 0, NULL, data);
    method = "eth_call";
    //    printf(" new params %s\n", params);
  } else if (strcmp(method, "send") == 0) {
    prepare_tx(sig, to, params, NULL, gas_limit, value, data);
    method = "eth_sendTransaction";
    //    printf(" new params %s\n", params);
  } else if (strcmp(method, "autocompletelist") == 0) {
    printf("send call pk2address mainnet tobalaba kovan goerli local volta true false latest -np -debug -c -chain -p -proof -s -signs -b -block -to -d -data -gas_limit -value -w -wait -hex -json in3_nodeList in3_stats in3_sign web3_clientVersion web3_sha3 net_version net_peerCount net_listening eth_protocolVersion eth_syncing eth_coinbase eth_mining eth_hashrate eth_gasPrice eth_accounts eth_blockNumber eth_getBalance eth_getStorageAt eth_getTransactionCount eth_getBlockTransactionCountByHash eth_getBlockTransactionCountByNumber eth_getUncleCountByBlockHash eth_getUncleCountByBlockNumber eth_getCode eth_sign eth_sendTransaction eth_sendRawTransaction eth_call eth_estimateGas eth_getBlockByHash eth_getBlockByNumber eth_getTransactionByHash eth_getTransactionByBlockHashAndIndex eth_getTransactionByBlockNumberAndIndex eth_getTransactionReceipt eth_pendingTransactions eth_getUncleByBlockHashAndIndex eth_getUncleByBlockNumberAndIndex eth_getCompilers eth_compileLLL eth_compileSolidity eth_compileSerpent eth_newFilter eth_newBlockFilter eth_newPendingTransactionFilter eth_uninstallFilter eth_getFilterChanges eth_getFilterLogs eth_getLogs eth_getWork eth_submitWork eth_submitHashrate\n");
    return 0;
  } else if (strcmp(method, "pk2address") == 0) {
    bytes32_t prv_key;
    uint8_t   public_key[65], sdata[32];
    hex2byte_arr(argv[argc - 1], -1, prv_key, 32);
    bytes_t pubkey_bytes = {.data = public_key + 1, .len = 64};
    ecdsa_get_public_key65(&secp256k1, prv_key, public_key);
    sha3_to(&pubkey_bytes, sdata);
    printf("0x");
    for (i = 0; i < 20; i++) printf("%02x", sdata[i + 12]);
    printf("\n");
    return 0;
  }

  if (c->evm_flags == EVM_PROP_DEBUG)
    in3_log_debug("..sending request %s %s", method, params);

  // send the request
  in3_client_rpc(c, method, params, &result, &error);
  // if we need to wait
  if (!error && result && wait && strcmp(method, "eth_sendTransaction") == 0) {
    bytes32_t txHash;
    hex2byte_arr(result + 3, 64, txHash, 32);
    result = eth_wait_for_receipt(c, txHash);
    if (!result) {
      fprintf(stderr, "Error waiting for the confirmation of the transaction\n");
      return 1;
    }
  }

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
  }
  return 0;
}
