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
#include "../../c/src/core/client/client.h"
#include "../../c/src/core/client/context_internal.h"
#include "../../c/src/core/client/keys.h"
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

char* EMSCRIPTEN_KEEPALIVE  in3_version() {
    return IN3_VERSION;
}
// clang-format on

bytes_t* storage_get_item(void* cptr, char* key) {
  UNUSED_VAR(cptr);
  char*    val = in3_cache_get(key);
  bytes_t* res = val ? hex_to_new_bytes(val, strlen(val)) : NULL;
  return res;
}

void storage_set_item(void* cptr, char* key, bytes_t* content) {
  UNUSED_VAR(cptr);
  char buffer[content->len * 2 + 1];
  bytes_to_hex(content->data, content->len, buffer);
  in3_cache_set(key, buffer);
}

char* EMSCRIPTEN_KEEPALIVE ctx_execute(in3_ctx_t* ctx) {
  in3_ctx_t *p = ctx, *last_waiting = NULL;
  sb_t*      sb = sb_new("{\"status\":");
  switch (in3_ctx_execute(ctx)) {
    case IN3_EIGNORE:
      while (p) {
        if (p->required && p->required->verification_state == IN3_EIGNORE) {
          last_waiting = p;
          break;
        }
        p = p->required;
      }
      if (!last_waiting) {
        sb_add_chars(sb, "\"error\",\"error\":\"could not find the last ignored context\"");
        break;
      } else {
        ctx_handle_failable(last_waiting);
        sb_free(sb);
        return ctx_execute(ctx);
      }
    case IN3_OK:
      sb_add_chars(sb, "\"ok\", \"result\":");
      sb_add_chars(sb, ctx->response_context->c);
      break;
    case IN3_WAITING:
      sb_add_chars(sb, "\"waiting\"");
      while (p) {
        if (!p->raw_response && in3_ctx_state(p) == CTX_WAITING_FOR_RESPONSE)
          last_waiting = p;
        p = p->required;
      }
      if (!last_waiting)
        sb_add_chars(sb, ",\"error\":\"could not find the last waiting context\"");
      break;
    default:
      sb_add_chars(sb, "\"error\",\"error\":\"");
      sb_add_chars(sb, ctx->error ? ctx->error : "Unknown error");
      sb_add_chars(sb, "\"");
  }

  // create next request
  if (last_waiting) {
    in3_request_t* request = in3_create_request(last_waiting);
    if (request == NULL)
      sb_add_chars(sb, ",\"error\",\"could not create request, memory?\"");
    else {
      request->times = _malloc(sizeof(uint32_t) * request->urls_len);
      uint32_t start = now();
      char     tmp[160];
      sb_add_chars(sb, ",\"request\":{ \"type\": ");
      sb_add_chars(sb, last_waiting->type == CT_SIGN ? "\"sign\"" : "\"rpc\"");
      sb_add_chars(sb, ",\"timeout\":");
      sprintf(tmp, "%d", (unsigned int) request->timeout);
      sb_add_chars(sb, tmp);
      sb_add_chars(sb, ",\"payload\":");
      sb_add_chars(sb, request->payload);
      sb_add_chars(sb, ",\"urls\":[");
      for (int i = 0; i < request->urls_len; i++) {
        request->times[i] = start;
        if (i) sb_add_char(sb, ',');
        sb_add_char(sb, '"');
        sb_add_chars(sb, request->urls[i]);
        sb_add_char(sb, '"');
      }
      sb_add_chars(sb, "],\"ptr\":");
      sprintf(tmp, "%d,\"ctx\":%d}", (unsigned int) request, (unsigned int) last_waiting);
      sb_add_chars(sb, tmp);
    }
  }
  sb_add_char(sb, '}');

  char* r = sb->data;
  _free(sb);
  return r;
}
void EMSCRIPTEN_KEEPALIVE ifree(void* ptr) {
  _free(ptr);
}
void EMSCRIPTEN_KEEPALIVE ctx_done_response(in3_ctx_t* ctx, in3_request_t* r) {
  request_free(r, ctx, false);
}

void EMSCRIPTEN_KEEPALIVE ctx_set_response(in3_ctx_t* ctx, in3_request_t* r, int i, int is_error, char* msg) {
  r->times[i] = now() - r->times[i];
  if (is_error)
    sb_add_chars(&r->results[i].error, msg);
  else if (ctx->type == CT_SIGN) {
    uint8_t sig[65];
    hex_to_bytes(msg, -1, sig, 65);
    sb_add_range(&r->results[i].result, (char*) sig, 0, 65);
  } else
    sb_add_chars(&r->results[i].result, msg);
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
  in3_t* c           = in3_for_chain(chain);
  c->cache           = malloc(sizeof(in3_storage_handler_t));
  c->cache->get_item = storage_get_item;
  c->cache->set_item = storage_set_item;

  in3_cache_init(c);
  in3_set_error(NULL);
  return c;
}
/* frees the references of the client */
void EMSCRIPTEN_KEEPALIVE in3_dispose(in3_t* a) {
  in3_free(a);
  in3_set_error(NULL);
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
  char* result = d_create_json(res->result);
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
  bytes_t  pubkey_bytes = {.data = public_key + 1, .len = 64};
  ecdsa_get_public_key65(&secp256k1, prv_key, public_key);
  sha3_to(&pubkey_bytes, sdata);
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