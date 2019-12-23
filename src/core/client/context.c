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

in3_ctx_t* ctx_new(in3_t* client, char* req_data) {

  in3_ctx_t* c = _calloc(1, sizeof(in3_ctx_t));
  if (!c) return NULL;
  c->attempt            = 0;
  c->cache              = NULL;
  c->client             = client;
  c->verification_state = IN3_WAITING;

  if (req_data != NULL) {
    c->request_context = parse_json(req_data);
    if (!c->request_context) {
      ctx_set_error(c, "Error parsing the JSON-request!", IN3_EINVAL);
      return c;
    }

    if (d_type(c->request_context->result) == T_OBJECT) {
      // it is a single result
      c->requests    = _malloc(sizeof(d_type_t*));
      c->requests[0] = c->request_context->result;
      c->len         = 1;
    } else if (d_type(c->request_context->result) == T_ARRAY) {
      // we have an array, so we need to store the request-data as array
      d_token_t* t = c->request_context->result + 1;
      c->len       = d_len(c->request_context->result);
      c->requests  = _malloc(sizeof(d_type_t*) * c->len);
      for (int i = 0; i < c->len; i++, t = d_next(t))
        c->requests[i] = t;
    } else
      ctx_set_error(c, "The Request is not a valid structure!", IN3_EINVAL);
  }

  if (c->len)
    c->requests_configs = _calloc(c->len, sizeof(in3_request_config_t));

  return c;
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

in3_ret_t ctx_set_error(in3_ctx_t* c, char* msg, in3_ret_t errnumber) {
  // if this is just waiting, it is not an error!
  if (errnumber == IN3_WAITING) return errnumber;
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
