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
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/secp256k1.h"
#include "../../verifier/eth1/nano/serialize.h"
#include <string.h>

typedef struct signer_key {
  bytes32_t pk;
  address_t account;
} signer_key_t;

/** hash data with given hasher type and sign the given data with give private key*/
in3_ret_t ec_sign_pk_hash(uint8_t* message, size_t len, uint8_t* pk, hasher_t hasher, uint8_t* dst) {
  if (hasher == hasher_sha3k && ecdsa_sign(&secp256k1, HASHER_SHA3K, pk, message, len, dst, dst + 64, NULL) < 0)
    return IN3_EUNKNOWN;
  return IN3_OK;
}

/**  sign the given data with give private key */
in3_ret_t ec_sign_pk_raw(uint8_t* message, uint8_t* pk, uint8_t* dst) {
  if (ecdsa_sign_digest(&secp256k1, pk, message, dst, dst + 64, NULL) < 0)
    return IN3_EUNKNOWN;
  return IN3_OK;
}

static void get_address(uint8_t* pk, uint8_t* address) {
  uint8_t public_key[65], sdata[32];
  ecdsa_get_public_key65(&secp256k1, pk, public_key);
  keccak(bytes(public_key + 1, 64), sdata);
  memcpy(address, sdata + 12, 20);
}

static bool add_key(in3_t* c, bytes32_t pk) {
  address_t address;
  get_address(pk, address);
  in3_sign_account_ctx_t ctx = {0};

  for (in3_plugin_t* p = c->plugins; p; p = p->next) {
    if (p->acts & (PLGN_ACT_SIGN_ACCOUNT | PLGN_ACT_SIGN) && p->action_fn(p->data, PLGN_ACT_SIGN_ACCOUNT, &ctx) == IN3_OK && ctx.accounts_len) {
      bool is_same_address = memcmp(ctx.accounts, address, 20) == 0;
      _free(ctx.accounts);
      if (is_same_address) return false;
    }
  }

  eth_set_pk_signer(c, pk);
  return true;
}

static in3_ret_t eth_sign_pk(void* data, in3_plugin_act_t action, void* action_ctx) {
  signer_key_t* k = data;
  switch (action) {
    case PLGN_ACT_SIGN: {
      in3_sign_ctx_t* ctx = action_ctx;
      if (ctx->account.len == 20 && memcmp(k->account, ctx->account.data, 20)) return IN3_EIGNORE;
      ctx->signature = bytes(_malloc(65), 65);
      switch (ctx->type) {
        case SIGN_EC_RAW:
          return ec_sign_pk_raw(ctx->message.data, k->pk, ctx->signature.data);
        case SIGN_EC_HASH:
          return ec_sign_pk_hash(ctx->message.data, ctx->message.len, k->pk, hasher_sha3k, ctx->signature.data);
        default:
          _free(ctx->signature.data);
          return IN3_ENOTSUP;
      }
    }

    case PLGN_ACT_SIGN_ACCOUNT: {
      // generate the address from the key
      in3_sign_account_ctx_t* ctx = action_ctx;
      ctx->signer_type            = SIGNER_ECDSA;
      ctx->accounts               = _malloc(20);
      ctx->accounts_len           = 1;
      memcpy(ctx->accounts, k->account, 20);
      return IN3_OK;
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
in3_ret_t eth_set_pk_signer(in3_t* in3, bytes32_t pk) {
  signer_key_t* k = _malloc(sizeof(signer_key_t));
  get_address(pk, k->account);
  memcpy(k->pk, pk, 32);
  return in3_plugin_register(in3, PLGN_ACT_SIGN_ACCOUNT | PLGN_ACT_SIGN | PLGN_ACT_TERM, eth_sign_pk, k, false);
}

// RPC-Handler
static in3_ret_t pk_rpc(void* data, in3_plugin_act_t action, void* action_ctx) {
  UNUSED_VAR(data);
  switch (action) {
    case PLGN_ACT_CONFIG_SET: {
      in3_configure_ctx_t* ctx = action_ctx;
      if (ctx->token->key == key("key")) {
        if (d_type(ctx->token) != T_BYTES || d_len(ctx->token) != 32) {
          ctx->error_msg = _strdupn("invalid key-length, must be 32", -1);
          return IN3_EINVAL;
        }
        eth_set_request_signer(ctx->client, ctx->token->data);
        return IN3_OK;
      }
      if (ctx->token->key == key("pk")) {
        if (d_type(ctx->token) == T_BYTES) {
          if (d_len(ctx->token) != 32) {
            ctx->error_msg = _strdupn("invalid key-length, must be 32", -1);
            return IN3_EINVAL;
          }
          add_key(ctx->client, d_bytes(ctx->token)->data);
          return IN3_OK;
        }
        else if (d_type(ctx->token) == T_ARRAY) {
          for (d_iterator_t iter = d_iter(ctx->token); iter.left; d_iter_next(&iter)) {
            if (d_type(iter.token) == T_BYTES) {
              if (d_len(iter.token) != 32) {
                ctx->error_msg = _strdupn("invalid key-length, must be 32", -1);
                return IN3_EINVAL;
              }
              add_key(ctx->client, d_bytes(iter.token)->data);
            }
          }
          return IN3_OK;
        }
        ctx->error_msg = _strdupn("invalid type for a pk", -1);
        return IN3_EINVAL;
      }
      return IN3_EIGNORE;
    }

    case PLGN_ACT_RPC_HANDLE: {
      in3_rpc_handle_ctx_t* ctx = action_ctx;
      if (strcmp(ctx->method, "in3_addRawKey") == 0) {
        if (d_len(ctx->params) != 1 || d_type(ctx->params + 1) != T_BYTES || d_len(ctx->params + 1) != 32)
          return req_set_error(ctx->req, "one argument with 32 bytes is required!", IN3_EINVAL);
        address_t adr;
        get_address(d_bytes(ctx->params + 1)->data, adr);
        add_key(ctx->req->client, d_bytes(ctx->params + 1)->data);
        return in3_rpc_handle_with_bytes(ctx, bytes(adr, 20));
      }
      if (strcmp(ctx->method, "eth_accounts") == 0) {
        sb_t*                  sb    = in3_rpc_handle_start(ctx);
        bool                   first = true;
        in3_sign_account_ctx_t sc    = {.req = ctx->req, .accounts = NULL, .accounts_len = 0, .signer_type = 0};
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
      return IN3_EIGNORE;
    }

    default:
      return IN3_ENOTSUP;
  }
}

/// RPC-signer

in3_ret_t eth_sign_req(void* data, in3_plugin_act_t action, void* action_ctx) {
  signer_key_t* k = data;
  switch (action) {
    case PLGN_ACT_PAY_SIGN_REQ: {
      in3_pay_sign_req_ctx_t* ctx = action_ctx;
      in3_ret_t               r   = ec_sign_pk_raw(ctx->request_hash, k->pk, ctx->signature);
      ctx->signature[64] += 27;
      return r;
    }
    case PLGN_ACT_SIGN: {
      in3_sign_ctx_t* ctx = action_ctx;
      if (ctx->account.len != 20 || memcmp(k->account, ctx->account.data, 20)) return IN3_EIGNORE;
      ctx->signature = bytes(_malloc(65), 65);
      switch (ctx->type) {
        case SIGN_EC_RAW:
          return ec_sign_pk_raw(ctx->message.data, k->pk, ctx->signature.data);
        case SIGN_EC_HASH:
          return ec_sign_pk_hash(ctx->message.data, ctx->message.len, k->pk, hasher_sha3k, ctx->signature.data);
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
  get_address(pk, k->account);
  return in3_plugin_register(in3, PLGN_ACT_PAY_SIGN_REQ | PLGN_ACT_TERM | PLGN_ACT_SIGN, eth_sign_req, k, true);
}

in3_ret_t eth_register_pk_signer(in3_t* in3) {
  return in3_plugin_register(in3, PLGN_ACT_CONFIG_SET | PLGN_ACT_RPC_HANDLE, pk_rpc, NULL, true);
}

/** sets the signer and a pk to the client*/
void eth_set_pk_signer_hex(in3_t* in3, char* key) {
  if (key[0] == '0' && key[1] == 'x') key += 2;
  if (strlen(key) != 64) return;
  bytes32_t key_bytes;
  hex_to_bytes(key, 64, key_bytes, 32);
  eth_set_pk_signer(in3, key_bytes);
}
