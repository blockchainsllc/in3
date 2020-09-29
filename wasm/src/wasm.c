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
#include "../../c/src/core/client/cache.h"
#include "../../c/src/core/client/context_internal.h"
#include "../../c/src/core/client/keys.h"
#include "../../c/src/core/client/nodelist.h"
#include "../../c/src/core/client/plugin.h"
#include "../../c/src/core/client/version.h"
#include "../../c/src/core/util/mem.h"
#include "../../c/src/third-party/crypto/ecdsa.h"
#include "../../c/src/third-party/crypto/secp256k1.h"
#include "../../c/src/verifier/in3_init.h"

#ifdef IPFS
#include "../../c/src/third-party/libb64/cdecode.h"
#include "../../c/src/third-party/libb64/cencode.h"
#endif
#include <emscripten.h>
#include <string.h>
#include <time.h>

#ifdef ETH_API
#include "../../c/src/api/eth1/abi.h"
#include "../../c/src/api/utils/api_utils.h"
#endif

#define err_string(msg) (":ERROR:" msg)
#define BLACKLISTTIME   24 * 3600

static char*    last_error = NULL;
static uint32_t now() {
  static uint64_t time_offset = 0;
  if (!time_offset) time_offset = current_ms();
  return (uint32_t)(current_ms() - time_offset);
}
void EMSCRIPTEN_KEEPALIVE in3_set_error(char* data) {
  if (last_error) free(last_error);
  last_error = data ? _strdupn(data, -1) : NULL;
}

/*
static char* to_hex_string(uint8_t* data, int l) {
  char* res = malloc((l << 1) + 3);
  res[0]    = '0';
  res[1]    = 'x';
  bytes_to_hex(data, l, res + 2);
  return res;
}
*/
// --------------- storage -------------------
// clang-format off
EM_JS(char*, in3_cache_get, (const char* key), {
  var val = Module.in3_cache.get(UTF8ToString(key));
  if (val) {
    var len = (val.length << 2) + 1;
    var ret = stackAlloc(len);
    stringToUTF8(val, ret, len);
    return ret;
  }
  return 0;
})

EM_JS(void, in3_cache_set, (const char* key, char* val), {
  Module.in3_cache.set(UTF8ToString(key),UTF8ToString(val));
})


char* EMSCRIPTEN_KEEPALIVE  in3_version() {
    return IN3_VERSION;
}
// clang-format on

bytes_t* storage_get_item(void* cptr, const char* key) {
  UNUSED_VAR(cptr);
  char*    val = in3_cache_get(key);
  bytes_t* res = val ? hex_to_new_bytes(val, strlen(val)) : NULL;
  return res;
}

void storage_set_item(void* cptr, const char* key, bytes_t* content) {
  UNUSED_VAR(cptr);
  char buffer[content->len * 2 + 1];
  bytes_to_hex(content->data, content->len, buffer);
  in3_cache_set(key, buffer);
}

EM_JS(int, plgn_exec_term, (in3_t * c, int index), {
  var client = Module.clients[c];
  var plgn   = client && client.plugins[index];
  if (!plgn) return -4;
  return plgn.term(client) || 0;
})

EM_JS(int, plgn_exec_rpc_handle, (in3_t * c, in3_ctx_t* ctx, char* req, int index), {
  var client = Module.clients[c];
  var plgn   = client && client.plugins[index];
  if (!plgn) return -4;
  try {
    var json = JSON.parse(UTF8ToString(req));
    var val  = plgn.handleRPC(client, json);
    if (typeof(val) == "undefined") return -17;
    if (!val.then) val = Promise.resolve(val);
    var id                 = ++in3w.promiseCount;
    in3w.promises["" + id] = val;
    json.in3               = {rpc : "promise://" + id};
    in3w.ccall("wasm_set_request_ctx", "void", [ "number", "string" ], [ ctx, JSON.stringify(json) ]);
  } catch (x) {
    setResponse(ctx, JSON.stringify({error : {message : x.message || x}}), 0, false)
  }
  return 0;
})

/**
 * the main plgn-function which is called for each js-plugin,
 * delegating it depending on the action.
 */
in3_ret_t wasm_plgn(void* data, in3_plugin_act_t action, void* ctx) {
  // we use the custom data pointer of the plugin as index within the clients plugin array
  int index = (int) data;

  switch (action) {
    case PLGN_ACT_INIT: return IN3_OK;
    case PLGN_ACT_TERM: return plgn_exec_term(ctx, index);
    case PLGN_ACT_RPC_HANDLE: {
      // extract the request as string, so we can pass it to js
      in3_rpc_handle_ctx_t* rc  = ctx;
      str_range_t           sr  = d_to_json(rc->request);
      char*                 req = alloca(sr.len + 1);
      memcpy(req, sr.data, sr.len);
      req[sr.len] = 0;
      return plgn_exec_rpc_handle(rc->ctx->client, rc->ctx, req, index);
    }
    default: break;
  }
  return IN3_ENOTSUP;
}

void EMSCRIPTEN_KEEPALIVE wasm_register_plugin(in3_t* c, in3_plugin_act_t action, int index) {
  // the index is used as the custom void* or data for the plugin.
  // This way we can cast it backward in order doing the call to js to find the plugin
  // if a js-plugin needs custom data, the it should do this in js withihn its own plugin object
  in3_plugin_register(NULL, c, action, wasm_plgn, (void*) index, false);
}

/**
 * repareses the request for the context with a new input.
 */
void EMSCRIPTEN_KEEPALIVE wasm_set_request_ctx(in3_ctx_t* ctx, char* req) {
  if (!ctx->request_context) return;
  char* src = ctx->request_context->c;                                     // we keep the old pointer since this may be an internal request where this needs to be freed.
  json_free(ctx->request_context);                                         // throw away the old pares context
  char* r                                  = _strdupn(req, -1);            // need to copy, because req is on the stack and the pointers of the tokens need to point to valid memory
  ctx->request_context                     = parse_json(r);                // parse the new
  ctx->requests[0]                         = ctx->request_context->result; //since we don't support bulks in custom rpc, requests must be allocated with len=1
  ctx->request_context->c                  = src;                          // set the old pointer, so the memory management will clean it correctly
  in3_cache_add_ptr(&ctx->cache, r)->props = CACHE_PROP_MUST_FREE;         // but add the copy to be cleaned when freeing ctx to avoid memory leaks.
}

/**
 * main execute function which generates a json representing the status and all required data to be handled in js.
 * The resulting string needs to be freed by the caller!
 */
char* EMSCRIPTEN_KEEPALIVE ctx_execute(in3_ctx_t* ctx) {
  in3_ctx_t*     p   = ctx;
  in3_request_t* req = NULL;
  sb_t*          sb  = sb_new("{\"status\":");

  switch (in3_ctx_exec_state(ctx)) {
    case CTX_SUCCESS:
      sb_add_chars(sb, "\"ok\", \"result\":");
      sb_add_chars(sb, ctx->response_context->c);
      break;
    case CTX_ERROR:
      sb_add_chars(sb, "\"error\",\"error\":\"");
      sb_add_escaped_chars(sb, ctx->error ? ctx->error : "Unknown error");
      sb_add_chars(sb, "\"");
      break;
    case CTX_WAITING_FOR_RESPONSE:
      sb_add_chars(sb, "\"waiting\",\"request\":{ \"type\": ");
      sb_add_chars(sb, ctx->type == CT_SIGN ? "\"sign\"" : "\"rpc\"");
      sb_add_chars(sb, ",\"ctx\":");
      sb_add_int(sb, (unsigned int) in3_ctx_last_waiting(ctx));
      sb_add_char(sb, '}');
      break;
    case CTX_WAITING_TO_SEND:
      sb_add_chars(sb, "\"request\"");
      in3_request_t* request = in3_create_request(ctx);
      if (request == NULL) {
        sb_add_chars(sb, ",\"error\",\"");
        sb_add_escaped_chars(sb, ctx->error ? ctx->error : "could not create request");
        sb_add_char(sb, '"');
      }
      else {
        uint32_t start = now();
        sb_add_chars(sb, ",\"request\":{ \"type\": ");
        sb_add_chars(sb, request->ctx->type == CT_SIGN ? "\"sign\"" : "\"rpc\"");
        sb_add_chars(sb, ",\"timeout\":");
        sb_add_int(sb, (uint64_t) request->ctx->client->timeout);
        sb_add_chars(sb, ",\"wait\":");
        sb_add_int(sb, (uint64_t) request->wait);
        sb_add_chars(sb, ",\"payload\":");
        sb_add_chars(sb, request->payload);
        sb_add_chars(sb, ",\"urls\":[");
        for (int i = 0; i < request->urls_len; i++) {
          request->ctx->raw_response[i].time = start;
          if (i) sb_add_char(sb, ',');
          sb_add_char(sb, '"');
          sb_add_escaped_chars(sb, request->urls[i]);
          sb_add_char(sb, '"');
        }
        sb_add_chars(sb, "],\"ctx\":");
        sb_add_int(sb, (uint64_t) request->ctx);
        sb_add_char(sb, '}');
        request_free(request);
      }
      break;
  }

  sb_add_char(sb, '}');

  char* r = sb->data;
  _free(sb);
  return r;
}
void EMSCRIPTEN_KEEPALIVE ifree(void* ptr) {
  _free(ptr);
}
void EMSCRIPTEN_KEEPALIVE wasm_init() {
  in3_init();
}
void* EMSCRIPTEN_KEEPALIVE imalloc(size_t size) {
  return _malloc(size);
}
void EMSCRIPTEN_KEEPALIVE in3_blacklist(in3_t* in3, char* url) {
  in3_chain_t* chain = in3_get_chain(in3);
  if (!chain) return;
  for (int i = 0; i < chain->nodelist_length; i++) {
    if (strcmp(chain->nodelist[i].url, url) == 0) {
      chain->weights[i].blacklisted_until = in3_time(NULL) + BLACKLISTTIME;
      // we don't update weights for local chains.
      if (in3->chain_id == CHAIN_ID_LOCAL) return;
      in3_cache_store_nodelist(in3, chain);
      return;
    }
  }
}

void EMSCRIPTEN_KEEPALIVE ctx_set_response(in3_ctx_t* ctx, int i, int is_error, char* msg) {
  if (!ctx->raw_response) ctx->raw_response = _calloc(sizeof(in3_response_t), i + 1);
  ctx->raw_response[i].time  = now() - ctx->raw_response[i].time;
  ctx->raw_response[i].state = is_error ? IN3_ERPC : IN3_OK;
  if (ctx->type == CT_SIGN && !is_error) {
    int l = (strlen(msg) + 1) / 2;
    if (l && msg[0] == '0' && msg[1] == 'x') l--;
    uint8_t* sig = alloca(l);
    hex_to_bytes(msg, -1, sig, l);
    sb_add_range(&ctx->raw_response[i].data, (char*) sig, 0, l);
  }
  else
    sb_add_chars(&ctx->raw_response[i].data, msg);
}
#ifdef IPFS

uint8_t* EMSCRIPTEN_KEEPALIVE base64Decode(char* input) {
  size_t   len = 0;
  uint8_t* b64 = base64_decode(input, &len);
  return b64;
}

char* EMSCRIPTEN_KEEPALIVE base64Encode(uint8_t* input, int len) {
  char* b64 = base64_encode(input, len);
  return b64;
}
#endif
in3_t* EMSCRIPTEN_KEEPALIVE in3_create(chain_id_t chain) {
  in3_t* c = in3_for_chain(chain);
  in3_set_storage_handler(c, storage_get_item, storage_set_item, NULL, NULL);
  in3_set_error(NULL);
  return c;
}
/* frees the references of the client */
void EMSCRIPTEN_KEEPALIVE in3_dispose(in3_t* a) {
  in3_free(a);
  in3_set_error(NULL);
}
/* frees the references of the client */
bool EMSCRIPTEN_KEEPALIVE in3_is_alive(in3_ctx_t* root, in3_ctx_t* ctx) {
  while (root) {
    if (ctx == root) return true;
    root = root->required;
  }
  return false;
}

char* EMSCRIPTEN_KEEPALIVE in3_config(in3_t* a, char* conf) {
  return in3_configure(a, conf);
}

char* EMSCRIPTEN_KEEPALIVE in3_last_error() {
  return last_error;
}

in3_ctx_t* EMSCRIPTEN_KEEPALIVE in3_create_request_ctx(in3_t* c, char* payload) {
  char*      src_data = _strdupn(payload, -1);
  in3_ctx_t* ctx      = ctx_new(c, src_data);
  if (ctx->error) {
    in3_set_error(ctx->error);
    ctx_free(ctx);
    return NULL;
  }
  // add the src-string as cache-entry so it will be freed when finalizing.
  in3_cache_add_ptr(&ctx->cache, src_data);

  return ctx;
}

void EMSCRIPTEN_KEEPALIVE in3_request_free(in3_ctx_t* ctx) {
  ctx_free(ctx);
}

uint8_t* EMSCRIPTEN_KEEPALIVE hash_keccak(uint8_t* data, int len) {
  uint8_t* result = malloc(32);
  if (result)
    keccak(bytes(data, len), result);
  else
    in3_set_error("malloc failed");

  return result;
}

char* EMSCRIPTEN_KEEPALIVE to_checksum_address(address_t adr, int chain_id) {
  char* result = malloc(43);
  if (!result) return err_string("malloc failed");
#ifdef ETH_API
  to_checksum(adr, chain_id, result);
#else
  UNUSED_VAR(adr);
  UNUSED_VAR(chain_id);
  strcpy(result, err_string("ETH_API deactivated!"));
#endif

  return result;
}

char* EMSCRIPTEN_KEEPALIVE abi_encode(char* sig, char* json_params) {
#ifdef ETH_API
  call_request_t* req = parseSignature(sig);
  if (!req) return err_string("invalid function signature");

  json_ctx_t* params = parse_json(json_params);
  if (!params) {
    req_free(req);
    return err_string("invalid json data");
  }

  if (set_data(req, params->result, req->in_data) < 0) {
    req_free(req);
    json_free(params);
    return err_string("invalid input data");
  }
  json_free(params);
  char* result = malloc(req->call_data->b.len * 2 + 3);
  if (!result) {
    req_free(req);
    return err_string("malloc failed for the result");
  }
  bytes_to_hex(req->call_data->b.data, req->call_data->b.len, result + 2);
  result[0] = '0';
  result[1] = 'x';
  req_free(req);
  return result;
#else
  UNUSED_VAR(sig);
  UNUSED_VAR(json_params);
  return _strdupn(err_string("ETH_API deactivated!"), -1);
#endif
}

char* EMSCRIPTEN_KEEPALIVE abi_decode(char* sig, uint8_t* data, int len) {
#ifdef ETH_API
  call_request_t* req = parseSignature(sig);
  if (!req) return err_string("invalid function signature");
  json_ctx_t* res = req_parse_result(req, bytes(data, len));
  req_free(req);
  if (!res)
    return err_string("the input data can not be decoded");
  char* result = d_create_json(res, res->result);
  json_free(res);
  return result;
#else
  UNUSED_VAR(sig);
  UNUSED_VAR(data);
  UNUSED_VAR(len);
  return _strdupn(err_string("ETH_API deactivated!"), -1);
#endif
}

/** private key to address */
uint8_t* EMSCRIPTEN_KEEPALIVE private_to_address(bytes32_t prv_key) {
  uint8_t* dst = malloc(20);
  uint8_t  public_key[65], sdata[32];
  ecdsa_get_public_key65(&secp256k1, prv_key, public_key);
  keccak(bytes(public_key + 1, 64), sdata);
  memcpy(dst, sdata + 12, 20);
  return dst;
}

/** signs the given data */
uint8_t* EMSCRIPTEN_KEEPALIVE ec_sign(bytes32_t pk, d_signature_type_t type, uint8_t* data, int len, bool adjust_v) {
  uint8_t* dst   = malloc(65);
  int      error = -1;
  switch (type) {
    case SIGN_EC_RAW:
      error = ecdsa_sign_digest(&secp256k1, pk, data, dst, dst + 64, NULL);
      break;
    case SIGN_EC_HASH:
      error = ecdsa_sign(&secp256k1, HASHER_SHA3K, pk, data, len, dst, dst + 64, NULL);
      break;

    default:
      error = -2;
  }
  if (error < 0) {
    free(dst);
    return NULL;
  }
  if (adjust_v) dst[64] += 27;
  return dst;
}