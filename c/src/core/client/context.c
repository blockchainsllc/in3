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

#include "context.h"
#include "../util/debug.h"
#include "../util/log.h"
#include "../util/mem.h"
#include "../util/stringbuilder.h"
#include "client.h"
#include "context_internal.h"
#include "keys.h"
#include "plugin.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

in3_ctx_t* ctx_new(in3_t* client, const char* req_data) {
  assert_in3(client);
  assert(req_data);

  if (client->pending == 0xFFFF) return NULL; // avoid overflows by not creating any new ctx anymore
  in3_ctx_t* ctx = _calloc(1, sizeof(in3_ctx_t));
  if (!ctx) return NULL;
  ctx->client             = client;
  ctx->verification_state = IN3_WAITING;
  client->pending++;

  if (req_data != NULL) {
    printf("parse %s \n",req_data);
    ctx->request_context = parse_json(req_data);
    if (!ctx->request_context) {
      ctx_set_error(ctx, "Error parsing the JSON-request!", IN3_EINVAL);
      return ctx;
    }

    if (d_type(ctx->request_context->result) == T_OBJECT) {
      // it is a single result
      ctx->requests    = _malloc(sizeof(d_token_t*));
      ctx->requests[0] = ctx->request_context->result;
      ctx->len         = 1;
    }
    else if (d_type(ctx->request_context->result) == T_ARRAY) {
      // we have an array, so we need to store the request-data as array
      d_token_t* t  = ctx->request_context->result + 1;
      ctx->len      = d_len(ctx->request_context->result);
      ctx->requests = _malloc(sizeof(d_token_t*) * ctx->len);
      for (uint_fast16_t i = 0; i < ctx->len; i++, t = d_next(t))
        ctx->requests[i] = t;
    }
    else
      ctx_set_error(ctx, "The Request is not a valid structure!", IN3_EINVAL);
  }
  return ctx;
}

char* ctx_get_error_data(in3_ctx_t* ctx) {
  return ctx ? ctx->error : "No request context";
}

char* ctx_get_response_data(in3_ctx_t* ctx) {
  assert_in3_ctx(ctx);

  sb_t sb = {0};
  if (d_type(ctx->request_context->result) == T_ARRAY) sb_add_char(&sb, '[');
  for (uint_fast16_t i = 0; i < ctx->len; i++) {
    if (i) sb_add_char(&sb, ',');
    str_range_t rr    = d_to_json(ctx->responses[i]);
    char*       start = NULL;
    if ((ctx->client->flags & FLAGS_KEEP_IN3) == 0 && (start = d_to_json(d_get(ctx->responses[i], K_IN3)).data) && start < rr.data + rr.len) {
      while (*start != ',' && start > rr.data) start--;
      sb_add_range(&sb, rr.data, 0, start - rr.data + 1);
      sb.data[sb.len - 1] = '}';
    }
    else
      sb_add_range(&sb, rr.data, 0, rr.len);
  }
  if (d_type(ctx->request_context->result) == T_ARRAY) sb_add_char(&sb, ']');
  return sb.data;
}

ctx_type_t ctx_get_type(in3_ctx_t* ctx) {
  assert_in3_ctx(ctx);
  return ctx->type;
}

in3_ret_t ctx_check_response_error(in3_ctx_t* c, int i) {
  assert_in3_ctx(c);

  d_token_t* r = d_get(c->responses[i], K_ERROR);
  if (!r)
    return IN3_OK;
  else if (d_type(r) == T_OBJECT) {
    str_range_t s   = d_to_json(r);
    char*       req = alloca(s.len + 1);
    strncpy(req, s.data, s.len);
    req[s.len] = '\0';
    return ctx_set_error(c, req, IN3_ERPC);
  }
  else
    return ctx_set_error(c, d_string(r), IN3_ERPC);
}

in3_ret_t ctx_set_error_intern(in3_ctx_t* ctx, char* message, in3_ret_t errnumber) {
  assert(ctx);

  // if this is just waiting, it is not an error!
  if (errnumber == IN3_WAITING) return errnumber;
  if (message) {
    const int l   = strlen(message);
    char*     dst = NULL;
    if (ctx->error) {
      dst = _malloc(l + 2 + strlen(ctx->error));
      strcpy(dst, message);
      dst[l] = ':';
      strcpy(dst + l + 1, ctx->error);
      _free(ctx->error);
    }
    else {
      dst = _malloc(l + 1);
      strcpy(dst, message);
    }
    ctx->error = dst;
    in3_log_trace("Intermediate error -> %s\n", message);
  }
  else if (!ctx->error) {
    ctx->error    = _malloc(2);
    ctx->error[0] = 'E';
    ctx->error[1] = 0;
  }
  ctx->verification_state = errnumber;
  return errnumber;
}

in3_ret_t ctx_get_error(in3_ctx_t* ctx, int id) {
  if (ctx->error)
    return IN3_ERPC;
  else if (id >= (int) ctx->len)
    return IN3_EINVAL;
  else if (!ctx->responses || !ctx->responses[id])
    return IN3_ERPCNRES;
  else if (NULL == d_get(ctx->responses[id], K_RESULT) || d_get(ctx->responses[id], K_ERROR))
    return IN3_EINVALDT;
  return IN3_OK;
}

int ctx_nodes_len(node_match_t* node) {
  int all = 0;
  while (node) {
    all++;
    node = node->next;
  }
  return all;
}

in3_proof_t in3_ctx_get_proof(in3_ctx_t* ctx, int i) {
  if (ctx->requests) {
    char* verfification = d_get_stringk(d_get(ctx->requests[i], K_IN3), key("verification"));
    if (verfification && strcmp(verfification, "none") == 0) return PROOF_NONE;
    if (verfification && strcmp(verfification, "proof") == 0) return PROOF_STANDARD;
  }
  if (ctx->signers_length && !ctx->client->proof) return PROOF_STANDARD;
  return ctx->client->proof;
}

NONULL void in3_req_add_response(
    in3_request_t* req,      /**< [in]the the request */
    int            index,    /**< [in] the index of the url, since this request could go out to many urls */
    bool           is_error, /**< [in] if true this will be reported as error. the message should then be the error-message */
    const char*    data,     /**<  the data or the the string*/
    int            data_len, /**<  the length of the data or the the string (use -1 if data is a null terminated string)*/
    uint32_t       time) {
  in3_ctx_add_response(req->ctx, index, is_error, data, data_len, time);
}

void in3_ctx_add_response(
    in3_ctx_t*  ctx,      /**< [in] the context */
    int         index,    /**< [in] the index of the url, since this request could go out to many urls */
    bool        is_error, /**< [in] if true this will be reported as error. the message should then be the error-message */
    const char* data,     /**<  the data or the the string*/
    int         data_len, /**<  the length of the data or the the string (use -1 if data is a null terminated string)*/
    uint32_t    time) {

  assert_in3_ctx(ctx);
  assert(data);

  if (!ctx->raw_response) {
    ctx_set_error(ctx, "no request created yet!", IN3_EINVAL);
    return;
  }
  in3_response_t* response = ctx->raw_response + index;
  response->time += time;
  if (response->state == IN3_OK && is_error) response->data.len = 0;
  response->state = is_error ? IN3_ERPC : IN3_OK;
  if (data_len == -1)
    sb_add_chars(&response->data, data);
  else
    sb_add_range(&response->data, data, 0, data_len);
}

sb_t* in3_rpc_handle_start(in3_rpc_handle_ctx_t* hctx) {
  assert(hctx);
  assert_in3_ctx(hctx->ctx);
  assert(hctx->request);
  assert(hctx->response);

  *hctx->response = _calloc(1, sizeof(in3_response_t));
  return sb_add_chars(&(*hctx->response)->data, "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":");
}
in3_ret_t in3_rpc_handle_finish(in3_rpc_handle_ctx_t* hctx) {
  sb_add_char(&(*hctx->response)->data, '}');
  return IN3_OK;
}

in3_ret_t in3_rpc_handle_with_bytes(in3_rpc_handle_ctx_t* hctx, bytes_t data) {
  sb_add_bytes(in3_rpc_handle_start(hctx), NULL, &data, 1, false);
  return in3_rpc_handle_finish(hctx);
}

in3_ret_t in3_rpc_handle_with_string(in3_rpc_handle_ctx_t* hctx, char* data) {
  sb_add_chars(in3_rpc_handle_start(hctx), data);
  return in3_rpc_handle_finish(hctx);
}

in3_ret_t in3_rpc_handle_with_int(in3_rpc_handle_ctx_t* hctx, uint64_t value) {
  uint8_t val[8];
  long_to_bytes(value, val);
  bytes_t b = bytes(val, 8);
  b_optimize_len(&b);
  char* s = alloca(b.len * 2 + 5);
  bytes_to_hex(b.data, b.len, s + 3);
  if (s[3] == '0') s++;
  int l    = strlen(s + 3) + 3;
  s[0]     = '"';
  s[1]     = '0';
  s[2]     = 'x';
  s[l]     = '"';
  s[l + 1] = 0;
  return in3_rpc_handle_with_string(hctx, s);
}

in3_ret_t ctx_send_sub_request(in3_ctx_t* parent, char* method, char* params, char* in3, d_token_t** result) {
  bool use_cache = strcmp(method, "eth_sendTransaction") == 0;
  if (params == NULL) params = "";
  char* req = NULL;
  if (use_cache) {
    req = alloca(strlen(params) + strlen(method) + 20 + (in3 ? 10 + strlen(in3) : 0));
    if (in3)
      sprintf(req, "{\"method\":\"%s\",\"params\":[%s],\"in3\":%s}", method, params, in3);
    else
      sprintf(req, "{\"method\":\"%s\",\"params\":[%s]}", method, params);
  }

  in3_ctx_t* ctx = parent->required;
  for (; ctx; ctx = ctx->required) {
    if (use_cache) {
      // only check first entry
      bool found = false;
      for (cache_entry_t* e = ctx->cache; e && !found; e = e->next) {
        if (e->props & CACHE_PROP_SRC_REQ) {
          if (strcmp((char*) e->value.data, req) == 0) found = true;
        }
      }
      if (found) break;
    }
    if (strcmp(d_get_stringk(ctx->requests[0], K_METHOD), method)) continue;
    d_token_t* t = d_get(ctx->requests[0], K_PARAMS);
    if (!t) continue;
    str_range_t p = d_to_json(t);
    if (strncmp(params, p.data + 1, p.len - 2) == 0) break;
  }

  if (ctx)
    switch (in3_ctx_state(ctx)) {
      case CTX_ERROR:
        return ctx_set_error(parent, ctx->error, ctx->verification_state ? ctx->verification_state : IN3_ERPC);
      case CTX_SUCCESS:
        *result = d_get(ctx->responses[0], K_RESULT);
        if (!*result) {
          char* s = d_get_stringk(d_get(ctx->responses[0], K_ERROR), K_MESSAGE);
          return ctx_set_error(parent, s ? s : "error executing provider call", IN3_ERPC);
        }
        return IN3_OK;
      case CTX_WAITING_TO_SEND:
      case CTX_WAITING_FOR_RESPONSE:
        return IN3_WAITING;
    }

  // create the call
  req = req ? _strdupn(req, -1) : _malloc(strlen(params) + strlen(method) + 20 + (in3 ? 10 + strlen(in3) : 0));
  if (!use_cache) {
    if (in3)
      sprintf(req, "{\"method\":\"%s\",\"params\":[%s],\"in3\":%s}", method, params, in3);
    else
      sprintf(req, "{\"method\":\"%s\",\"params\":[%s]}", method, params);
  }
  ctx = ctx_new(parent->client, req);
  if (!ctx) return ctx_set_error(parent, "Invalid request!", IN3_ERPC);
  if (use_cache)
    in3_cache_add_ptr(&ctx->cache, req)->props = CACHE_PROP_SRC_REQ;
  return ctx_add_required(parent, ctx);
}

in3_ret_t ctx_require_signature(in3_ctx_t* ctx, char* method, uint8_t* sig, bytes_t raw_data, bytes_t from) {
  bytes_t* cached_sig = in3_cache_get_entry(ctx->cache, &raw_data);
  if (cached_sig) {
    memcpy(sig, cached_sig->data, 65);
    return IN3_OK;
  }

  // get the signature from required
  in3_ctx_t* c = ctx_find_required(ctx, method);
  if (c)
    switch (in3_ctx_state(c)) {
      case CTX_ERROR:
        return ctx_set_error(ctx, c->error, IN3_ERPC);
      case CTX_WAITING_FOR_RESPONSE:
      case CTX_WAITING_TO_SEND:
        return IN3_WAITING;
      case CTX_SUCCESS: {
        if (c->raw_response && c->raw_response->state == IN3_OK && c->raw_response->data.len == 65) {
          in3_cache_add_entry(&ctx->cache, cloned_bytes(raw_data), cloned_bytes(bytes((uint8_t*) c->raw_response->data.data, 65)));
          memcpy(sig, c->raw_response->data.data, 65);
          ctx_remove_required(ctx, c, false);
          return IN3_OK;
        }
        else if (c->raw_response && c->raw_response->state)
          return ctx_set_error(ctx, c->raw_response->data.data, c->raw_response->state);
        else
          return ctx_set_error(ctx, "no data to sign", IN3_EINVAL);

        default:
          return ctx_set_error(ctx, "invalid state", IN3_EINVAL);
      }
    }
  else {
    sb_t req = {0};
    sb_add_chars(&req, "{\"method\":\"");
    sb_add_chars(&req, method);
    sb_add_bytes(&req, "\",\"params\":[", &raw_data, 1, false);
    sb_add_chars(&req, ",");
    sb_add_bytes(&req, NULL, &from, 1, false);
    sb_add_chars(&req, "]}");
    c = ctx_new(ctx->client, req.data);
    if (!c) return IN3_ECONFIG;
    c->type = CT_SIGN;
    return ctx_add_required(ctx, c);
  }
}
