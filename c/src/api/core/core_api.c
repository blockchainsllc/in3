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

#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/client/request_internal.h"
#include "../../core/client/version.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "../../third-party/crypto/rand.h"
#include "../../third-party/crypto/secp256k1.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static in3_ret_t in3_sha3(in3_rpc_handle_ctx_t* ctx) {
  if (!ctx->params || d_len(ctx->params) != 1) return req_set_error(ctx->req, "no data", IN3_EINVAL);
  bytes32_t hash;
  keccak(d_to_bytes(ctx->params + 1), hash);
  return in3_rpc_handle_with_bytes(ctx, bytes(hash, 32));
}
static in3_ret_t in3_sha256(in3_rpc_handle_ctx_t* ctx) {
  if (!ctx->params || d_len(ctx->params) != 1) return req_set_error(ctx->req, "no data", IN3_EINVAL);
  bytes32_t  hash;
  bytes_t    data = d_to_bytes(ctx->params + 1);
  SHA256_CTX c;
  sha256_Init(&c);
  sha256_Update(&c, data.data, data.len);
  sha256_Final(&c, hash);
  return in3_rpc_handle_with_bytes(ctx, bytes(hash, 32));
}
static in3_ret_t web3_clientVersion(in3_rpc_handle_ctx_t* ctx) {
  // for local chains, we return the client version of rpc endpoint.
  return ctx->req->client->chain.chain_id == CHAIN_ID_LOCAL
             ? IN3_EIGNORE
             : in3_rpc_handle_with_string(ctx, "\"Incubed/" IN3_VERSION "\"");
}

static in3_ret_t in3_config(in3_rpc_handle_ctx_t* ctx) {
  if (!ctx->params || d_len(ctx->params) != 1 || d_type(ctx->params + 1) != T_OBJECT) return req_set_error(ctx->req, "no valid config-object as argument", IN3_EINVAL);

  ctx->req->client->pending--; // we need to to temporarly decrees it in order to allow configuring
  str_range_t r   = d_to_json(ctx->params + 1);
  char        old = r.data[r.len];
  r.data[r.len]   = 0;
  char* ret       = in3_configure(ctx->req->client, r.data);
  r.data[r.len]   = old;
  ctx->req->client->pending++;

  if (ret) {
    req_set_error(ctx->req, ret, IN3_ECONFIG);
    free(ret);
    return IN3_ECONFIG;
  }

  return in3_rpc_handle_with_string(ctx, "true");
}

static in3_ret_t in3_getConfig(in3_rpc_handle_ctx_t* ctx) {
  char* ret = in3_get_config(ctx->req->client);
  in3_rpc_handle_with_string(ctx, ret);
  _free(ret);
  return IN3_OK;
}

static in3_ret_t in3_cacheClear(in3_rpc_handle_ctx_t* ctx) {
  TRY(in3_plugin_execute_first(ctx->req, PLGN_ACT_CACHE_CLEAR, NULL));
  return in3_rpc_handle_with_string(ctx, "true");
}

static in3_ret_t in3_createKey(in3_rpc_handle_ctx_t* ctx) {
  bytes32_t  hash;
  d_token_t* arg = d_get_at(ctx->params, 0);
  FILE*      r   = NULL;
  if (d_type(arg) == T_BYTES) {
    keccak(d_to_bytes(arg), hash);
    srand(bytes_to_int(hash, 4));
  }
  else {
#ifndef WASM
    r = fopen("/dev/urandom", "r");
    if (r) {
      for (int i = 0; i < 32; i++) hash[i] = (uint8_t) fgetc(r);
      fclose(r);
    }
    else
#endif
      srand(current_ms() % 0xFFFFFFFF);
  }

  if (!r) {
#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__)
    unsigned int number;
    for (int i = 0; i < 32; i++) {
      hash[i] = (rand_s(&number) ? rand() : (int) number) % 256;
    }
#else
    for (int i = 0; i < 32; i++) hash[i] = rand() % 256;
#endif
  }
  return in3_rpc_handle_with_bytes(ctx, bytes(hash, 32));
}

static in3_ret_t handle_intern(void* pdata, in3_plugin_act_t action, void* plugin_ctx) {
  in3_rpc_handle_ctx_t* ctx = plugin_ctx;
  UNUSED_VAR(pdata);
  UNUSED_VAR(action);
  UNUSED_VAR(ctx); // in case RPC_ONLY is used

#if !defined(RPC_ONLY) || defined(RPC_WEB3_SHA3)
  TRY_RPC("web3_sha3", in3_sha3(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_KECCAK)
  TRY_RPC("keccak", in3_sha3(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_SHA256)
  TRY_RPC("sha256", in3_sha256(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_WEB3_CLIENTVERSION)
  TRY_RPC("web3_clientVersion", web3_clientVersion(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_CONFIG)
  TRY_RPC("in3_config", in3_config(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_GETCONFIG)
  TRY_RPC("in3_getConfig", in3_getConfig(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_CACHECLEAR)
  TRY_RPC("in3_cacheClear", in3_cacheClear(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_CREATEKEY)
  TRY_RPC("in3_createKey", in3_createKey(ctx))
#endif

  return IN3_EIGNORE;
}

in3_ret_t in3_register_core_api(in3_t* c) {
  return in3_plugin_register(c, PLGN_ACT_RPC_HANDLE, handle_intern, NULL, false);
}
