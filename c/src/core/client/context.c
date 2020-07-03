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
#include <stdio.h>
#include <string.h>

in3_ctx_t* ctx_new(in3_t* client, const char* req_data) {

  if (client->pending == 0xFFFF) return NULL; // avoid overflows by not creating any new ctx anymore
  in3_ctx_t* ctx = _calloc(1, sizeof(in3_ctx_t));
  if (!ctx) return NULL;
  ctx->client             = client;
  ctx->verification_state = IN3_WAITING;
  client->pending++;

  if (req_data != NULL) {
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
    } else if (d_type(ctx->request_context->result) == T_ARRAY) {
      // we have an array, so we need to store the request-data as array
      d_token_t* t  = ctx->request_context->result + 1;
      ctx->len      = d_len(ctx->request_context->result);
      ctx->requests = _malloc(sizeof(d_token_t*) * ctx->len);
      for (uint_fast16_t i = 0; i < ctx->len; i++, t = d_next(t))
        ctx->requests[i] = t;
    } else
      ctx_set_error(ctx, "The Request is not a valid structure!", IN3_EINVAL);
  }
  return ctx;
}

char* ctx_get_error_data(in3_ctx_t* ctx) {
  return ctx->error;
}

in3_ctx_t* ctx_get_next_required(in3_ctx_t* ctx) {
  return ctx->required;
}

bool ctx_is_ignore(in3_ctx_t* ctx) {
  return ctx->verification_state == IN3_EIGNORE;
}

bool ctx_is_waiting_response(in3_ctx_t* ctx) {
  return !ctx->raw_response && in3_ctx_state(ctx) == CTX_WAITING_FOR_RESPONSE;
}

char* ctx_get_response_data(in3_ctx_t* ctx) {
  str_range_t rr = d_to_json(ctx->responses[0]), rin3;
  if ((ctx->client->flags & FLAGS_KEEP_IN3) == 0 && (rin3 = d_to_json(d_get(ctx->responses[0], K_IN3))).data) {
    while (*rin3.data != ',' && rin3.data > rr.data) rin3.data--;
    *rin3.data = '}';
    rr.len     = rin3.data - rr.data + 1;
  }
  char* res   = _malloc(rr.len + 1);
  res[rr.len] = 0; // we can now manipulating the response, since we will free it anyway.
  memcpy(res, rr.data, rr.len);
  return res;
}

ctx_type_t ctx_get_type(in3_ctx_t* ctx) {
  return ctx->type;
}

in3_ret_t ctx_check_response_error(in3_ctx_t* c, int i) {
  d_token_t* r = d_get(c->responses[i], K_ERROR);
  if (!r)
    return IN3_OK;
  else if (d_type(r) == T_OBJECT) {
    str_range_t s   = d_to_json(r);
    char*       req = alloca(s.len + 1);
    strncpy(req, s.data, s.len);
    req[s.len] = '\0';
    return ctx_set_error(c, req, IN3_ERPC);
  } else
    return ctx_set_error(c, d_string(r), IN3_ERPC);
}

in3_ret_t ctx_set_error_intern(in3_ctx_t* ctx, char* message, in3_ret_t errnumber) {
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
    } else {
      dst = _malloc(l + 1);
      strcpy(dst, message);
    }
    ctx->error = dst;
    in3_log_trace("Intermediate error -> %s\n", message);
  } else if (!ctx->error) {
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

in3_proof_t in3_ctx_get_proof(in3_ctx_t* ctx) {
  if (ctx->requests) {
    char* verfification = d_get_stringk(d_get(ctx->requests[0], K_IN3), key("verification"));
    if (verfification && strcmp(verfification, "none") == 0) return PROOF_NONE;
    if (verfification && strcmp(verfification, "proof") == 0) return PROOF_STANDARD;
  }
  if (ctx->signers_length && !ctx->client->proof) return PROOF_STANDARD;
  return ctx->client->proof;
}