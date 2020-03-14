/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 * 
 * 
 * COMMERCIAL LICENSE USAGE
 * 
 * Licensees holding a valid commercial license may use this file in accordance 
 * with the commercial license agreement provided with the Software or, alternatively, 
 * in accordance with the terms contained in a written agreement between you and 
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
 * information please contact slock.it at in3@slock.it.
 * 	
 * Alternatively, this file may be used under the AGPL license as follows:
 *    
 * AGPL LICENSE USAGE
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

/** @file 
 * simple commandline-util sending in3-requests.
 * */

#include "../../api/eth1/abi.h"
#include "../../api/eth1/eth_api.h"
#include "../../core/util/bitset.h"
#include "../../core/util/data.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/rand.h"
#include "../../third-party/crypto/secp256k1.h"
#ifdef USE_CURL
#include "../../transport/curl/in3_curl.h"
#else
#include "../../transport/http/in3_http.h"
#endif
#ifdef IN3_SERVER
#include "../http-server/http_server.h"
#endif
#include "../../core/client/cache.h"
#include "../../core/client/context.h"
#include "../../core/client/keys.h"
#include "../../core/client/nodelist.h"
#include "../../core/client/version.h"
#include "../../core/util/colors.h"
#include "../../verifier/eth1/basic/signer.h"
#include "../../verifier/eth1/evm/evm.h"
#include "../../verifier/eth1/full/eth_full.h"
#include "../../verifier/eth1/nano/chainspec.h"
#include "in3_storage.h"
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef BTC
#include "../../verifier/btc/btc.h"
#endif
#ifdef IPFS
#include "../../api/ipfs/ipfs_api.h"
#include "../../verifier/ipfs/ipfs.h"
#endif

#ifndef IN3_VERSION
#define IN3_VERSION "local"
#endif

// helpstring
void show_help(char* name) {
  printf("Usage: %s <options> method <params> ... \n\
\n\
-c, -chain     the chain to use. (mainnet,kovan,tobalaba,goerli,local or any RPCURL)\n\
-a             max number of attempts before giving up (default 5)\n\
-rc            number of request per try (default 1)\n\
-ns            no stats if set requests will not be part of the official metrics and considered a service request\n\
-p, -proof     specifies the Verification level: (none, standard(default), full)\n\
-md            specifies the minimum Deposit of a node in order to be selected as a signer\n\
-np            short for -p none\n\
-eth           converts the result (as wei) to ether.\n\
-l, -latest    replaces \"latest\" with latest BlockNumber - the number of blocks given.\n\
-s, -signs     number of signatures to use when verifying.\n\
-f             finality : number of blocks on top of the current one.\n\
-port          if specified it will run as http-server listening to the given port.\n\
-b, -block     the blocknumber to use when making calls. could be either latest (default),earliest or a hexnumbner\n\
-to            the target address of the call\n\
-d, -data      the data for a transaction. This can be a filepath, a 0x-hexvalue or - for stdin.\n\
-gas           the gas limit to use when sending transactions. (default: 100000) \n\
-pk            the private key as raw as keystorefile \n\
-st, -sigtype  the type of the signature data : eth_sign (use the prefix and hash it), raw (hash the raw data), hash (use the already hashed data). Default: raw \n\
-pwd           password to unlock the key \n\
-value         the value to send when sending a transaction. can be hexvalue or a float/integer with the suffix eth or wei like 1.8eth (default: 0)\n\
-w, -wait      if given, instead returning the transaction, it will wait until the transaction is mined and return the transactionreceipt.\n\
-json          if given the result will be returned as json, which is especially important for eth_call results with complex structres.\n\
-hex           if given the result will be returned as hex.\n\
-kin3          if kin3 is specified, the response including in3-section is returned\n\
-debug         if given incubed will output debug information when executing. \n\
-k             32bytes raw private key to sign requests.\n\
-q             quit. no additional output. \n\
-ri            read response from stdin \n\
-ro            write raw response to stdout \n\
-version       displays the version \n\
-help          displays this help message \n\
\n\
As method, the following can be used:\n\
\n\
<JSON-RPC>-method\n\
  all official supported JSON-RPC-Method may be used.\n\
\n\
send <signature> ...args\n\
  based on the -to, -value and -pk a transaction is build, signed and send. \n\
  if there is another argument after send, this would be taken as a function-signature of the smart contract followed by optional argument of the function.\n\
\n\
call <signature> ...args\n\
  uses eth_call to call a function. Following the call argument the function-signature and its arguments must follow. \n\
\n\
in3_nodeList\n\
  returns the nodeList of the Incubed NodeRegistry as json.\n\
\n\
in3_sign <blocknumber>\n\
  requests a node to sign. in order to specify the signer, you need to pass the url with -c\n\
\n\
ipfs_get <ipfs_hash>\n\
  requests and verifies the content for a given ipfs-hash and write the content to stdout\n\
\n\
ipfs_put\n\
  reads a content from stdin and pushes to the ipfs-network. it write the ipfs-hash to stdout.\n\
\n\
in3_stats\n\
  returns the stats of a node. unless you specify the node with -c <rpcurl> it will pick a random node.\n\
\n\
abi_encode <signature> ...args\n\
  encodes the arguments as described in the method signature using ABI-Encoding\n\
\n\
abi_decode <signature> data\n\
  decodes the data based on the signature.\n\
\n\
pk2address <privatekey>\n\
  extracts the public address from a private key\n\
\n\
pk2public <privatekey>\n\
  extracts the public key from a private key\n\
\n\
ecrecover <msg> <signature>\n\
  extracts the address and public key from a signature\n\
\n\
key <keyfile>\n\
  reads the private key from JSON-Keystore file and returns the private key.\n\
\n\
in3_weights\n\
  list all current weights and stats\n\
\n\
in3_ens <domain> <field>\n\
  resolves a ens-domain. field can be addr(deault), owner, resolver or hash\n\
\n",
         name);
}

static void die(char* msg) {
  fprintf(stderr, COLORT_RED "Error: %s" COLORT_RESET "\n", msg);
  exit(EXIT_FAILURE);
}

static bool debug_mode = false;
static void print_hex(uint8_t* data, int len) {
  printf("0x");
  for (int i = 0; i < len; i++) printf("%02x", data[i]);
  printf("\n");
}
// helper to read the password from tty
void read_pass(char* pw, int pwsize) {
  int i = 0, ch = 0;
  fprintf(stderr, COLORT_HIDDEN); //conceal typing and save position
  while (true) {
    ch = getchar();
    if (ch == '\r' || ch == '\n' || ch == EOF) break; //get characters until CR or NL
    if (i < pwsize - 1) {                             //do not save pw longer than space in pw
      pw[i]     = ch;                                 //longer pw can be entered but excess is ignored
      pw[i + 1] = 0;
    }
    i++;
  }
  fprintf(stderr, COLORT_RESETHIDDEN); //reveal typing
}

// accepts a value as
// 0.1eth
// 2keth
// 2.3meth
char* get_wei(char* val) {
  if (*val == '0' && val[1] == 'x') return val;
  int    l     = strlen(val);
  double value = 0;
  if (l > 3 && val[l - 1] > '9') {
    char unit[4];
    strcpy(unit, val + l - 3); // we copy the last 3 characters as unit
    double f = 1;              // and define a modifying factor for the prefix of the unit
    if (val[l - 4] == 'k')
      f = 1000;
    else if (val[l - 4] == 'M')
      f = 1000000;
    else if (val[l - 4] == 'm')
      f = 0.001;
    val[l - ((f == 1) ? 3 : 4)] = 0;         // so we let the value string end where the unit starts
    value                       = atof(val); // so we can easily parse the value

    if (strcmp(unit, "eth") == 0)
      value *= 1000000000000000000l;
    else if (strcmp(unit, "fin") == 0)
      value *= 1000000000000000l;
    else if (strcmp(unit, "wei"))
      die("unsupported unit in value!");
    value *= f;
  } else
    value = atof(val);

  value = floor(value); // make sure it is a integer value
  if (value < 0) die("negative values are not allowed");

  // now convert the double into a hexstring
  // this is no cleaning up the mempry correctly, but since it is a comandline tool
  // we don't need to clean up
  char *res = _malloc(200), *p = res + 199;
  res[199]         = 0;
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
        exit(EXIT_SUCCESS);
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
      in3_ctx_t* ctx = ctx_new(c, sb->data);
      if (ctx->error)
        printf("{\"jsonrpc\":\"2.0\",\"id\":%i,\"error\":{\"code\":%i,\"message\":\"%s\"}\n", 1, ctx->verification_state, ctx->error);
      else {
        in3_ret_t ret = in3_send_ctx(ctx);
        uint32_t  id  = d_get_intk(ctx->requests[0], K_ID);
        if (ctx->error) {
          for (char* x = ctx->error; *x; x++) {
            if (*x == '\n') *x = ' ';
          }
        }

        if (ret == IN3_OK) {
          if (c->flags & FLAGS_KEEP_IN3) {
            str_range_t rr  = d_to_json(ctx->responses[0]);
            rr.data[rr.len] = 0;
            printf("%s\n", rr.data);
          } else {
            d_token_t* result = d_get(ctx->responses[0], K_RESULT);
            d_token_t* error  = d_get(ctx->responses[0], K_ERROR);
            char*      r      = d_create_json(result ? result : error);
            if (result)
              printf("{\"jsonrpc\":\"2.0\",\"id\":%i,\"result\":%s}\n", id, r);
            else
              printf("{\"jsonrpc\":\"2.0\",\"id\":%i,\"error\":%s}\n", id, r);
            _free(r);
          }
        } else
          printf("{\"jsonrpc\":\"2.0\",\"id\":%i,\"error\":{\"code\":%i,\"message\":\"%s\"}}\n", id, ctx->verification_state, ctx->error == NULL ? "Unknown error" : ctx->error);
      }
      ctx_free(ctx);
      first   = 0;
      sb->len = 0;
    }
  }
}

char* resolve(in3_t* c, char* name) {
  if (!name) return NULL;
  if (name[0] == '0' && name[1] == 'x') return name;
  if (strstr(name, ".eth")) {
    char* params = alloca(strlen(name) + 10);
    sprintf(params, "[\"%s\"]", name);
    char *res = NULL, *err = NULL;
    in3_client_rpc(c, "in3_ens", params, &res, &err);
    if (err) {
      res = alloca(strlen(err) + 100);
      sprintf(res, "Could not resolve %s : %s", name, err);
      die(res);
    }
    if (res[0] == '"') {
      res[strlen(res) - 1] = 0;
      res++;
    }
    return res;
  }
  return name;
}

// read data from a file and return the bytes
bytes_t readFile(FILE* f) {
  if (!f) die("Could not read the input file");
  size_t   allocated = 1024, len = 0, r;
  uint8_t* buffer = _malloc(1025);
  while (1) {
    r = fread(buffer + len, 1, allocated - len, f);
    len += r;
    if (feof(f)) break;
    size_t new_alloc = allocated * 2 + 1;
    buffer           = _realloc(buffer, new_alloc, allocated);
    allocated        = new_alloc;
  }
  buffer[len] = 0;
  return bytes(buffer, len);
}

// read from stdin and try to optimize hexdata.
bytes_t* get_std_in() {
  if (feof(stdin)) return NULL;
  bytes_t content = readFile(stdin);

  // this check tries to discover a solidity compilation poutput
  char* bin = strstr((char*) content.data, "\nBinary: \n");
  if (bin) {
    bin += 10;
    char* end = strstr(bin, "\n");
    if (end)
      return hex_to_new_bytes(bin, end - bin);
  }

  // is it content starting with 0x, we treat it as hex otherwisae as rwa string
  bytes_t* res = (content.len > 1 && *content.data == '0' && content.data[1] == 'x')
                     ? hex_to_new_bytes((char*) content.data + 2, content.len - 2)
                     : hex_to_new_bytes((char*) content.data, content.len);
  _free(content.data);
  return res;
}

// convert the name to a chain_id
uint64_t getchain_id(char* name) {
  if (strcmp(name, "mainnet") == 0) return ETH_CHAIN_ID_MAINNET;
  if (strcmp(name, "kovan") == 0) return ETH_CHAIN_ID_KOVAN;
  if (strcmp(name, "goerli") == 0) return ETH_CHAIN_ID_GOERLI;
  if (strcmp(name, "ipfs") == 0) return ETH_CHAIN_ID_IPFS;
  if (strcmp(name, "btc") == 0) return ETH_CHAIN_ID_BTC;
  if (strcmp(name, "local") == 0) return ETH_CHAIN_ID_LOCAL;
  if (name[0] == '0' && name[1] == 'x') {
    bytes32_t d;
    return bytes_to_long(d, hex_to_bytes(name + 2, -1, d, 32));
  }
  die("Unknown or unsupported chain");
  return 0;
}

// set the chain_id in the client
void set_chain_id(in3_t* c, char* id) {
  c->chain_id = strstr(id, "://") ? 0xFFFFL : getchain_id(id);
  if (c->chain_id == 0xFFFFL) {
    in3_chain_t* chain = in3_find_chain(c, c->chain_id);
    if (strstr(id, "://")) // its a url
      chain->nodelist[0].url = id;
    if (chain->nodelist_upd8_params) {
      _free(chain->nodelist_upd8_params);
      chain->nodelist_upd8_params = NULL;
    }
  }
}

// prepare a eth_call or eth_sendTransaction
call_request_t* prepare_tx(char* fn_sig, char* to, char* args, char* block_number, uint64_t gas, char* value, bytes_t* data) {
  call_request_t* req = fn_sig ? parseSignature(fn_sig) : NULL;                          // only if we have a function signature, we will parse it and create a call_request.
  if (req && req->error) die(req->error);                                                // parse-error we stop here.
  if (req && req->in_data->type == A_TUPLE) {                                            // if type is a tuple, it means we have areuments we need to parse.
    json_ctx_t* in_data = parse_json(args);                                              // the args are passed as a "[]"- json-array string.
    if (set_data(req, in_data->result, req->in_data) < 0) die("Could not set the data"); // we then set the data, which appends the arguments to the functionhash.
    json_free(in_data);                                                                  // of course we clean up ;-)
  }                                                                                      //
  sb_t* params = sb_new("[{");                                                           // now we create the transactionobject as json-argument.
  if (to) {                                                                              // if this is a deployment we must not include the to-property
    sb_add_chars(params, "\"to\":\"");
    sb_add_chars(params, to);
    sb_add_chars(params, "\" ");
  }
  if (req || data) {                                                  // if we have a request context or explicitly data we create the data-property
    if (params->len > 2) sb_add_char(params, ',');                    // add comma if this is not the first argument
    sb_add_chars(params, "\"data\":");                                // we will have a data-property
    if (req && data) {                                                // if we have a both, we need to concat thewm (this is the case when depkloying a contract with constructorarguments)
      uint8_t* full = _malloc(req->call_data->b.len - 4 + data->len); // in this case we skip the functionsignature.
      memcpy(full, data->data, data->len);
      memcpy(full + data->len, req->call_data->b.data + 4, req->call_data->b.len - 4);
      bytes_t bb = bytes(full, req->call_data->b.len - 4 + data->len);
      sb_add_bytes(params, "", &bb, 1, false);
      _free(full);
    } else if (req)
      sb_add_bytes(params, "", &req->call_data->b, 1, false);
    else if (data)
      sb_add_bytes(params, "", data, 1, false);
  }

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
// decode pk
void read_pk(char* pk_file, char* pwd, in3_t* c, char* method) {
  if (pk_file) {
    if (!pwd) {
      fprintf(stderr, "Passphrase:\n");
      pwd = malloc(500);
      read_pass(pwd, 500);
    }
    char* content;
    if (strcmp(pk_file, "-") == 0)
      content = (char*) readFile(stdin).data;
    else if (pk_file[0] == '{')
      content = pk_file;
    else
      content = (char*) readFile(fopen(pk_file, "r")).data;

    json_ctx_t* key_json = parse_json(content);
    if (!key_json) die("invalid json in pk file");

    uint8_t* pk_seed = malloc(32);
    if (decrypt_key(key_json->result, pwd, pk_seed)) die("Invalid key");

    if (!method || strcmp(method, "keystore") == 0 || strcmp(method, "key") == 0) {
      char tmp[64];
      bytes_to_hex(pk_seed, 32, tmp);
      printf("0x%s\n", tmp);
      exit(0);
    } else
      eth_set_pk_signer(c, pk_seed);
  }
}

static bytes_t*  last_response;
static bytes_t   in_response = {.data = NULL, .len = 0};
static in3_ret_t debug_transport(in3_request_t* req) {
#ifndef DEBUG
  if (debug_mode)
    fprintf(stderr, "send request to %s: \n" COLORT_RYELLOW "%s" COLORT_RESET "\n", req->urls_len ? req->urls[0] : "none", req->payload);
#endif
  if (in_response.len) {
    for (int i = 0; i < req->urls_len; i++)
      sb_add_range(&req->results[i].result, (char*) in_response.data, 0, in_response.len);
    return 0;
  }
#ifdef USE_CURL
  in3_ret_t r = send_curl(req);
#else
  in3_ret_t r = send_http(req);
#endif
  last_response = b_new(req->results[0].result.data, req->results[0].result.len);
#ifndef DEBUG
  if (debug_mode) {
    if (req->results[0].result.len)
      fprintf(stderr, "success response \n" COLORT_RGREEN "%s" COLORT_RESET "\n", req->results[0].result.data);
    else
      fprintf(stderr, "error response \n" COLORT_RRED "%s" COLORT_RESET "\n", req->results[0].error.data);
  }
#endif
  return r;
}
static char*     test_name = NULL;
static in3_ret_t test_transport(in3_request_t* req) {
#ifdef USE_CURL
  in3_ret_t r = send_curl(req);
#else
  in3_ret_t r = send_http(req);
#endif
  if (r == IN3_OK) {
    req->payload[strlen(req->payload) - 1] = 0;
    printf("[{ \"descr\": \"%s\",\"chainId\": \"0x1\", \"verification\": \"proof\",\"binaryFormat\": false, \"request\": %s, \"response\": %s }]", test_name, req->payload + 1, req->results->result.data);
    exit(0);
  }

  return r;
}

int main(int argc, char* argv[]) {
  // check for usage
  if (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-help") == 0)) {
    show_help(argv[0]);
    return 0;
  }

  if (argc >= 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-version") == 0)) {
    printf("in3 " IN3_VERSION "\nbuild " __DATE__ " with");
#ifdef TEST
    printf(" -DTEST=true");
#endif
#ifdef EVM_GAS
    printf(" -DEVM_GAS=true");
#endif
#ifdef CMD
    printf(" -DCMD=true");
#endif
#ifdef IN3_MATH_FAST
    printf(" -DFAST_MATH=true");
#endif
#ifdef IN3_SERVER
    printf(" -DIN3_SERVER=true");
#endif
#ifdef USE_CURL
    printf(" -DUSE_CURL=true");
#else
    printf(" -DUSE_CURL=false");
#endif
    printf("\n(c) " IN3_COPYRIGHT "\n");
    return 0;
  }

  // define vars
  char *method = NULL, params[50000];
  params[0]    = '[';
  params[1]    = 0;
  int       p  = 1, i;
  bytes32_t pk;

  // use the storagehandler to cache data in .in3
  in3_storage_handler_t storage_handler;
  storage_handler.get_item = storage_get_item;
  storage_handler.set_item = storage_set_item;
  storage_handler.clear    = storage_clear;
  storage_handler.cptr     = NULL;

  // we want to verify all
  in3_register_eth_full();
#ifdef IPFS
  in3_register_ipfs();
#endif
#ifdef BTC
  in3_register_btc();
#endif
  in3_register_eth_api();
  in3_log_set_level(LOG_INFO);

  // create the client
  in3_t* c                         = in3_for_chain(0);
  c->transport                     = debug_transport;
  c->request_count                 = 1;
  c->cache                         = &storage_handler;
  bool            out_response     = false;
  bool            run_test_request = false;
  bool            force_hex        = false;
  char*           sig              = NULL;
  char*           to               = NULL;
  char*           block_number     = "latest";
  char*           name             = NULL;
  call_request_t* req              = NULL;
  bool            json             = false;
  uint64_t        gas_limit        = 100000;
  char*           value            = NULL;
  bool            wait             = false;
  char*           pwd              = NULL;
  char*           pk_file          = NULL;
  char*           validators       = NULL;
  bytes_t*        data             = NULL;
  char*           port             = NULL;
  char*           sig_type         = "raw";
  bool            to_eth           = false;
#ifdef __MINGW32__
  c->flags |= FLAGS_HTTP;
#endif
#ifndef USE_CURL
  c->flags |= FLAGS_HTTP;
#endif
  // handle clear cache opt before initializing cache
  for (i = 1; i < argc; i++)
    if (strcmp(argv[i], "-ccache") == 0)
      c->cache->clear(c->cache->cptr);

  // read data from cache
  in3_cache_init(c);

  // check env
  if (getenv("IN3_PK")) {
    hex_to_bytes(getenv("IN3_PK"), -1, pk, 32);
    eth_set_pk_signer(c, pk);
  }

  if (getenv("IN3_CHAIN"))
    set_chain_id(c, getenv("IN3_CHAIN"));

  // fill from args
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-pk") == 0) { // private key?
      if (argv[i + 1][0] == '0' && argv[i + 1][1] == 'x') {
        hex_to_bytes(argv[++i], -1, pk, 32);
        eth_set_pk_signer(c, pk);
      } else
        pk_file = argv[++i];
    } else if (strcmp(argv[i], "-chain") == 0 || strcmp(argv[i], "-c") == 0) // chain_id
      set_chain_id(c, argv[++i]);
    else if (strcmp(argv[i], "-ccache") == 0) // NOOP - should have been handled earlier
      ;
    else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "-data") == 0) { // data
      char* d = argv[++i];
      if (strcmp(d, "-") == 0)
        data = get_std_in();
      else if (*d == '0' && d[1] == 'x')
        data = hex_to_new_bytes(d + 2, strlen(d) - 2);
      else {
        FILE*   f       = fopen(d, "r");
        bytes_t content = readFile(f);
        data            = hex_to_new_bytes((char*) content.data + 2, content.len - 2);
        fclose(f);
      }
    } else if (strcmp(argv[i], "-block") == 0 || strcmp(argv[i], "-b") == 0)
      block_number = argv[++i];
    else if (strcmp(argv[i], "-latest") == 0 || strcmp(argv[i], "-l") == 0)
      c->replace_latest_block = atoll(argv[++i]);
    else if (strcmp(argv[i], "-tr") == 0)
      run_test_request = true;
    else if (strcmp(argv[i], "-eth") == 0)
      to_eth = true;
    else if (strcmp(argv[i], "-md") == 0)
      c->min_deposit = atoll(argv[++i]);
    else if (strcmp(argv[i], "-kin3") == 0)
      c->flags |= FLAGS_KEEP_IN3;
    else if (strcmp(argv[i], "-to") == 0)
      to = argv[++i];
    else if (strcmp(argv[i], "-gas") == 0 || strcmp(argv[i], "-gas_limit") == 0)
      gas_limit = atoll(argv[++i]);
    else if (strcmp(argv[i], "-test") == 0) {
      test_name    = argv[++i];
      c->transport = test_transport;
    } else if (strcmp(argv[i], "-pwd") == 0)
      pwd = argv[++i];
    else if (strcmp(argv[i], "-q") == 0)
      in3_log_set_level(LOG_FATAL);
    else if (strcmp(argv[i], "-value") == 0)
      value = get_wei(argv[++i]);
    else if (strcmp(argv[i], "-port") == 0)
      port = argv[++i];
    else if (strcmp(argv[i], "-rc") == 0)
      c->request_count = atoi(argv[++i]);
    else if (strcmp(argv[i], "-a") == 0)
      c->max_attempts = atoi(argv[++i]);
    else if (strcmp(argv[i], "-name") == 0)
      name = argv[++i];
    else if (strcmp(argv[i], "-validators") == 0)
      validators = argv[++i];
    else if (strcmp(argv[i], "-hex") == 0)
      force_hex = true;
    else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "-finality") == 0)
      c->finality = (uint16_t) atoi(argv[++i]);
    else if (strcmp(argv[i], "-response-out") == 0 || strcmp(argv[i], "-ro") == 0)
      out_response = true;
    else if (strcmp(argv[i], "-response-in") == 0 || strcmp(argv[i], "-ri") == 0)
      in_response = readFile(stdin);
    else if (strcmp(argv[i], "-wait") == 0 || strcmp(argv[i], "-w") == 0)
      wait = true;
    else if (strcmp(argv[i], "-json") == 0)
      json = true;
    else if (strcmp(argv[i], "-k") == 0) {
      if (argc <= i + 1 || strlen(argv[i + 1]) > 66) die("Invalid signer key");
      c->key = _calloc(32, 1);
      hex_to_bytes(argv[++i], -1, c->key, 32);
    } else if (strcmp(argv[i], "-np") == 0)
      c->proof = PROOF_NONE;
    else if (strcmp(argv[i], "-ns") == 0)
      c->flags ^= FLAGS_STATS;
    else if (strcmp(argv[i], "-sigtype") == 0 || strcmp(argv[i], "-st") == 0)
      sig_type = argv[++i];
    else if (strcmp(argv[i], "-debug") == 0) {
      in3_log_set_level(LOG_TRACE);
      debug_mode = true;
    } else if (strcmp(argv[i], "-signs") == 0 || strcmp(argv[i], "-s") == 0)
      c->signature_count = atoi(argv[++i]);
    else if (strcmp(argv[i], "-proof") == 0 || strcmp(argv[i], "-p") == 0) {
      if (strcmp(argv[i + 1], "none") == 0)
        c->proof = PROOF_NONE;
      else if (strcmp(argv[i + 1], "standard") == 0)
        c->proof = PROOF_STANDARD;
      else if (strcmp(argv[i + 1], "full") == 0)
        c->proof = PROOF_FULL;
      else
        die("Invalid Argument for proof, must be none, standard or full");
      i++;
    }
    // now handle arguments for special methods
    else {
      if (method == NULL)
        method = argv[i];
      else if (strcmp(method, "keystore") == 0 || strcmp(method, "key") == 0)
        pk_file = argv[i];
      else if (strcmp(method, "sign") == 0 && !data)
        data = b_new(argv[i], strlen(argv[i]));
      else if (sig == NULL && (strcmp(method, "call") == 0 || strcmp(method, "send") == 0 || strcmp(method, "abi_encode") == 0 || strcmp(method, "abi_decode") == 0))
        sig = argv[i];
      else {
        // otherwise we add it to the params
        if (p > 1) params[p++] = ',';
        if (*argv[i] >= '0' && *argv[i] <= '9' && *(argv[i] + 1) != 'x' && strlen(argv[i]) < 16)
          p += sprintf(params + p, "\"0x%x\"", atoi(argv[i]));
        else
          p += sprintf(params + p,
                       (argv[i][0] == '{' || argv[i][0] == '[' || strcmp(argv[i], "true") == 0 || strcmp(argv[i], "false") == 0 || (*argv[i] >= '0' && *argv[i] <= '9' && strlen(argv[i]) < 16 && *(argv[i] + 1) != 'x'))
                           ? "%s"
                           : "\"%s\"",
                       strcmp(method, "in3_ens") ? resolve(c, argv[i]) : argv[i]);
      }
    }
  }
  params[p++]  = ']';
  params[p]    = 0;
  char *result = NULL, *error = NULL;

#ifdef IN3_SERVER
  // start server
  if (!method && port) {
    http_run_server(port, c);
    return 0;
  }
#else
  (void) (port);
#endif

  // handle private key
  if (pk_file) read_pk(pk_file, pwd, c, method);

  // no proof for rpc-chain
  if (c->chain_id == 0xFFFF) c->proof = PROOF_NONE;

  // execute the method
  if (sig && *sig == '-') die("unknown option");
  if (!method) {
    in3_log_info("in3 " IN3_VERSION " - reading json-rpc from stdin. (exit with ctrl C)\n________________________________________________\n");
    execute(c, stdin);
    return EXIT_SUCCESS;
  }
  if (*method == '-') die("unknown option");

  // call -> eth_call
  if (strcmp(method, "call") == 0) {
    req    = prepare_tx(sig, resolve(c, to), params, block_number, 0, NULL, data);
    method = "eth_call";
  } else if (strcmp(method, "abi_encode") == 0) {
    if (!sig) die("missing signature");
    req = parseSignature(sig);
    if (req && req->in_data->type == A_TUPLE) {
      json_ctx_t* in_data = parse_json(params);
      if (set_data(req, in_data->result, req->in_data) < 0) die("invalid arguments for given signature");
    }
    if (!req || !req->call_data) die("missing call data");
    print_hex(req->call_data->b.data, req->call_data->b.len);
    return 0;
  } else if (strcmp(method, "abi_decode") == 0) {
    if (!sig) die("missing signature");
    if (!strchr(sig, ':')) {
      char* tmp = malloc(strlen(sig) + 5);
      strcpy(tmp, "d():");
      strcpy(tmp + 4, sig);
      sig = tmp;
    }
    json_ctx_t* res = req_parse_result(parseSignature(sig), d_to_bytes(d_get_at(parse_json(params)->result, 0)));
    if (json)
      printf("%s\n", d_create_json(res->result));
    else
      print_val(res->result);
    return 0;
#ifdef IPFS
  } else if (strcmp(method, "ipfs_get") == 0) {
    c->chain_id = ETH_CHAIN_ID_IPFS;
    int size    = strlen(params);
    if (p == 1 || params[1] != '"' || size < 20 || strstr(params + 2, "\"") == NULL) die("missing ipfs has");
    params[size - 2] = 0;
    bytes_t* content = ipfs_get(c, params + 2);
    if (!content) die("IPFS hash not found!");
    fwrite(content->data, content->len, 1, stdout);
    fflush(stdout);
    return 0;

  } else if (strcmp(method, "ipfs_put") == 0) {
    c->chain_id         = ETH_CHAIN_ID_IPFS;
    bytes_t data        = readFile(stdin);
    data.data[data.len] = 0;
    printf("%s\n", ipfs_put(c, &data));
    return 0;

#endif
  } else if (strcmp(method, "in3_weights") == 0) {
    c->max_attempts = 1;
    uint32_t block = 0, b = 0;
    BIT_CLEAR(c->flags, FLAGS_AUTO_UPDATE_LIST);
    uint64_t     now   = in3_time(NULL);
    in3_chain_t* chain = in3_find_chain(c, c->chain_id);
    printf("   : %45s : %7s : %5s : %5s: %s\n----------------------------------------------------------------------------------------\n", "URL", "BL", "CNT", "AVG", run_test_request ? "WEIGHT   : LAST_BLOCK" : "WEIGHT");
    for (int i = 0; i < chain->nodelist_length; i++) {
      in3_ctx_t* ctx = NULL;
      if (run_test_request) {
        char req[300];
        char adr[41];
        bytes_to_hex((chain->nodelist + i)->address->data, 20, adr);
        sprintf(req, "{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\",\"params\":[],\"in3\":{\"dataNodes\":[\"0x%s\"]}}", adr);
        ctx = ctx_new(c, req);
        if (ctx) in3_send_ctx(ctx);
      }
      in3_node_t*        node        = chain->nodelist + i;
      in3_node_weight_t* weight      = chain->weights + i;
      uint64_t           blacklisted = weight->blacklisted_until > now ? weight->blacklisted_until : 0;
      uint32_t           calc_weight = in3_node_calculate_weight(weight, node->capacity);
      char *             tr = NULL, *warning = NULL;
      if (ctx) {
        tr = _malloc(300);
        if (!ctx->error) {
          b = d_get_intk(ctx->responses[0], K_RESULT);
          if (block < b) block = b;

          if (b < block - 1)
            sprintf((warning = tr), "#%i ( out of sync : %i blocks behind latest )", b, block - b);
          else if (strncmp(node->url, "https://", 8))
            sprintf((warning = tr), "#%i (missing https, which is required in a browser )", b);
          else
            sprintf(tr, "#%i", b);
        } else if (!strlen(node->url) || !node->props)
          sprintf((warning = tr), "No URL spcified anymore props = %i ", (int) (node->props & 0xFFFFFF));
        else if ((node->props & NODE_PROP_DATA) == 0)
          sprintf((warning = tr), "The node is marked as not supporting Data-Providing");
        else if (c->proof != PROOF_NONE && (node->props & NODE_PROP_PROOF) == 0)
          sprintf((warning = tr), "The node is marked as able to provide proof");
        else if ((c->flags & FLAGS_HTTP) && (node->props & NODE_PROP_HTTP) == 0)
          sprintf((warning = tr), "The node is marked as able to support http-requests");
        else
          tr = ctx->error;
      }
      if (blacklisted)
        printf(COLORT_RED);
      else if (warning)
        printf(COLORT_YELLOW);
      else
        printf(COLORT_GREEN);
      printf("%2i   %45s   %7i   %5i   %5i  %5i %s", i, node->url, (int) (blacklisted ? blacklisted - now : 0), weight->response_count, weight->response_count ? (weight->total_response_time / weight->response_count) : 0, calc_weight, tr ? tr : "");
      printf(COLORT_RESET "\n");
      if (tr && tr != ctx->error) _free(tr);
      if (ctx) ctx_free(ctx);
    }

    return 0;
  } else if (strcmp(method, "send") == 0) {
    prepare_tx(sig, resolve(c, to), params, NULL, gas_limit, value, data);
    method = "eth_sendTransaction";
  } else if (strcmp(method, "sign") == 0) {
    if (!data) die("no data given");
    if (data->len > 2 && data->data[0] == '0' && data->data[1] == 'x')
      data = hex_to_new_bytes((char*) data->data + 2, data->len - 2);
    if (strcmp(sig_type, "eth_sign") == 0) {
      char* tmp = alloca(data->len + 30);
      int   l   = sprintf(tmp, "\x19"
                           "Ethereum Signed Message:\n%i",
                      data->len);
      memcpy(tmp + l, data->data, data->len);
      data     = b_new(tmp, l + data->len);
      sig_type = "raw";
    }

    if (!c->signer) die("No private key given");
    uint8_t   sig[65];
    in3_ctx_t ctx;
    ctx.client = c;
    c->signer->sign(&ctx, strcmp(sig_type, "hash") == 0 ? SIGN_EC_RAW : SIGN_EC_HASH, *data, bytes(NULL, 0), sig);
    sig[64] += 27;
    print_hex(sig, 65);
    return 0;
  } else if (strcmp(method, "chainspec") == 0) {
    char* json;
    if (strlen(params) > 2) {
      params[strlen(params) - 2] = 0;
      json                       = (char*) readFile(fopen(params + 2, "r")).data;
    } else
      json = (char*) readFile(stdin).data;
    d_track_keynames(1);
    json_ctx_t*  j    = parse_json(json);
    chainspec_t* spec = chainspec_create_from_json(j->result);
    if (validators) {
      // first PoA without validators-list
      for (uint32_t i = 0; i < spec->consensus_transitions_len; i++) {
        if (spec->consensus_transitions[i].validators.len == 0) {
          spec->consensus_transitions[i].validators = *hex_to_new_bytes(validators + 2, strlen(validators) - 2);
          break;
        }
      }
    }
    bytes_builder_t* bb = bb_new();
    chainspec_to_bin(spec, bb);

    if (force_hex)
      print_hex(bb->b.data, bb->b.len);
    else {
      bool is_hex = false;
      printf("#define CHAINSPEC_%s \"", name);
      for (i = 0; i < (int) bb->b.len; i++) {
        uint8_t c = bb->b.data[i];
        if (is_hex && ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) printf("\" \"");
        is_hex = c < ' ' || c > 0x7E || c == 0x5C || c == '"';
        printf(is_hex ? "\\x%02x" : "%c", c);
      }
      printf("\"\n");
    }

    return 0;
  } else if (strcmp(method, "autocompletelist") == 0) {
    printf("send call abi_encode abi_decode ipfs_get ipfs_put ecrecover key -sigtype -st eth_sign raw hash sign createkey -ri -ro keystore unlock pk2address pk2public mainnet tobalaba kovan goerli local volta true false latest -np -debug -c -chain -p -version -proof -s -signs -b -block -to -d -data -gas_limit -value -w -wait -hex -json in3_nodeList in3_stats in3_sign web3_clientVersion web3_sha3 net_version net_peerCount net_listening eth_protocolVersion eth_syncing eth_coinbase eth_mining eth_hashrate eth_gasPrice eth_accounts eth_blockNumber eth_getBalance eth_getStorageAt eth_getTransactionCount eth_getBlockTransactionCountByHash eth_getBlockTransactionCountByNumber eth_getUncleCountByBlockHash eth_getUncleCountByBlockNumber eth_getCode eth_sign eth_sendTransaction eth_sendRawTransaction eth_call eth_estimateGas eth_getBlockByHash eth_getBlockByNumber eth_getTransactionByHash eth_getTransactionByBlockHashAndIndex eth_getTransactionByBlockNumberAndIndex eth_getTransactionReceipt eth_pendingTransactions eth_getUncleByBlockHashAndIndex eth_getUncleByBlockNumberAndIndex eth_getCompilers eth_compileLLL eth_compileSolidity eth_compileSerpent eth_newFilter eth_newBlockFilter eth_newPendingTransactionFilter eth_uninstallFilter eth_getFilterChanges eth_getFilterLogs eth_getLogs eth_getWork eth_submitWork eth_submitHashrate in3_cacheClear\n");
    return 0;
  } else if (strcmp(method, "createkey") == 0) {
    time_t t;
    srand((unsigned) time(&t));
    printf("0x");
    for (i = 0; i < 32; i++) printf("%02x", rand() % 256);
    printf("\n");
    return 0;
  } else if (strcmp(method, "pk2address") == 0) {
    bytes32_t prv_key;
    uint8_t   public_key[65], sdata[32];
    hex_to_bytes(argv[argc - 1], -1, prv_key, 32);
    bytes_t pubkey_bytes = {.data = public_key + 1, .len = 64};
    ecdsa_get_public_key65(&secp256k1, prv_key, public_key);
    sha3_to(&pubkey_bytes, sdata);
    printf("0x");
    for (i = 0; i < 20; i++) printf("%02x", sdata[i + 12]);
    printf("\n");
    return 0;
  } else if (strcmp(method, "pk2public") == 0) {
    bytes32_t prv_key;
    uint8_t   public_key[65];
    hex_to_bytes(argv[argc - 1], -1, prv_key, 32);
    ecdsa_get_public_key65(&secp256k1, prv_key, public_key);
    print_hex(public_key + 1, 64);
    return 0;
  } else if (strcmp(method, "ecrecover") == 0) {
    json_ctx_t* rargs = parse_json(params);
    if (!rargs || d_len(rargs->result) < 2) die("Invalid arguments for recovery args must be : <message> <signature> ");
    bytes_t   msg = d_to_bytes(d_get_at(rargs->result, 0));
    bytes_t   sig = d_to_bytes(d_get_at(rargs->result, 1));
    bytes32_t hash;
    uint8_t   pub[65];
    bytes_t   pubkey_bytes = {.len = 64, .data = ((uint8_t*) &pub) + 1};
    if (strcmp(sig_type, "eth_sign") == 0) {
      char* tmp = alloca(msg.len + 30);
      int   l   = sprintf(tmp, "\x19"
                           "Ethereum Signed Message:\n%i",
                      msg.len);
      memcpy(tmp + l, msg.data, msg.len);
      msg = *b_new(tmp, l + msg.len);
    }
    if (strcmp(sig_type, "hash") == 0) {
      if (msg.len != 32) die("The message hash must be 32 byte");
      memcpy(hash, msg.data, 32);
    } else
      sha3_to(&msg, hash);
    if (sig.len != 65) die("The signature must be 65 bytes");

    if (ecdsa_recover_pub_from_sig(&secp256k1, pub, sig.data, hash, sig.data[64] >= 27 ? sig.data[64] - 27 : sig.data[64]))
      die("Invalid Signature");

    sha3_to(&pubkey_bytes, hash);
    print_hex(hash + 12, 20);
    print_hex(pub + 1, 64);
    return 0;
  }

  in3_log_debug("..sending request %s %s\n", method, params);

  // send the request
  in3_client_rpc(c, method, params, &result, &error);

  // Update nodelist if a newer latest block was reported
  if (in3_find_chain(c, c->chain_id)->nodelist_upd8_params && in3_find_chain(c, c->chain_id)->nodelist_upd8_params->exp_last_block) {
    char *r = NULL, *e = NULL;
    in3_client_rpc(c, "eth_blockNumber", "", &r, &e);
  }

  // if we need to wait
  if (!error && result && wait && strcmp(method, "eth_sendTransaction") == 0) {
    bytes32_t txHash;
    hex_to_bytes(result + 3, 64, txHash, 32);
    result = eth_wait_for_receipt(c, txHash);
    if (!result) die("Error waiting for the confirmation of the transaction");
  }

  if (error)
    die(error);
  else if (!result)
    die("No result");
  else {

    if (out_response && last_response) {
      char* r = alloca(last_response->len + 1);
      memcpy(r, last_response->data, last_response->len);
      r[last_response->len] = 0;
      printf("%s\n", r);
      return 0;
    }

    // if the result is a string, we remove the quotes
    if (result[0] == '"' && result[strlen(result) - 1] == '"') {
      memmove(result, result + 1, strlen(result) + 1);
      result[strlen(result) - 1] = 0;
    }

    // if the request was a eth_call, we decode the result
    if (req) {
      int l = strlen(result) / 2 - 1;
      if (l) {
        uint8_t*    tmp = alloca(l + 1);
        json_ctx_t* res = req_parse_result(req, bytes(tmp, hex_to_bytes(result, -1, tmp, l + 1)));
        if (json)
          printf("%s\n", d_create_json(res->result));
        else
          print_val(res->result);
      }
      // if not we simply print the result
    } else {
      if (to_eth && result[0] == '0' && result[1] == 'x' && strlen(result) <= 18) {
        double val = char_to_long(result, strlen(result));
        printf("%.3f\n", val / 1000000000000000000L);
      } else if (!force_hex && result[0] == '0' && result[1] == 'x' && strlen(result) <= 18)
        printf("%" PRIu64 "\n", char_to_long(result, strlen(result)));
      else
        printf("%s\n", result);
    }
  }
  return EXIT_SUCCESS;
}
