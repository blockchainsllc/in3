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

#include "../../core/client/context_internal.h"
#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/secp256k1.h"
#include "../../verifier/eth1/nano/serialize.h"
#include "iamo_zk.h"
#include <string.h>
#include <time.h>

#define TRY_WALLETS(expr, w1, w2) TRY_CATCH(expr, wallet_free(&w1, false); wallet_free(&w2, false))
typedef enum role {
  ROLE_CHALLENGER = 1,
  ROLE_INITIATOR  = 2,
  ROLE_APPROVER   = 4
} role_t;
typedef struct owner {
  address_t address;
  role_t    role;
} owner_t;

typedef struct wallet {
  address_t account;
  uint32_t  threshold;
  uint32_t  owner_len;
  owner_t*  owners;
} wallet_t;

static in3_ret_t wallet_from_json(in3_ctx_t* ctx, d_token_t* data, wallet_t* w) {
  bytes_t    address = d_to_bytes(d_get(data, K_ADDRESS));
  d_token_t* owners  = d_get(data, key("owners"));
  if (address.len != 20) return ctx_set_error(ctx, "Invalid address in wallet!", IN3_EINVAL);
  if (!owners || d_type(owners) != T_ARRAY) return ctx_set_error(ctx, "Invalid owners in wallet!", IN3_EINVAL);
  memcpy(w->account, address.data, 20);
  w->threshold = (uint32_t) d_get_intk(data, key("threshold"));
  w->owner_len = (uint32_t) d_len(owners);
  w->owners    = _malloc(w->owner_len * sizeof(owner_t));
  for (unsigned int i = 0; i < w->owner_len; i++) {
    d_token_t* o = d_get_at(owners, i);
    if (d_type(o) != T_OBJECT) {
      _free(w->owners);
      return ctx_set_error(ctx, "Invalid owner, must be an object!", IN3_EINVAL);
    }
    w->owners[i].role = (role_t) d_get_intk(o, key("roles"));
    address           = d_to_bytes(d_get(o, K_ADDRESS));
    if (address.len != 20) {
      _free(w->owners);
      return ctx_set_error(ctx, "Invalid owner-addres", IN3_EINVAL);
    }
    memcpy(w->owners[i].address, address.data, 20);
  }
  return IN3_OK;
}

static in3_ret_t wallet_from_bytes(bytes_t data, wallet_t* w) {
  w->owner_len = (data.len - 24) / 21;
  w->threshold = bytes_to_int(data.data + 20, 4);
  memcpy(w->account, data.data, 20);
  w->owners = _malloc(w->owner_len * sizeof(owner_t));
  for (unsigned int i = 0; i < w->owner_len; i++) {
    w->owners[i].role = (role_t) data.data[i * 21 + 24];
    memcpy(w->owners[i].address, data.data + i * 21 + 25, 20);
  }
  return IN3_OK;
  ;
}

static bytes_t wallet_to_bytes(wallet_t* w) {
  bytes_t data = bytes(NULL, 24 + w->owner_len * 21);
  data.data    = _malloc(data.len);
  memcpy(data.data, w->account, 20);
  int_to_bytes(w->threshold, data.data + 20);
  for (unsigned int i = 0; i < w->owner_len; i++) {
    data.data[i * 21 + 24] = (uint8_t) w->owners[i].role;
    memcpy(data.data + i * 21 + 25, w->owners[i].address, 20);
  }
  return data;
}

static void wallet_free(wallet_t* w, bool is_heap) {
  if (w->owners) _free(w->owners);
  if (is_heap) _free(w);
}

static in3_ret_t wallet_get_from_cache(in3_ctx_t* ctx, address_t address, wallet_t* wallet) {
  char account[42];
  account[0] = 'W';
  bytes_to_hex(address, 20, account + 1);
  in3_cache_ctx_t cc = {.content = NULL, .ctx = ctx, .key = account};
  TRY(in3_plugin_execute_first_or_none(ctx, PLGN_ACT_CACHE_GET, &cc))
  if (cc.content)
    TRY_FINAL(wallet_from_bytes(*cc.content, wallet), b_free(cc.content))
  else
    memset(wallet, 0, sizeof(wallet_t));
  return IN3_OK;
}

static in3_ret_t wallet_store_in_cache(in3_ctx_t* ctx, wallet_t* wallet) {
  char account[42];
  account[0] = 'W';
  bytes_to_hex(wallet->account, 20, account + 1);
  bytes_t         data = wallet_to_bytes(wallet);
  in3_cache_ctx_t cc   = {.content = &data, .ctx = ctx, .key = account};
  TRY_FINAL(in3_plugin_execute_first_or_none(ctx, PLGN_ACT_CACHE_SET, &cc), _free(data.data))
  return IN3_OK;
}

static in3_ret_t wallet_verify_signatures(in3_ctx_t* ctx, bytes_t message, wallet_t* wallet, d_token_t* signatures) {
  bytes32_t    msg_hash;
  bytes32_t    tmp;
  uint8_t      pub[65];
  bytes_t      pubkey_bytes     = {.len = 64, .data = ((uint8_t*) &pub) + 1};
  unsigned int valid_signatures = 0;
  bool         initiator        = false;
  keccak(message, msg_hash);
  for (d_iterator_t iter = d_iter(signatures); iter.left; d_iter_next(&iter)) {
    bytes_t sig = d_to_bytes(iter.token);
    if (sig.len != 65) return ctx_set_error(ctx, "Invalid signature (must be 65 bytes)", IN3_EINVAL);
    if (ecdsa_recover_pub_from_sig(&secp256k1, pub, sig.data, msg_hash, sig.data[64] >= 27 ? sig.data[64] - 27 : sig.data[64]))
      return ctx_set_error(ctx, "Invalid Signature", IN3_EINVAL);
    keccak(pubkey_bytes, tmp);

    // find owner
    for (unsigned int i = 0; i < wallet->owner_len; i++) {
      if (memcmp(tmp + 12, wallet->owners[i].address, 20) == 0) {
        if (wallet->owners[i].role & ROLE_INITIATOR) initiator = true;
        if (wallet->owners[i].role & (ROLE_APPROVER | ROLE_INITIATOR)) valid_signatures++;
        break;
      }
    }
  }

  if (!initiator) return ctx_set_error(ctx, "No initiator signature!", IN3_EINVAL);
  if (wallet->threshold > valid_signatures) return ctx_set_error(ctx, "threshold not reached", IN3_EINVAL);

  return IN3_OK;
}

static in3_ret_t wallet_sign(in3_ctx_t* ctx, bytes_t message, wallet_t* wallet, sb_t* sb) {
  bytes32_t    msg_hash;
  unsigned int valid_signatures = 0;
  bool         initiator        = false;
  keccak(message, msg_hash);

  for (unsigned int i = 0; i < wallet->owner_len && valid_signatures < wallet->threshold; i++) {
    if ((wallet->owners[i].role & (ROLE_APPROVER | ROLE_INITIATOR)) == 0) continue;
    bytes_t sig;
    TRY(ctx_require_signature(ctx, SIGN_EC_RAW, &sig, bytes(msg_hash, 32), bytes(wallet->account, 20)))
    if (sig.data[64] < 27) sig.data[64] += 27;

    // find owner
    sb_add_rawbytes(sb, valid_signatures ? "\",\"0x" : "[\"0x", sig, 0);
    if (wallet->owners[i].role & ROLE_INITIATOR) initiator = true;
    valid_signatures++;
  }

  sb_add_chars(sb, valid_signatures ? "\"]" : "[]");

  if (!initiator) return ctx_set_error(ctx, "No initiator signature!", IN3_EINVAL);
  if (wallet->threshold > valid_signatures) return ctx_set_error(ctx, "threshold not reached", IN3_EINVAL);

  return IN3_OK;
}

in3_ret_t wallet_sign_and_send(iamo_zk_config_t* conf, in3_rpc_handle_ctx_t* ctx, wallet_t* wallet, bytes_t message) {
  // we are sending this the the server
  in3_ctx_t* sub = ctx_find_required(ctx->ctx, "iamo_zk_add_ms");
  if (sub) { // do we have a result?
    switch (in3_ctx_state(sub)) {
      case CTX_ERROR:
        return ctx_set_error(ctx->ctx, sub->error, sub->verification_state ? sub->verification_state : IN3_ERPC);
      case CTX_SUCCESS:
        return IN3_OK;
      case CTX_WAITING_TO_SEND:
      case CTX_WAITING_FOR_RESPONSE:
        return IN3_WAITING;
    }
  }

  sb_t  req         = {0};
  char* wallet_data = d_create_json(ctx->ctx->request_context, ctx->params + 1);
  sb_add_chars(&req, "{\"method\":\"iamo_zk_add_ms\",\"params\":[");
  sb_add_chars(&req, wallet_data);
  sb_add_char(&req, ',');
  _free(wallet_data);
  TRY_CATCH(wallet_sign(ctx->ctx, message, wallet, &req), _free(req.data))
  sb_add_chars(&req, "],\"in3\":{\"rpc\":\"");
  sb_add_chars(&req, conf->cosign_rpc);
  sb_add_chars(&req, "\"}}");
  return ctx_add_required(ctx->ctx, ctx_new(ctx->ctx->client, req.data));
}

in3_ret_t iamo_zk_add_ms(iamo_zk_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  CHECK_PARAMS_LEN(ctx->ctx, ctx->params, (conf->cosign_rpc ? 1 : 2))
  CHECK_PARAM_TYPE(ctx->ctx, ctx->params, 0, T_OBJECT)                       // wallet-data
  if (!conf->cosign_rpc) CHECK_PARAM_TYPE(ctx->ctx, ctx->params, 1, T_ARRAY) // signatures
  if (conf->cosign_rpc && d_get(d_get(ctx->request, key("in3")), key("rpc"))) return IN3_EIGNORE;

  // fill wallet
  wallet_t wallet = {0};
  TRY(wallet_from_json(ctx->ctx, ctx->params + 1, &wallet))
  bytes_t message = wallet_to_bytes(&wallet);
  b_to_stack(message);

  if (conf->cosign_rpc)
    // we will sign the message and send it to the server
    TRY_FINAL(wallet_sign_and_send(conf, ctx, &wallet, message), wallet_free(&wallet, false))
  else {

    // get existing wallet
    wallet_t existing;
    TRY_CATCH(wallet_get_from_cache(ctx->ctx, wallet.account, &existing), wallet_free(&wallet, false))

    // check signatures
    TRY_WALLETS(wallet_verify_signatures(ctx->ctx, message, existing.owners ? &existing : &wallet, d_get_at(ctx->params, 1)), wallet, existing)

    // first check the cache
    wallet_free(&existing, false);
    TRY_FINAL(wallet_store_in_cache(ctx->ctx, &wallet), wallet_free(&wallet, false))
  }
  return in3_rpc_handle_with_string(ctx, "true");
}

in3_ret_t iamo_is_valid(iamo_zk_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  UNUSED_VAR(conf);
  CHECK_PARAMS_LEN(ctx->ctx, ctx->params, 3)
  CHECK_PARAM_ADDRESS(ctx->ctx, ctx->params, 1)
  CHECK_PARAM_TYPE(ctx->ctx, ctx->params, 2, T_ARRAY)

  // read arguments
  bytes_t  msg     = d_to_bytes(ctx->params + 1);
  uint8_t* account = d_get_bytes_at(ctx->params, 1)->data;

  // is the wallet registered?
  wallet_t wallet;
  TRY(wallet_get_from_cache(ctx->ctx, account, &wallet))
  if (!wallet.owners) return ctx_set_error(ctx->ctx, "The Account is not registered!", IN3_EINVAL);

  // check signatures (and free the wallet)
  TRY_FINAL(wallet_verify_signatures(ctx->ctx, msg, &wallet, d_get_at(ctx->params, 2)), wallet_free(&wallet, false))

  // if we made it here, signatures are valid
  return in3_rpc_handle_with_string(ctx, "true");
}

in3_ret_t iamo_create_zksync_wallet(iamo_zk_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  // first get from server:
  // - the pubkey
  // - the codehash (deploycode + mastercopy)
  // - creator (iamo factory)
  UNUSED_VAR(conf);

  CHECK_PARAMS_LEN(ctx->ctx, ctx->params, 1)
  CHECK_PARAM_NUMBER(ctx->ctx, ctx->params, 0) // threshold
  bool     has_initiator = false;
  wallet_t wallet        = {0};
  wallet.owners          = alloca((d_len(ctx->params) - 1) * sizeof(owner_t));
  for (d_iterator_t iter = d_iter(ctx->params); iter.left; d_iter_next(&iter)) {
    switch (d_type(iter.token)) {
      case T_STRING: {
        role_t role = 0;
        char*  c    = d_string(iter.token);
        for (int i = 0; i < 4 && *c && *c != ':'; i++, c++) {
          switch (*c) {
            case 'R':
            case 'r':
            case 'C':
            case 'c':
              role |= ROLE_CHALLENGER;
              break;
            case 'A':
            case 'a':
              role |= ROLE_APPROVER;
              break;
            case 'I':
            case 'i':
              has_initiator = true;
              role |= ROLE_INITIATOR;
              break;
            default:
              return ctx_set_error(ctx->ctx, "invalid role-prefix. Must be I,A or C", IN3_EINVAL);
          }
        }
        if (*c != ':') return ctx_set_error(ctx->ctx, "invalid role-prefix. Must be I,A or C, followed by : and the address", IN3_EINVAL);
        c++;
        if (hex_to_bytes(c, -1, wallet.owners[wallet.owner_len].address, 20) != 20) return ctx_set_error(ctx->ctx, "invalid address of owner, must be 20 bytes", IN3_EINVAL);
        wallet.owners[wallet.owner_len++].role = role;
        break;
      }
      case T_INTEGER:
        if (wallet.threshold) return ctx_set_error(ctx->ctx, "threshold already set", IN3_EINVAL);
        wallet.threshold = (uint32_t) d_int(iter.token);
        break;
      case T_BYTES:
        if (d_len(iter.token) != 20) return ctx_set_error(ctx->ctx, "a owner must be a adddress of 20 bytes", IN3_EINVAL);
        memcpy(wallet.owners[wallet.owner_len].address, d_bytes(iter.token)->data, 20);
        wallet.owners[wallet.owner_len++].role = ROLE_APPROVER;
        break;

      default:
        return ctx_set_error(ctx->ctx, "Invalid argument for owner of multisig", IN3_EINVAL);
    }
  }

  if (!wallet.threshold) return ctx_set_error(ctx->ctx, "threshold is missing", IN3_EINVAL);
  if (wallet.threshold > wallet.owner_len) return ctx_set_error(ctx->ctx, "threshold must be less or equal to the number of owners", IN3_EINVAL);
  if (!has_initiator) return ctx_set_error(ctx->ctx, "at least one owner must have a initiator role", IN3_EINVAL);

  // encode the setup-tx

  // if we made it here, signatures are valid
  return in3_rpc_handle_with_string(ctx, "true");
}
