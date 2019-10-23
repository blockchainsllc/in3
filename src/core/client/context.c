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

#include "context.h"
#include "../util/debug.h"
#include "../util/log.h"
#include "../util/mem.h"
#include "../util/stringbuilder.h"
#include "client.h"
#include "keys.h"
#include <stdio.h>
#include <string.h>

in3_ctx_t* new_ctx(in3_t* client, char* req_data) {

  in3_ctx_t* c = _calloc(1, sizeof(in3_ctx_t));
  c->attempt   = 0;
  c->cache     = NULL;
  c->client    = client;

  if (req_data != NULL) {
    c->request_context = parse_json(req_data);
    if (!c->request_context) {
      ctx_set_error(c, "Error parsing the JSON-request!", IN3_EINVAL);
      return c;
    }

    int        i;
    d_token_t* t = NULL;

    if (d_type(c->request_context->result) == T_OBJECT) {
      // it is a single result
      c->requests    = _malloc(sizeof(d_type_t*));
      c->requests[0] = c->request_context->result;
      c->len         = 1;
    } else if (d_type(c->request_context->result) == T_ARRAY) {
      c->len      = d_len(c->request_context->result);
      c->requests = _malloc(sizeof(d_type_t*) * c->len);
      for (i = 0, t = c->request_context->result + 1; i < c->len; i++, t = d_next(t))
        c->requests[i] = t;
    } else
      ctx_set_error(c, "The Request is not a valid structure!", IN3_EINVAL);
  }

  if (c->len)
    c->requests_configs = _calloc(c->len, sizeof(in3_request_config_t));

  return c;
}

in3_ret_t ctx_parse_response(in3_ctx_t* ctx, char* response_data, int len) {
  int        i;
  d_token_t* t = NULL;

  d_track_keynames(1);
  ctx->response_context = (response_data[0] == '{' || response_data[0] == '[') ? parse_json(response_data) : parse_binary_str(response_data, len);
  d_track_keynames(0);
  if (!ctx->response_context) {
    // printf("\nresponse: %s\n", response_data);
    return ctx_set_error(ctx, "Error parsing the JSON-response!", IN3_EINVALDT);
  }

  if (d_type(ctx->response_context->result) == T_OBJECT) {
    // it is a single result
    ctx->responses    = _malloc(sizeof(d_token_t*));
    ctx->responses[0] = ctx->response_context->result;
    if (ctx->len != 1) return ctx_set_error(ctx, "The response must be a single object!", IN3_EINVALDT);
  } else if (d_type(ctx->response_context->result) == T_ARRAY) {
    if (d_len(ctx->response_context->result) != ctx->len)
      return ctx_set_error(ctx, "The responses must be a array with the same number as the requests!", IN3_EINVALDT);
    ctx->responses = _malloc(sizeof(d_type_t*) * ctx->len);
    for (i = 0, t = ctx->response_context->result + 1; i < ctx->len; i++, t = d_next(t))
      ctx->responses[i] = t;
  } else
    return ctx_set_error(ctx, "The response must be a Object or Array", IN3_EINVALDT);

  return IN3_OK;
}

void free_ctx(in3_ctx_t* ctx) {
  int i;
  if (ctx->error) _free(ctx->error);
  free_ctx_nodes(ctx->nodes);
  if (ctx->response_context) {
    //if (ctx->response_context->allocated)
    _free(ctx->response_context->c);
    free_json(ctx->response_context);
  }
  if (ctx->request_context)
    free_json(ctx->request_context);

  if (ctx->requests) _free(ctx->requests);
  if (ctx->responses) _free(ctx->responses);
  if (ctx->requests_configs) {
    for (i = 0; i < ctx->len; i++) {
      if (ctx->requests_configs[i].signaturesCount)
        _free(ctx->requests_configs[i].signatures);
    }
    _free(ctx->requests_configs);
  }
  if (ctx->cache)
    in3_cache_free(ctx->cache);

  _free(ctx);
}

static unsigned long counter = 1;

in3_ret_t ctx_create_payload(in3_ctx_t* c, sb_t* sb) {
  int        i;
  d_token_t *r, *t;
  char       temp[100];
  sb_add_char(sb, '[');

  for (i = 0; i < c->len; i++) {
    if (i > 0) sb_add_char(sb, ',');
    sb_add_char(sb, '{');
    r = c->requests[i];
    if ((t = d_get(r, K_ID)) == NULL)
      sb_add_key_value(sb, "id", temp, sprintf(temp, "%lu", counter++), false);
    else if (d_type(t) == T_INTEGER)
      sb_add_key_value(sb, "id", temp, sprintf(temp, "%i", d_int(t)), false);
    else
      sb_add_key_value(sb, "id", d_string(t), d_len(t), true);
    sb_add_char(sb, ',');
    sb_add_key_value(sb, "jsonrpc", "2.0", 3, true);
    sb_add_char(sb, ',');
    if ((t = d_get(r, K_METHOD)) == NULL)
      return ctx_set_error(c, "missing method-property in request", IN3_EINVAL);
    else
      sb_add_key_value(sb, "method", d_string(t), d_len(t), true);
    sb_add_char(sb, ',');
    if ((t = d_get(r, K_PARAMS)) == NULL)
      sb_add_key_value(sb, "params", "[]", 2, false);
    else {
      //TODO this only works with JSON!!!!
      str_range_t ps = d_to_json(t);
      sb_add_key_value(sb, "params", ps.data, ps.len, false);
    }

    in3_request_config_t* rc = c->requests_configs + i;
    if (rc->verification != VERIFICATION_NEVER) {
      sb_add_char(sb, ',');

      // add in3
      //TODO This only works for chainIds < uint_32t, but ZEPHYR has some issues with PRIu64
      sb_add_range(sb, temp, 0, sprintf(temp, "\"in3\":{\"version\": \"%s\",\"chainId\":\"0x%x\"", IN3_PROTO_VER, (unsigned int) rc->chainId));
      if (rc->clientSignature)
        sb_add_bytes(sb, ",\"clientSignature\":", rc->clientSignature, 1, false);
      if (rc->finality)
        sb_add_range(sb, temp, 0, sprintf(temp, ",\"finality\":%i", rc->finality));
      if (rc->includeCode)
        sb_add_chars(sb, ",\"includeCode\":true");
      if (rc->latestBlock)
        sb_add_range(sb, temp, 0, sprintf(temp, ",\"latestBlock\":%i", rc->latestBlock));
      if (rc->signaturesCount)
        sb_add_bytes(sb, ",\"signatures\":", rc->signatures, rc->signaturesCount, true);
      if (rc->includeCode && strcmp(d_get_stringk(r, K_METHOD), "eth_call") == 0)
        sb_add_chars(sb, ",\"includeCode\":true");
      if (rc->useFullProof)
        sb_add_chars(sb, ",\"useFullProof\":true");
      if (rc->useBinary)
        sb_add_chars(sb, ",\"useBinary\":true");
      if (rc->verification == VERIFICATION_PROOF)
        sb_add_chars(sb, ",\"verification\":\"proof\"");
      else if (rc->verification == VERIFICATION_PROOF_WITH_SIGNATURE)
        sb_add_chars(sb, ",\"verification\":\"proofWithSignature\"");
      if (rc->verifiedHashesCount)
        sb_add_bytes(sb, ",\"verifiedHashes\":", rc->verifiedHashes, rc->verifiedHashesCount, true);
      sb_add_range(sb, "}}", 0, 2);
    } else
      sb_add_char(sb, '}');
  }
  sb_add_char(sb, ']');
  return IN3_OK;
}

in3_ret_t ctx_set_error(in3_ctx_t* c, char* msg, in3_ret_t errnumber) {
  int   l   = strlen(msg);
  char* dst = NULL;
  if (c->error) {
    dst = _malloc(l + 2 + strlen(c->error));
    strcpy(dst, msg);
    dst[l] = '\n';
    strcpy(dst + l + 1, c->error);
    _free(c->error);
  } else {
    dst = _malloc(l + 1);
    strcpy(dst, msg);
  }
  c->error = dst;
  in3_log_error("%s\n", msg);
  return errnumber;
}

in3_ret_t ctx_get_error(in3_ctx_t* ctx, int id) {
  if (ctx->error)
    return IN3_ERPC;
  else if (id > ctx->len)
    return IN3_EINVAL;
  else if (!ctx->responses || !ctx->responses[id])
    return IN3_ERPCNRES;
  else if (NULL == d_get(ctx->responses[id], K_RESULT) || d_get(ctx->responses[id], K_ERROR))
    return IN3_EINVALDT;
  return IN3_OK;
}

int ctx_nodes_len(node_weight_t* c) {
  int all = 0;
  while (c) {
    all++;
    c = c->next;
  }
  return all;
}

void free_ctx_nodes(node_weight_t* c) {
  node_weight_t* p = NULL;
  while (c) {
    p = c;
    c = c->next;
    _free(p);
  }
}
