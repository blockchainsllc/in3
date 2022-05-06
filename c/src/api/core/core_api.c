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
#include "../../core/util/crypto.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static in3_ret_t in3_sha3(in3_rpc_handle_ctx_t* ctx) {
  bytes_t b;
  TRY_PARAM_GET_REQUIRED_BYTES(b, ctx, 0, 0, 0)
  bytes32_t hash;
  keccak(b, hash);
  return in3_rpc_handle_with_bytes(ctx, bytes(hash, 32));
}
static in3_ret_t in3_sha256(in3_rpc_handle_ctx_t* ctx) {
  bytes_t   b;
  bytes32_t hash;
  TRY_PARAM_GET_REQUIRED_BYTES(b, ctx, 0, 0, 0)
  in3_digest_t c = crypto_create_hash(DIGEST_SHA256);
  if (!c.ctx) return req_set_error(ctx->req, "sha256 not supported", IN3_ENOTSUP);
  crypto_update_hash(c, b);
  crypto_finalize_hash(c, hash);
  return in3_rpc_handle_with_bytes(ctx, bytes(hash, 32));
}
static in3_ret_t web3_clientVersion(in3_rpc_handle_ctx_t* ctx) {
  // for local chains, we return the client version of rpc endpoint.
  return in3_chain_id(ctx->req) == CHAIN_ID_LOCAL
             ? IN3_EIGNORE
             : in3_rpc_handle_with_string(ctx, "\"Incubed/" IN3_VERSION "\"");
}

static in3_ret_t in3_config(in3_rpc_handle_ctx_t* ctx) {
  d_token_t* cnf;
  TRY_PARAM_GET_REQUIRED_OBJECT(cnf, ctx, 0)

  ctx->req->client->pending--; // we need to to temporarly decrees it in order to allow configuring
  str_range_t r   = d_to_json(cnf);
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
  bytes32_t hash;
  random_buffer(hash, 32);
  return in3_rpc_handle_with_bytes(ctx, bytes(hash, 32));
}

static in3_ret_t in3_bip32(in3_rpc_handle_ctx_t* ctx) {
  char*   curvename = NULL;
  char*   path      = NULL;
  bytes_t seed      = {0};

  TRY_PARAM_GET_REQUIRED_BYTES(seed, ctx, 0, 16, 0)
  TRY_PARAM_GET_STRING(curvename, ctx, 1, "secp256k1")
  TRY_PARAM_GET_STRING(path, ctx, 2, NULL)

  bytes32_t pk;
  in3_ret_t r = bip32(seed, ECDSA_SECP256K1, path, pk);

  return r
             ? req_set_error(ctx->req, "BIP32 not supported! Please reconfigure cmake!", r)
             : in3_rpc_handle_with_bytes(ctx, bytes(pk, 32));
}

static in3_ret_t in3_bip39_create(in3_rpc_handle_ctx_t* ctx) {
  bytes32_t hash;
  bytes_t   pk      = {0};
  int       words   = 24;
  int       seedlen = 256;
  TRY_PARAM_GET_BYTES(pk, ctx, 0, 0, 0)
  TRY_PARAM_GET_INT(words, ctx, 1, 24)

  switch (words) {
    case 24: seedlen = 256; break;
    case 21: seedlen = 224; break;
    case 18: seedlen = 192; break;
    case 15: seedlen = 160; break;
    case 12: seedlen = 128; break;
    default: return rpc_throw(ctx->req, "The wordlen (%i) must be either 12,15,18,21 or 24", words);
  }

  if (!pk.data) {
    random_buffer(hash, seedlen / 8);
    pk = bytes(hash, seedlen / 8);
  }

  char* r = mnemonic_create(pk);
  in3_rpc_handle(ctx, "\"%s\"", r);
  memzero(hash, 32);
  memzero(r, strlen(r));
  _free(r);
  return IN3_OK;
}

static in3_ret_t in3_bip39_decode(in3_rpc_handle_ctx_t* ctx) {
  char*   mnemonic   = NULL;
  char*   passphrase = NULL;
  uint8_t seed[64];

  TRY_PARAM_GET_REQUIRED_STRING(mnemonic, ctx, 0)
  TRY_PARAM_GET_STRING(passphrase, ctx, 1, "")

  if (mnemonic_verify(mnemonic)) return rpc_throw(ctx->req, "Invalid mnemonic! %s", mnemonic);

  mnemonic_to_seed(mnemonic, passphrase, seed, NULL);
  return in3_rpc_handle_with_bytes(ctx, bytes(seed, 64));
}

static in3_ret_t in3_encode(in3_rpc_handle_ctx_t* ctx, in3_encoding_type_t type) {
  bytes_t data;
  TRY_PARAM_GET_REQUIRED_BYTES(data, ctx, 0, 0, 0)
  char* c = _malloc(encode_size(type, data.len));
  int   l = encode(type, data, c);
  if (l >= 0) sb_printx(in3_rpc_handle_start(ctx), "\"%S\"", c);
  _free(c);
  return l < 0
             ? req_set_error(ctx->req, "Encoding not supported", IN3_ENOTSUP)
             : in3_rpc_handle_finish(ctx);
}

static in3_ret_t in3_crypto_convert(in3_rpc_handle_ctx_t* ctx) {
  bytes_t data;
  char*   type;
  TRY_PARAM_GET_REQUIRED_BYTES(data, ctx, 0, 0, 0)
  TRY_PARAM_GET_REQUIRED_STRING(type, ctx, 1)

  in3_curve_type_t   curve = ECDSA_SECP256K1;
  in3_convert_type_t ct    = CONV_PK32_TO_PUB64;
  if (strncmp(type, "ecdsa_", 6) == 0) {
    curve = ECDSA_SECP256K1;
    type += 6;
  }
  else if (strncmp(type, "eddsa_", 6) == 0) {
    curve = EDDSA_ED25519;
    type += 6;
  }

  if (strcmp(type, "pub64") == 0)
    ct = CONV_PK32_TO_PUB64;
  else if (strcmp(type, "pub32") == 0)
    ct = CONV_PK32_TO_PUB32;
  else if (strcmp(type, "sig_der") == 0)
    ct = CONV_SIG65_TO_DER;
  else if (strcmp(type, "pub_der") == 0)
    ct = CONV_PUB64_TO_DER;
  else
    return rpc_throw(ctx->req, "Unknown convert type (%s)", type);

  uint8_t buf[100]; // 100 bytes are enough, because even converting a signature to der will not take more.
  int     buf_len;
  TRY(crypto_convert(curve, ct, data, buf, &buf_len))
  return in3_rpc_handle_with_bytes(ctx, bytes(buf, buf_len));
}

static in3_ret_t in3_decode(in3_rpc_handle_ctx_t* ctx, in3_encoding_type_t type) {
  char* txt;
  TRY_PARAM_GET_REQUIRED_STRING(txt, ctx, 0);
  int      len = strlen(txt);
  uint8_t* dst = _malloc(decode_size(type, len));
  int      l   = decode(type, txt, len, dst);
  if (l >= 0) sb_printx(in3_rpc_handle_start(ctx), "\"%B\"", bytes(dst, l));
  _free(dst);
  return l < 0
             ? req_set_error(ctx->req, "Decoding not supported", IN3_ENOTSUP)
             : in3_rpc_handle_finish(ctx);
}

static in3_ret_t in3_ed25519_pk2pub(in3_rpc_handle_ctx_t* ctx) {
  bytes_t   pk;
  bytes32_t pub;
  TRY_PARAM_GET_REQUIRED_BYTES(pk, ctx, 0, 32, 32);
  TRY(crypto_convert(EDDSA_ED25519, CONV_PK32_TO_PUB32, pk, pub, NULL))
  return in3_rpc_handle_with_bytes(ctx, bytes(pub, 32));
}

static in3_ret_t in3_ed25519_sign(in3_rpc_handle_ctx_t* ctx) {
  bytes_t   msg;
  bytes_t   pk;
  bytes32_t pub;
  uint8_t   signature[64];
  TRY_PARAM_GET_REQUIRED_BYTES(msg, ctx, 0, 0, 0);
  TRY_PARAM_GET_REQUIRED_BYTES(pk, ctx, 1, 32, 32);
  TRY(crypto_convert(EDDSA_ED25519, CONV_PK32_TO_PUB32, pk, pub, NULL))
  TRY(crypto_sign_digest(EDDSA_ED25519, msg, pk.data, pub, signature))
  return in3_rpc_handle_with_bytes(ctx, bytes(signature, 64));
}

static in3_ret_t in3_ed25519_verify(in3_rpc_handle_ctx_t* ctx) {
  bytes_t msg;
  bytes_t pub;
  bytes_t signature;
  TRY_PARAM_GET_REQUIRED_BYTES(msg, ctx, 0, 0, 0);
  TRY_PARAM_GET_REQUIRED_BYTES(signature, ctx, 1, 64, 64);
  TRY_PARAM_GET_REQUIRED_BYTES(pub, ctx, 2, 32, 32);
  in3_ret_t r = crypto_recover(EDDSA_ED25519, msg, signature, pub.data);
  return (r == IN3_OK || r == IN3_EINVAL)
             ? in3_rpc_handle_with_string(ctx, r ? "false" : "true")
             : req_set_error(ctx->req, "ed25519 Not supported", r);
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

#if !defined(RPC_ONLY) || defined(RPC_IN3_BIP32)
  TRY_RPC("in3_bip32", in3_bip32(ctx))
#endif

#if !defined(RPC_ONLY) || defined(RPC_IN3_BIP39_CREATE)
  TRY_RPC("in3_bip39_create", in3_bip39_create(ctx))
#endif

#if !defined(RPC_ONLY) || defined(RPC_IN3_BIP39_decode)
  TRY_RPC("in3_bip39_decode", in3_bip39_decode(ctx))
#endif

#if !defined(RPC_ONLY) || defined(RPC_IN3_BASE58_ENCODE)
  TRY_RPC("in3_base58_encode", in3_encode(ctx, ENC_BASE58))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_BASE58_DECODE)
  TRY_RPC("in3_base58_decode", in3_decode(ctx, ENC_BASE58))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_BASE64_ENCODE)
  TRY_RPC("in3_base64_encode", in3_encode(ctx, ENC_BASE64))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_BASE64_DECODE)
  TRY_RPC("in3_base64_decode", in3_decode(ctx, ENC_BASE64))
#endif
  TRY_RPC("in3_ed25519_sign", in3_ed25519_sign(ctx))
  TRY_RPC("in3_ed25519_pk2pub", in3_ed25519_pk2pub(ctx))
  TRY_RPC("in3_ed25519_verify", in3_ed25519_verify(ctx))
#if !defined(RPC_ONLY) || defined(RPC_IN3_CONVERT)
  TRY_RPC("in3_crypto_convert", in3_crypto_convert(ctx))
#endif
  return IN3_EIGNORE;
}

in3_ret_t in3_register_core_api(in3_t* c) {
  return in3_plugin_register(c, PLGN_ACT_RPC_HANDLE, handle_intern, NULL, false);
}
