/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2019 slock.it GmbH, Blockchains LLC
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

#include "../../api/eth1/abi.h"
#include "../../api/eth1/eth_api.h"
#include "../../core/client/client.h"
#include "../../core/client/context.h"
#include "../../core/client/send.h"
#include "../../core/util/mem.h"
#include "../../verifier/eth1/full/eth_full.h"
#include <emscripten.h>
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
EM_JS(char*, in3_cache_get, (char* key), {
  var val = Module.in3_cache.get(UTF8ToString(key));
  if (val) {
    var len = (val.length << 2) + 1;
    var ret = stackAlloc(len); 
    stringToUTF8(val, ret, len);
    return ret;
  }
  return 0;
})
EM_JS(void, in3_cache_set, (char* key, char* val), {
  Module.in3_cache.set(UTF8ToString(key),UTF8ToString(val));
})
// clang-format on

bytes_t* storage_get_item(void* cptr, char* key) {
  UNUSED_VAR(cptr);
  char*    val = in3_cache_get(key);
  bytes_t* res = val ? hex2byte_new_bytes(val, strlen(val)) : NULL;
  return res;
}

void storage_set_item(void* cptr, char* key, bytes_t* content) {
  UNUSED_VAR(cptr);
  char buffer[content->len * 2 + 1];
  bytes_to_hex(content->data, content->len, buffer);
  in3_cache_set(key, buffer);
}

// clang-format off
EM_JS(void, transport_send, (in3_response_t* result,  char* url, char* payload), {
  Asyncify.handleSleep(function(wakeUp) {
    Module.transport(UTF8ToString(url),UTF8ToString(payload))
      .then(res => {
        Module.ccall('request_set_result','void',['number','string'],[result,res]);
        wakeUp();
      })
      .catch(res => {
        Module.ccall('request_set_error','void',['number','string'],[result,res.message || res]);
        wakeUp();
      })
  });
});

EM_JS(void, in3_req_done, (in3_ctx_t* ctx), {
  Module.pendingRequests[ctx+""]();

});

// clang-format on

int in3_fetch(char** urls, int urls_len, char* payload, in3_response_t* result) {
  for (int i = 0; i < urls_len; i++)
    transport_send(result + i, urls[i], payload);
  return IN3_OK;
}

static char* last_error = NULL;

static void in3_set_error(char* data) {
  if (last_error) free(last_error);
  last_error = data ? _strdupn(data, -1) : NULL;
}

in3_t* EMSCRIPTEN_KEEPALIVE in3_create() {
  // register a chain-verifier for full Ethereum-Support
  in3_register_eth_full();

  in3_t* c                  = in3_new();
  c->transport              = in3_fetch;
  c->cacheStorage           = malloc(sizeof(in3_storage_handler_t));
  c->cacheStorage->get_item = storage_get_item;
  c->cacheStorage->set_item = storage_set_item;

  in3_cache_init(c);

  in3_set_error(NULL);
  return c;
}
/* frees the references of the client */
void EMSCRIPTEN_KEEPALIVE in3_dispose(in3_t* a) {
  in3_free(a);
  in3_set_error(NULL);
}
/* frees the references of the client */
in3_ret_t EMSCRIPTEN_KEEPALIVE in3_config(in3_t* a, char* conf) {
  return in3_configure(a, conf);
}

char* EMSCRIPTEN_KEEPALIVE in3_last_error() {
  return last_error;
}

in3_ctx_t* EMSCRIPTEN_KEEPALIVE in3_create_request(in3_t* c, char* payload) {
  char*      src_data = _strdupn(payload, -1);
  in3_ctx_t* ctx      = new_ctx(c, src_data);
  if (ctx->error) {
    in3_set_error(ctx->error);
    free_ctx(ctx);
    return NULL;
  }
  return ctx;
}

void EMSCRIPTEN_KEEPALIVE in3_send_request(in3_ctx_t* ctx) {
  in3_set_error(NULL);
  in3_send_ctx(ctx);
  ctx->client = NULL;
  in3_req_done(ctx);
}

void EMSCRIPTEN_KEEPALIVE in3_free_request(in3_ctx_t* ctx) {
  if (ctx->request_context && ctx->request_context->c) free(ctx->request_context->c);
  free_ctx(ctx);
}

bool EMSCRIPTEN_KEEPALIVE request_is_done(in3_ctx_t* r) {
  return r->client == NULL;
}

char* EMSCRIPTEN_KEEPALIVE request_get_result(in3_ctx_t* r) {
  if (r->error) return NULL;
  // we have a result and copy it
  str_range_t s = d_to_json(r->responses[0]);
  s.data[s.len] = 0;
  return s.data;
}

char* EMSCRIPTEN_KEEPALIVE request_get_error(in3_ctx_t* r) {
  return r->error;
}

void EMSCRIPTEN_KEEPALIVE request_set_result(in3_response_t* r, char* data) {
  sb_add_chars(&r->result, data);
}

void EMSCRIPTEN_KEEPALIVE request_set_error(in3_response_t* r, char* data) {
  sb_add_chars(&r->error, data);
}

char* EMSCRIPTEN_KEEPALIVE abi_encode(char* fn_sig, char* args) {
  call_request_t* req = parseSignature(fn_sig);                                  // parse it and create a call_request.
  if (req && req->in_data->type == A_TUPLE) {                                    // if type is a tuple, it means we have areuments we need to parse.
    json_ctx_t* in_data = parse_json(args);                                      // the args are passed as a "[]"- json-array string.
    if (set_data(req, in_data->result, req->in_data) < 0) return "invalid data"; // we then set the data, which appends the arguments to the functionhash.
    free_json(in_data);                                                          // of course we clean up ;-)
  }
  char* result = malloc(req->call_data->b.len * 2 + 3);
  result[0]    = '0';
  result[1]    = 'x';
  bytes_to_hex(req->call_data->b.data, req->call_data->b.len, result + 2);
  return result;
}

uint8_t* EMSCRIPTEN_KEEPALIVE keccak(uint8_t* data, int len) {
  bytes_t  src    = bytes(data, len);
  uint8_t* result = malloc(32);
  if (result)
    sha3_to(&src, result);
  else
    in3_set_error("malloc failed");

  return result;
}

char* EMSCRIPTEN_KEEPALIVE to_checksum_address(address_t adr, int chain_id) {
  char* result = malloc(43);
  if (result)
    to_checksum(adr, chain_id, result);
  else
    in3_set_error("malloc failed");
  return result;
}
