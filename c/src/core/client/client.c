/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
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

#include "../util/data.h"
#include "../util/mem.h"
#include "keys.h"
#include "plugin.h"
#include "request.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

in3_req_t* in3_client_rpc_ctx_raw(in3_t* c, const char* req) {
  assert_in3(c);
  assert(req);
  // create a new context by parsing the request
  in3_req_t* ctx = req_new(c, req);

  // this happens if the request is not parseable (JSON-error in params)
  if (ctx->error) {
    ctx->verification_state = IN3_EINVAL;
    return ctx;
  }

  // execute it
  in3_ret_t ret = in3_send_req(ctx);
  if (ret == IN3_OK) {
    // the request was succesfull, so we delete interim errors (which can happen in case in3 had to retry)
    if (ctx->error) _free(ctx->error);
    ctx->error = NULL;
  }
  else
    ctx->verification_state = ret;

  return ctx; // return context and hope the calle will clean it.
}

in3_req_t* in3_client_rpc_ctx(in3_t* c, const char* method, const char* params) {
  assert_in3(c);
  assert(method);
  assert(params);

  // generate the rpc-request
  const int  max  = strlen(method) + strlen(params) + 200;                                     // determine the max length of the request string
  const bool heap = max > 500;                                                                 // if we need more than 500 bytes, we better put it in the heap
  char*      req  = heap ? _malloc(max) : alloca(max);                                         // allocate memory in heap or stack
  snprintX(req, max, "{\"method\":\"%s\",\"jsonrpc\":\"2.0\",\"params\":%s}", method, params); // create request

  in3_req_t* ctx = in3_client_rpc_ctx_raw(c, req);

  if (heap) _free(req); // free request string if we created it in heap
  return ctx;           // return context and hope the calle will clean it.
}

static in3_ret_t ctx_rpc(in3_req_t* ctx, char** result, char** error) {
  assert(ctx);
  assert_in3(ctx->client);
  if (result) result[0] = 0;
  *error = NULL;

  in3_ret_t res = ctx ? ctx->verification_state : IN3_ENOMEM;
  if (!ctx) return res;

  // check parse-errors
  if (ctx->error) {
    // we have an error
    *error = _malloc(strlen(ctx->error) + 1);
    strcpy(*error, ctx->error);
    if (res == IN3_OK) res = IN3_EUNKNOWN;
    goto clean;
  }

  // there was an error, but no message, so we create one
  if (res != IN3_OK) {
    *error = _strdupn(in3_errmsg(res), -1);
    goto clean;
  }

  // do we have an error-property in the response?
  bool       is_obj = d_type(ctx->responses[0]) == T_OBJECT;
  d_token_t* r      = is_obj ? d_get(ctx->responses[0], K_ERROR) : NULL;
  if (d_type(r) != T_NULL) {
    if (d_type(r) == T_STRING)
      *error = _strdupn(d_string(r), -1);
    else if (d_type(r) == T_OBJECT) {
      char* msg = d_get_string(r, K_MESSAGE);
      *error    = msg ? _strdupn(msg, -1) : d_create_json(ctx->response_context, r);
    }
    else
      *error = d_create_json(ctx->response_context, r);
    res = IN3_ERPC;
    goto clean;
  }

  if ((r = (is_obj ? d_get(ctx->responses[0], K_RESULT) : NULL)) == NULL) {
    if (strcmp(d_get_string(ctx->requests[0], K_METHOD), "in3_http") == 0) {
      *result = d_type(ctx->responses[0]) == T_BYTES
                    ? _strdupn((void*) ctx->responses[0]->data, ctx->responses[0]->len + 1)
                    : d_create_json(ctx->response_context, r);
      res     = IN3_OK;
    }
    else {
      // we have no result
      *error = _strdupn("no result or error in rpc-response", -1);
      res    = IN3_ERPC;
    }
    goto clean;
  }

  // we have a result and copy it
  if (result) *result = d_create_json(ctx->response_context, r);

clean:
  req_free(ctx);

  // if we have an error, we always return IN3_EUNKNOWN
  return res;
}

in3_ret_t in3_client_rpc(in3_t* c, const char* method, const char* params, char** result, char** error) {
  if (!error) return IN3_EINVAL;
  return ctx_rpc(in3_client_rpc_ctx(c, method, params), result, error);
}

in3_ret_t in3_client_rpc_raw(in3_t* c, const char* request, char** result, char** error) {
  if (!error) return IN3_EINVAL;
  return ctx_rpc(in3_client_rpc_ctx_raw(c, request), result, error);
}

static char* create_rpc_error(in3_req_t* ctx, int code, char* error) {
  sb_t          sb       = {0};
  bool          is_array = ctx && ctx->request_context && d_type(ctx->request_context->result) == T_ARRAY;
  uint_fast16_t len      = (ctx && ctx->len) ? ctx->len : 1;
  if (is_array) sb_add_char(&sb, '[');
  for (uint_fast16_t i = 0; i < len; i++) {
    if (i) sb_add_char(&sb, ',');
    sb_add_chars(&sb, "{\"id\":");
    sb_add_int(&sb, (ctx && ctx->requests && i < ctx->len) ? d_get_int(ctx->requests[i], K_ID) : 0);
    sb_add_chars(&sb, ",\"jsonrpc\":\"2.0\",\"error\":{\"code\":");
    sb_add_int(&sb, code);
    sb_add_chars(&sb, ",\"message\":\"");
    sb_add_escaped_chars(&sb, error);
    sb_add_chars(&sb, "\"}}");
  }
  if (is_array) sb_add_char(&sb, ']');
  return sb.data;
}

char* req_get_error_rpc(in3_req_t* ctx, in3_ret_t ret) {
  return create_rpc_error(ctx, ret ? ret : ctx->verification_state, ctx->error);
}

char* in3_client_exec_req(
    in3_t* c,  /**< [in] the pointer to the incubed client config. */
    char*  req /**< [in] the request as rpc. */
) {
  // parse it
  char*      res = NULL;
  in3_req_t* ctx = req_new(c, req);

  //  not enough memory
  if (!ctx) return NULL;

  // make sure result & error are clean
  // check parse-errors
  if (ctx->error) {
    res = create_rpc_error(ctx, -32700, ctx->error);
    goto clean;
  }

  // execute the request
  in3_ret_t ret = in3_send_req(ctx);

  // do we have an error?
  if (ctx->error) {
    res = req_get_error_rpc(ctx, ret);
    goto clean;
  }

  // no error message, but an error-code?
  if (ret != IN3_OK) {
    res = create_rpc_error(ctx, ret, in3_errmsg(ret));
    goto clean;
  }

  // looks good, so we use the resonse and return it
  res = req_get_response_data(ctx);

clean:

  req_free(ctx);
  return res;
}

/**
 * helper function to retrieve the message from a in3_sign_ctx_t
 */
bytes_t in3_sign_ctx_get_message(
    in3_sign_ctx_t* ctx /**< the signer context */
) {
  return ctx->message;
}

/**
 * helper function to retrieve the account from a in3_sign_ctx_t
 */
bytes_t in3_sign_ctx_get_account(
    in3_sign_ctx_t* ctx /**< the signer context */
) {
  return ctx->account;
}

/**
 * helper function to retrieve the signature from a in3_sign_ctx_t
 */
void in3_sign_ctx_set_signature_hex(
    in3_sign_ctx_t* ct, /**< the signer context */
    const char*     sig) {

  int l = (strlen(sig) + 1) / 2;
  if (l && sig[0] == '0' && sig[1] == 'x') l--;
  ct->signature = bytes(_malloc(l), l);
  hex_to_bytes(sig, -1, ct->signature.data, l);
}

/**
 * getter to retrieve the payload from a in3_http_request_t struct
 */
char* in3_get_request_payload(
    in3_http_request_t* request /**< request struct */
) {
  return request->payload;
}

/**
 * getter to retrieve the length of the payload from a in3_http_request_t struct
 */
uint32_t in3_get_request_payload_len(
    in3_http_request_t* request /**< request struct */
) {
  return request->payload_len;
}

/**
 * getter to retrieve the http-method from a in3_http_request_t struct
 */
char* in3_get_request_method(
    in3_http_request_t* request /**< request struct */
) {
  return request->method;
}
/**
 * getter to retrieve the urls list from a in3_http_request_t struct
 */
char** in3_get_request_urls(
    in3_http_request_t* request /**< request struct */
) {
  return request->urls;
}

/**
 * getter to retrieve the urls list length from a in3_http_request_t struct
 */
int in3_get_request_urls_len(
    in3_http_request_t* request /**< request struct */
) {
  return request->urls_len;
}

/**
 * getter to retrieve the urls list length from a in3_http_request_t struct
 */
int in3_get_request_headers_len(
    in3_http_request_t* request /**< request struct */
) {
  int n = 0;
  for (in3_req_header_t* h = request->headers; h; h = h->next) n++;
  return n;
}
/**
 * getter to retrieve the urls list length from a in3_http_request_t struct
 */
char* in3_get_request_headers_at(
    in3_http_request_t* request, /**< request struct */
    int                 index    /**< the inde xof the header */
) {
  int n = 0;
  for (in3_req_header_t* h = request->headers; h; h = h->next, n++) {
    if (n == index) return h->value;
  }
  return NULL;
}

/**
 * getter to retrieve the urls list length from a in3_http_request_t struct
 */
uint32_t in3_get_request_timeout(
    in3_http_request_t* request /**< request struct */
) {
  return request->req->client->timeout;
}

/** 
 * storage handler to handle cache.
 **/
typedef struct in3_storage_handler {
  in3_storage_get_item get_item; /**< function pointer returning a stored value for the given key.*/
  in3_storage_set_item set_item; /**< function pointer setting a stored value for the given key.*/
  in3_storage_clear    clear;    /**< function pointer clearing all contents of cache.*/
  void*                cptr;     /**< custom pointer which will be passed to functions */
} in3_storage_handler_t;

static in3_ret_t handle_cache(void* data, in3_plugin_act_t action, void* arg) {
  in3_cache_ctx_t*       ctx     = arg;
  in3_storage_handler_t* handler = data;
  switch (action) {
    case PLGN_ACT_CACHE_GET:
      return (ctx->content = handler->get_item(handler->cptr, ctx->key)) ? IN3_OK : IN3_EIGNORE;
    case PLGN_ACT_CACHE_SET: {
      handler->set_item(handler->cptr, ctx->key, ctx->content);
      return IN3_OK;
    }
    case PLGN_ACT_CACHE_CLEAR: {
      if (handler->clear) handler->clear(handler->cptr);
      return IN3_OK;
    }
    case PLGN_ACT_TERM: {
      _free(data);
      return IN3_OK;
    }
    default: return IN3_EINVAL;
  }
}
void in3_set_storage_handler(
    in3_t*               c,        /**< the incubed client */
    in3_storage_get_item get_item, /**< function pointer returning a stored value for the given key.*/
    in3_storage_set_item set_item, /**< function pointer setting a stored value for the given key.*/
    in3_storage_clear    clear,    /**< function pointer setting a stored value for the given key.*/
    void*                cptr      /**< custom pointer which will will be passed to functions */
) {

  in3_storage_handler_t* handler = _calloc(1, sizeof(in3_storage_handler_t));
  handler->cptr                  = cptr;
  handler->get_item              = get_item;
  handler->set_item              = set_item;
  handler->clear                 = clear;
  in3_plugin_register(c, PLGN_ACT_CACHE | PLGN_ACT_TERM, handle_cache, handler, true);
}
