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

#include "signer.h"
#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/crypto.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "../../verifier/eth1/nano/serialize.h"
#include <string.h>

typedef struct signer_key {
  bytes32_t      pk;
  uint8_t        account[64];
  unsigned int   account_len;
  d_curve_type_t type;
} signer_key_t;

static int get_address(uint8_t* pk, uint8_t* address, d_curve_type_t type) {
  in3_ret_t r;
  switch (type) {
    case SIGN_CURVE_ECDSA: {
      uint8_t public_key[64];
      r = crypto_convert(ECDSA_SECP256K1, CONV_PK32_TO_PUB64, bytes(pk, 32), public_key, NULL);
      keccak(bytes(public_key, 64), public_key);
      memcpy(address, public_key + 12, 20);
      return r == IN3_OK ? 20 : r;
    }
    case SIGN_CURVE_ED25519:
      r = crypto_convert(EDDSA_ED25519, CONV_PK32_TO_PUB32, bytes(pk, 32), address, NULL);
      return r == IN3_OK ? 32 : r;
    default:
      return IN3_ENOTSUP;
  }
}

static bool add_key(in3_t* c, bytes32_t pk, d_curve_type_t type) {
  uint8_t                pub[64];
  int                    l   = get_address(pk, pub, type);
  in3_sign_account_ctx_t ctx = {0};
  in3_req_t              r   = {0};
  ctx.req                    = &r;
  r.client                   = c;

  for (in3_plugin_t* p = c->plugins; p; p = p->next) {
    if ((p->acts & PLGN_ACT_SIGN_ACCOUNT) && (p->acts & PLGN_ACT_SIGN) && p->action_fn(p->data, PLGN_ACT_SIGN_ACCOUNT, &ctx) == IN3_OK && ctx.accounts_len) {
      bool is_same_address = type == ctx.curve_type && memcmp(ctx.accounts, pub, l) == 0;
      _free(ctx.accounts);
      if (is_same_address) return false;
    }
  }

  eth_set_pk_signer(c, pk, type, NULL);
  return true;
}
/** Signs message after hashing it with hasher function given in 'hasher_t', with the given private key*/
in3_ret_t ec_sign_pk_hash(uint8_t* message, size_t len, uint8_t* pk, d_digest_type_t hasher, uint8_t* dst) {
  bytes_t res = sign_with_pk(pk, bytes(message, len), hasher);
  if (res.data) {
    memcpy(dst, res.data, res.len);
    _free(res.data);
    return IN3_OK;
  }
  return IN3_EINVAL;
}

void eth_create_prefixed_msg_hash(bytes32_t dst, bytes_t msg) {
  in3_digest_t d      = crypto_create_hash(DIGEST_KECCAK);
  const char*  PREFIX = "\x19"
                        "Ethereum Signed Message:\n";
  crypto_update_hash(d, bytes((uint8_t*) PREFIX, strlen(PREFIX)));
  crypto_update_hash(d, bytes(dst, sprintf((char*) dst, "%d", (int) msg.len)));
  if (msg.len) crypto_update_hash(d, msg);
  crypto_finalize_hash(d, dst);
}

bytes_t sign_with_pk(const bytes32_t pk, const bytes_t data, const d_digest_type_t type) {
  bytes_t res = bytes(_malloc(65), 65);
  switch (type) {
    case SIGN_EC_RAW:
      if (crypto_sign_digest(ECDSA_SECP256K1, data, pk, NULL, res.data)) {
        _free(res.data);
        res = NULL_BYTES;
      }
      break;

    case SIGN_EC_PREFIX: {
      bytes32_t hash;
      eth_create_prefixed_msg_hash(hash, data);
      if (crypto_sign_digest(ECDSA_SECP256K1, bytes(hash, 32), pk, NULL, res.data)) {
        _free(res.data);
        res = NULL_BYTES;
      }
      break;
    }
    case SIGN_EC_HASH: {
      bytes32_t hash;
      keccak(data, hash);
      if (crypto_sign_digest(ECDSA_SECP256K1, bytes(hash, 32), pk, NULL, res.data)) {
        _free(res.data);
        res = NULL_BYTES;
      }
      break;
    }
    case SIGN_EC_BTC: {
      bytes32_t    hash;
      in3_digest_t d = crypto_create_hash(DIGEST_SHA256_BTC);
      crypto_update_hash(d, data);
      crypto_finalize_hash(d, hash);
      if (crypto_sign_digest(ECDSA_SECP256K1, bytes(hash, 32), pk, NULL, res.data)) {
        _free(res.data);
        res = NULL_BYTES;
      }
      break;
    }
    default:
      _free(res.data);
      res = NULL_BYTES;
  }
  return res;
}

static in3_ret_t eth_sign_pk(void* data, in3_plugin_act_t action, void* action_ctx) {
  signer_key_t* k = data;
  switch (action) {
    case PLGN_ACT_SIGN: {
      in3_sign_ctx_t* ctx = action_ctx;
      if (ctx->account.len && (ctx->account.len != k->account_len || memcmp(k->account, ctx->account.data, k->account_len))) return IN3_EIGNORE;
      switch (ctx->curve_type) {
        case SIGN_CURVE_ECDSA:
          ctx->signature = sign_with_pk(k->pk, ctx->message, ctx->digest_type);
          break;
        case SIGN_CURVE_ED25519: {
          uint8_t sig[64];
          TRY(crypto_sign_digest(EDDSA_ED25519, ctx->message, k->pk, k->account, sig))
          ctx->signature = bytes_dup(bytes(sig, 64));
        }
      }
      return ctx->signature.data ? IN3_OK : IN3_ENOTSUP;
    }

    case PLGN_ACT_SIGN_ACCOUNT: {
      // generate the address from the key
      in3_sign_account_ctx_t* ctx = action_ctx;
      ctx->signer_type            = SIGNER_ECDSA;
      ctx->curve_type             = k->type;
      ctx->accounts               = _malloc(20);
      ctx->accounts_len           = 1;
      memcpy(ctx->accounts, k->account, 20);
      return IN3_OK;
    }

    case PLGN_ACT_SIGN_PUBLICKEY: {
      // generate the address from the key
      in3_sign_public_key_ctx_t* ctx = action_ctx;
      if (k->type != ctx->curve_type || (ctx->account && memcmp(ctx->account, k->account, k->account_len))) return IN3_EIGNORE;
      switch (ctx->curve_type) {
        case SIGN_CURVE_ECDSA:
          return crypto_convert(ECDSA_SECP256K1, ctx->convert_type, bytes(k->pk, 32), ctx->public_key, NULL);
        case SIGN_CURVE_ED25519:
          return crypto_convert(EDDSA_ED25519, CONV_PK32_TO_PUB32, bytes(k->pk, 32), ctx->public_key, NULL);
        default:
          return IN3_ENOTSUP;
      }
    }

    case PLGN_ACT_TERM: {
      _free(k);
      return IN3_OK;
    }

    default:
      return IN3_ENOTSUP;
  }
}

/** sets the signer and a pk to the client*/
in3_ret_t eth_set_pk_signer(in3_t* in3, bytes32_t pk, d_curve_type_t type, uint8_t** address) {
  if (!pk) return IN3_EINVAL;
  signer_key_t* k = _malloc(sizeof(signer_key_t));
  k->type         = type;
  k->account_len  = get_address(pk, k->account, type);
  memcpy(k->pk, pk, 32);
  if (address) *address = k->account;
  return in3_plugin_register(in3, PLGN_ACT_SIGN_ACCOUNT | PLGN_ACT_SIGN | PLGN_ACT_TERM | PLGN_ACT_SIGN_PUBLICKEY, eth_sign_pk, k, false);
}

static in3_ret_t add_raw_key(in3_rpc_handle_ctx_t* ctx) {
  bytes_t b;
  char*   curve = NULL;
  TRY_PARAM_GET_REQUIRED_BYTES(b, ctx, 0, 32, 32);
  TRY_PARAM_GET_STRING(curve, ctx, 1, "secp256k1");
  d_curve_type_t ct = SIGN_CURVE_ECDSA;
  if (curve && strcmp(curve, "ed25519") == 0) ct = SIGN_CURVE_ED25519;

  uint8_t adr[64];
  int     l = get_address(b.data, adr, ct);
  add_key(ctx->req->client, b.data, ct);
  return in3_rpc_handle_with_bytes(ctx, bytes(adr, l));
}

static in3_ret_t in3_addJsonKey(in3_rpc_handle_ctx_t* ctx) {
  char*      passphrase = NULL;
  d_token_t* res        = NULL;
  d_token_t* data       = NULL;
  TRY_PARAM_GET_REQUIRED_OBJECT(data, ctx, 0)
  TRY_PARAM_GET_REQUIRED_STRING(passphrase, ctx, 1)
  char* params = sprintx("%j,\"%S\"", data, passphrase);
  TRY_FINAL(req_send_sub_request(ctx->req, "in3_decryptKey", params, NULL, &res, NULL), _free(params))
  bytes_t pk = d_bytes(res);
  if (!pk.data && pk.len != 32) return req_set_error(ctx->req, "invalid key", IN3_EINVAL);
  address_t adr;
  int       l = get_address(pk.data, adr, SIGN_CURVE_ECDSA);
  add_key(ctx->req->client, pk.data, SIGN_CURVE_ECDSA);
  return in3_rpc_handle_with_bytes(ctx, bytes(adr, l));
}

static in3_ret_t in3_addMnemonic(in3_rpc_handle_ctx_t* ctx) {

  char*      curvename  = NULL;
  char*      mnemonic   = NULL;
  char*      passphrase = NULL;
  d_token_t* paths      = NULL;
  uint8_t    seed[64];

  TRY_PARAM_GET_REQUIRED_STRING(mnemonic, ctx, 0)
  TRY_PARAM_GET_STRING(passphrase, ctx, 1, "")
  TRY_PARAM_GET_ARRAY(paths, ctx, 2);
  TRY_PARAM_GET_STRING(curvename, ctx, 3, "secp256k1")
  d_curve_type_t ct = SIGN_CURVE_ECDSA;
  if (curvename && strcmp(curvename, "ed25519") == 0) ct = SIGN_CURVE_ED25519;

  if (mnemonic_verify(mnemonic)) return req_set_error(ctx->req, "Invalid mnemonic!", IN3_ERPC);

  mnemonic_to_seed(mnemonic, passphrase, seed, NULL);
  sb_t path = {0};

  if (d_type(paths) == T_ARRAY) {
    for_children_of(iter, paths) {
      if (path.len) sb_add_char(&path, ' ');
      sb_add_chars(&path, d_string(iter.token));
    }
  }
  else if (d_type(paths) == T_STRING)
    sb_add_chars(&path, d_string(paths));
  else
    sb_add_chars(&path, "m/44'/60'/0'/0/0");

  int l = 1;
  for (int i = 0; i < (int) path.len; i++) {
    if (path.data[i] == ' ' || path.data[i] == ',') l++;
  }

  uint8_t*  pks = _malloc(l * 32);
  in3_ret_t r   = bip32(bytes(seed, 64), ECDSA_SECP256K1, path.data, pks);
  _free(path.data);
  memzero(seed, 64);
  if (r == IN3_OK) {
    sb_t* sb = in3_rpc_handle_start(ctx);
    sb_add_char(sb, '[');
    for (int i = 0; i < l; i++) {
      if (add_key(ctx->req->client, pks + i * 32, ct)) {
        if (sb->data[sb->len - 1] != '[') sb_add_char(sb, ',');
        uint8_t adr[64];
        int     l = get_address(pks + i * 32, adr, ct);
        sb_printx(sb, "\"%B\"", bytes(adr, l));
      }
    }
    sb_add_char(sb, ']');
    memzero(pks, l * 32);
    _free(pks);
    return in3_rpc_handle_finish(ctx);
  }
  else {
    _free(pks);
    return req_set_error(ctx->req, "Invalid seed or bip39 not supported!", r);
  }
}

static in3_ret_t signer_ids(in3_rpc_handle_ctx_t* ctx) {
  sb_t*                  sb    = in3_rpc_handle_start(ctx);
  bool                   first = true;
  in3_sign_account_ctx_t sc    = {.req = ctx->req, .accounts = NULL, .accounts_len = 0, .signer_type = 0, .curve_type = SIGN_CURVE_ECDSA};
  for (in3_plugin_t* p = ctx->req->client->plugins; p; p = p->next) {
    if (p->acts & PLGN_ACT_SIGN_ACCOUNT && p->action_fn(p->data, PLGN_ACT_SIGN_ACCOUNT, &sc) == IN3_OK) {
      for (int i = 0; i < sc.accounts_len; i++) {
        sb_add_rawbytes(sb, first ? "[\"0x" : "\",\"0x", bytes(sc.accounts + i * 20, 20), 20);
        first = false;
      }
      if (sc.accounts) {
        _free(sc.accounts);
        sc.accounts_len = 0;
      }
    }
  }
  sb_add_chars(sb, first ? "[]" : "\"]");
  return in3_rpc_handle_finish(ctx);
}
static in3_ret_t pk_add_from_config(in3_configure_ctx_t* ctx, d_curve_type_t type) {
  if (d_type(ctx->token) == T_ARRAY) {
    for_children_of(iter, ctx->token) {
      bytes_t b = d_bytes(iter.token);
      if (b.len != 32) {
        ctx->error_msg = _strdupn("invalid key-length, must be 32", -1);
        return IN3_EINVAL;
      }
      add_key(ctx->client, b.data, type);
    }
  }
  else {
    bytes_t b = d_bytes(ctx->token);
    if (b.len != 32) {
      ctx->error_msg = _strdupn("invalid key-length, must be 32", -1);
      return IN3_EINVAL;
    }
    add_key(ctx->client, b.data, type);
  }
  return IN3_OK;
}

// RPC-Handler
static in3_ret_t pk_rpc(void* data, in3_plugin_act_t action, void* action_ctx) {
  UNUSED_VAR(data);
  switch (action) {
    case PLGN_ACT_CONFIG_SET: {
      in3_configure_ctx_t* ctx = action_ctx;
      if (d_is_key(ctx->token, CONFIG_KEY("key"))) {
        bytes_t b = d_bytes(ctx->token);
        if (b.len != 32) {
          ctx->error_msg = _strdupn("invalid key-length, must be 32", -1);
          return IN3_EINVAL;
        }
        eth_set_request_signer(ctx->client, b.data);
        return IN3_OK;
      }
      if (d_is_key(ctx->token, CONFIG_KEY("pk"))) return pk_add_from_config(ctx, SIGN_CURVE_ECDSA);
      if (d_is_key(ctx->token, CONFIG_KEY("pk_ed25519"))) return pk_add_from_config(ctx, SIGN_CURVE_ED25519);
      return IN3_EIGNORE;
    }

    case PLGN_ACT_RPC_HANDLE: {
      in3_rpc_handle_ctx_t* ctx = action_ctx;
      UNUSED_VAR(ctx); // in case RPC_ONLY is used
#if !defined(RPC_ONLY) || defined(RPC_IN3_ADDJSONKEY)
      TRY_RPC("in3_addJsonKey", in3_addJsonKey(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_ADDRAWKEY)
      TRY_RPC("in3_addRawKey", add_raw_key(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_ADDMNEMONIC)
      TRY_RPC("in3_addMnemonic", in3_addMnemonic(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_SIGNER_IDS)
      TRY_RPC("signer_ids", signer_ids(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_SIGNER_IDS)
      TRY_RPC("eth_accounts", signer_ids(ctx)) // Legacy RPC call. Please, use signer_ids instead
#endif
      return IN3_EIGNORE;
    }

    default:
      return IN3_ENOTSUP;
  }
}

/// RPC-signer
/** signs a reques*/
in3_ret_t eth_sign_req(void* data, in3_plugin_act_t action, void* action_ctx) {
  signer_key_t* k = data;
  switch (action) {
    case PLGN_ACT_PAY_SIGN_REQ: {
      in3_pay_sign_req_ctx_t* ctx = action_ctx;
      in3_ret_t               r   = crypto_sign_digest(ECDSA_SECP256K1, bytes(ctx->request_hash, 32), k->pk, NULL, ctx->signature);
      ctx->signature[64] += 27;
      return r;
    }
    case PLGN_ACT_SIGN: {
      in3_sign_ctx_t* ctx = action_ctx;
      if (ctx->account.len != 20 || memcmp(k->account, ctx->account.data, 20)) return IN3_EIGNORE;
      ctx->signature = bytes(_malloc(65), 65);
      switch (ctx->digest_type) {
        case SIGN_EC_RAW:
          return crypto_sign_digest(ECDSA_SECP256K1, ctx->message, k->pk, NULL, ctx->signature.data);
        case SIGN_EC_HASH: {
          bytes32_t hash;
          keccak(ctx->message, hash);
          return crypto_sign_digest(ECDSA_SECP256K1, bytes(hash, 32), k->pk, NULL, ctx->signature.data);
        }
        default:
          _free(ctx->signature.data);
          return IN3_ENOTSUP;
      }
    }

    case PLGN_ACT_TERM: {
      _free(k);
      return IN3_OK;
    }

    default:
      return IN3_ENOTSUP;
  }
}

/** sets the signer and a pk to the client*/
in3_ret_t eth_set_request_signer(in3_t* in3, bytes32_t pk) {
  signer_key_t* k = _malloc(sizeof(signer_key_t));
  memcpy(k->pk, pk, 32);
  k->account_len = get_address(pk, k->account, SIGN_CURVE_ECDSA);
  k->type        = SIGN_CURVE_ECDSA;
  return in3_plugin_register(in3, PLGN_ACT_PAY_SIGN_REQ | PLGN_ACT_TERM | PLGN_ACT_SIGN, eth_sign_req, k, true);
}

/** register the signer as rpc-plugin providing accounts management functions */
in3_ret_t eth_register_pk_signer(in3_t* in3) {
  return in3_plugin_register(in3, PLGN_ACT_CONFIG_SET | PLGN_ACT_RPC_HANDLE, pk_rpc, NULL, true);
}

/** sets the signer and a pk to the client*/
char* eth_set_pk_signer_from_string(in3_t* in3, char* key, char* path, char* passphrase) {
  bytes32_t key_bytes;
  if (strchr(key, ' ')) { //  it is a seedphrase
    uint8_t seed[64];
    if (mnemonic_verify(key)) return "invalid seedphrase";
    mnemonic_to_seed(key, passphrase, seed, NULL);
    if (!path) path = "m/44'/60'/0'/0/0";

    int path_len = strlen(path);
    int l        = 1;
    for (int i = 0; i < path_len; i++) {
      if (path[i] == ' ' || path[i] == ',') l++;
    }

    uint8_t*  pks = _malloc(l * 32);
    in3_ret_t r   = bip32(bytes(seed, 64), ECDSA_SECP256K1, path, pks);
    memzero(seed, 64);
    if (r == IN3_OK) {
      for (int i = 0; i < l; i++) eth_set_pk_signer(in3, pks + i * 32, SIGN_CURVE_ECDSA, NULL);
      memzero(pks, l * 32);
    }
    _free(pks);
    return r ? in3_errmsg(r) : NULL;
  }
  else {
    if (key[0] == '0' && key[1] == 'x') key += 2;
    if (strlen(key) != 64) return "invalid keylength";
    hex_to_bytes(key, 64, key_bytes, 32);
  }

  eth_set_pk_signer(in3, key_bytes, SIGN_CURVE_ECDSA, NULL);
  return NULL;
}

void eth_set_pk_signer_hex(in3_t* in3, char* key) {
  eth_set_pk_signer_from_string(in3, key, NULL, NULL);
}
