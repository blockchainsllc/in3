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
#include "../../pay/zksync/zk_helper.h"
#include "../../pay/zksync/zksync.h"
#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/secp256k1.h"
#include "../../third-party/zkcrypto/lib.h"
#include "../../verifier/eth1/nano/serialize.h"
#include "iamo_zk.h"
#include <string.h>
#include <time.h>

#define TRY_WALLETS(expr, w1, w2) TRY_CATCH(expr, wallet_free(&w1, false); wallet_free(&w2, false))
#define ONLY_SERVER(ctx) \
  if (d_get(d_get(ctx->request, key("in3")), key("rpc"))) return IN3_EIGNORE;

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

static in3_ret_t find_zksync_conf(in3_ctx_t* ctx, zksync_config_t** conf) {
  for (in3_plugin_t* p = ctx->client->plugins; p; p = p->next) {
    if (p->action_fn == handle_zksync) {
      *conf = p->data;
      return IN3_OK;
    }
  }
  return ctx_set_error(ctx, "no zksync plugin found!", IN3_ECONFIG);
}
static in3_ret_t iamo_zk_check_rpc(iamo_zk_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  if (conf->cosign_rpc) return IN3_OK;
  zksync_config_t* zconf;
  TRY(find_zksync_conf(ctx->ctx, &zconf))
  if (zconf->musig_urls) {
    for (size_t i = 0; i < 2; i++) {
      if (zconf->musig_urls[i]) {
        conf->cosign_rpc = _strdupn(zconf->musig_urls[i], -1);
        return IN3_OK;
      }
    }
  }
  return IN3_OK;
}

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

static void wallet_to_json(wallet_t* w, sb_t* sb) {
  sb_add_rawbytes(sb, "{\"address\":\"0x", bytes(w->account, 20), 0);
  sb_add_chars(sb, "\",\"threshold\":");
  sb_add_int(sb, w->threshold);
  sb_add_chars(sb, ",\"owners\":");
  for (unsigned int i = 0; i < w->owner_len; i++) {
    sb_add_chars(sb, i ? ",{\"roles\":" : "[{\"roles\":");
    sb_add_int(sb, w->owners[i].role);
    sb_add_rawbytes(sb, ",\"address\":\"0x", bytes(w->owners[i].address, 20), 0);
    sb_add_chars(sb, "\"}");
  }
  sb_add_chars(sb, w->owner_len ? "]}" : "[]}");
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
    TRY(ctx_require_signature(ctx, SIGN_EC_RAW, &sig, bytes(msg_hash, 32), bytes(wallet->owners[i].address, 20)))
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

in3_ret_t wallet_sign_and_send(iamo_zk_config_t* conf, in3_ctx_t* ctx, wallet_t* wallet, bytes_t message) {
  // we are sending this the the server
  in3_ctx_t* sub = ctx_find_required(ctx, "iamo_zk_add_wallet");
  if (sub) { // do we have a result?
    switch (in3_ctx_state(sub)) {
      case CTX_ERROR:
        return ctx_set_error(ctx, sub->error, sub->verification_state ? sub->verification_state : IN3_ERPC);
      case CTX_SUCCESS:
        return IN3_OK;
      case CTX_WAITING_TO_SEND:
      case CTX_WAITING_FOR_RESPONSE:
        return IN3_WAITING;
    }
  }

  sb_t req = {0};
  sb_add_chars(&req, "{\"method\":\"iamo_zk_add_wallet\",\"params\":[");
  wallet_to_json(wallet, &req);
  sb_add_char(&req, ',');
  TRY_CATCH(wallet_sign(ctx, message, wallet, &req), _free(req.data))
  sb_add_chars(&req, "],\"in3\":{\"rpc\":\"");
  sb_add_chars(&req, conf->cosign_rpc);
  sb_add_chars(&req, "\"}}");
  return ctx_add_required(ctx, ctx_new(ctx->client, req.data));
}

in3_ret_t iamo_zk_add_wallet(iamo_zk_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  TRY(iamo_zk_check_rpc(conf, ctx))
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
    TRY_FINAL(wallet_sign_and_send(conf, ctx->ctx, &wallet, message), wallet_free(&wallet, false))
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

in3_ret_t iamo_zk_is_valid(iamo_zk_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
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

static in3_ret_t zksync_get_user_pubkey(zksync_config_t* conf, in3_ctx_t* ctx, uint8_t* pubkey) {
  if (!memiszero(conf->pub_key, 32))
    memcpy(pubkey, conf->pub_key, 32);
  else {
    bytes32_t k;
    TRY(zksync_get_sync_key(conf, ctx, k))
    TRY(zkcrypto_pk_to_pubkey(k, pubkey))
    memcpy(conf->pub_key, pubkey, 32);
  }
  return IN3_OK;
}

static bytes_t encode_setup(wallet_t* wallet) {
  bytes_t res = bytes(NULL, wallet->owner_len * 64 + 388);
  res.data    = _calloc(res.len, 1);
  memcpy(res.data, "\x6e\xfc\x73\xce", 4);                                                 // setup(address[],uint8[],uint32,address,bytes,address,address,uint256,address)
  int_to_bytes(288, res.data + 4 + 32 - 4);                                                // offset for owners
  int_to_bytes(320 + 32 * wallet->owner_len, res.data + 4 + 64 - 4);                       // offset for roles
  int_to_bytes(wallet->threshold, res.data + 4 + 96 - 4);                                  // threshold
  int_to_bytes(352 + 64 * wallet->owner_len, res.data + 4 + 160 - 4);                      // offset for txdata ( currently empty)
  int_to_bytes(wallet->owner_len, res.data + 4 + 320 - 4);                                 // owner address array len
  int_to_bytes(wallet->owner_len, res.data + 4 + 352 + wallet->owner_len * 32 - 4);        // role array len
  for (unsigned int i = 0; i < wallet->owner_len; i++) {                                   // handle all owners
    memcpy(res.data + 4 + 320 + 12 + i * 32, wallet->owners[i].address, 20);               // copy owner address
    int_to_bytes(wallet->owners[i].role, res.data + 4 + 384 + wallet->owner_len * 32 - 4); // roles
  }
  return res;
}

static bytes_t encode_deploy_data(address_t master_copy, bytes_t setup, bytes32_t nonce) {
  bytes_t res = bytes(NULL, ((setup.len + 31) / 32) * 32 + 4 * 32 + 4);
  res.data    = _calloc(res.len, 1);
  memcpy(res.data, "\x16\x88\xf0\xb9", 4);           // createProxyWithNonce(address,bytes,uint256)
  memcpy(res.data + 4 + 12, master_copy, 20);        // mastercopy
  memcpy(res.data + 4 + 64, nonce, 32);              // nonce
  int_to_bytes(96, res.data + 4 + 64 - 4);           // offset for setup data
  int_to_bytes(setup.len, res.data + 4 + 128 - 4);   // len of setup data
  memcpy(res.data + 4 + 128, setup.data, setup.len); // setup data
  return res;
}

static in3_ret_t read_server_config(iamo_zk_config_t* conf, in3_ctx_t* ctx, d_token_t** result) {
  if (!conf->cosign_rpc) return ctx_set_error(ctx, "No cosign-rpc set in config!", IN3_ECONFIG);
  char* in3 = alloca(strlen(conf->cosign_rpc) + 20);
  sprintf(in3, "{\"rpc\":\"%s\"}", conf->cosign_rpc);
  return ctx_send_sub_request(ctx, "iamo_zk_get_config", "", in3, result);
}

static in3_ret_t wallet_from_args(in3_rpc_handle_ctx_t* ctx, wallet_t* wallet) {
  bool has_initiator = false;

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
        if (hex_to_bytes(c, -1, wallet->owners[wallet->owner_len].address, 20) != 20) return ctx_set_error(ctx->ctx, "invalid address of owner, must be 20 bytes", IN3_EINVAL);
        wallet->owners[wallet->owner_len++].role = role;
        break;
      }
      case T_INTEGER:
        if (wallet->threshold) return ctx_set_error(ctx->ctx, "threshold already set", IN3_EINVAL);
        wallet->threshold = (uint32_t) d_int(iter.token);
        break;
      case T_BYTES:
        if (d_len(iter.token) != 20) return ctx_set_error(ctx->ctx, "a owner must be a adddress of 20 bytes", IN3_EINVAL);
        memcpy(wallet->owners[wallet->owner_len].address, d_bytes(iter.token)->data, 20);
        wallet->owners[wallet->owner_len++].role = ROLE_APPROVER;
        break;

      default:
        return ctx_set_error(ctx->ctx, "Invalid argument for owner of multisig", IN3_EINVAL);
    }
  }

  if (!wallet->threshold) return ctx_set_error(ctx->ctx, "threshold is missing", IN3_EINVAL);
  if (wallet->threshold > wallet->owner_len) return ctx_set_error(ctx->ctx, "threshold must be less or equal to the number of owners", IN3_EINVAL);
  if (!has_initiator) return ctx_set_error(ctx->ctx, "at least one owner must have a initiator role", IN3_EINVAL);
  return IN3_OK;
}

in3_ret_t iamo_zk_create_wallet(iamo_zk_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  TRY(iamo_zk_check_rpc(conf, ctx))
  CHECK_PARAMS_LEN(ctx->ctx, ctx->params, 1)
  CHECK_PARAM_NUMBER(ctx->ctx, ctx->params, 0) // threshold
  wallet_t wallet = {0};
  wallet.owners   = alloca((d_len(ctx->params) - 1) * sizeof(owner_t));
  TRY(wallet_from_args(ctx, &wallet))
  // first get from server:
  d_token_t* server_config;
  TRY(read_server_config(conf, ctx->ctx, &server_config))

  // encode the setup-tx

  bytes32_t        saltarg;
  zksync_config_t* zksync_conf;
  TRY(find_zksync_conf(ctx->ctx, &zksync_conf))

  // build common pubkey
  uint8_t pubkeys[64];
  TRY(zksync_get_user_pubkey(zksync_conf, ctx->ctx, pubkeys))
  bytes_t mastercopy    = d_to_bytes(d_get(server_config, key("mastercopy")));
  bytes_t creator       = d_to_bytes(d_get(server_config, key("creator")));
  bytes_t codehash      = d_to_bytes(d_get(server_config, key("codehash")));
  bytes_t server_pubkey = d_to_bytes(d_get(server_config, key("pubkey")));
  if (mastercopy.len != 20) return ctx_set_error(ctx->ctx, "Invalid mastercopy from the server!", IN3_EINVAL);
  if (creator.len != 20) return ctx_set_error(ctx->ctx, "Invalid creator from the server!", IN3_EINVAL);
  if (codehash.len != 32) return ctx_set_error(ctx->ctx, "Invalid codehash from the server!", IN3_EINVAL);
  if (server_pubkey.len != 32) return ctx_set_error(ctx->ctx, "Invalid public key from the server!", IN3_EINVAL);
  memcpy(pubkeys + 32, server_pubkey.data, 32);

  bytes32_t pubkeyhash = {0};
  bytes32_t common_pubkey;
  TRY(zkcrypto_compute_aggregated_pubkey(bytes(pubkeys, 64), common_pubkey))
  TRY(zkcrypto_pubkey_hash(bytes(common_pubkey, 32), pubkeyhash + 12))

  bytes_t setup_data = encode_setup(&wallet);
  keccak(setup_data, saltarg);

  // calculate account-address
  zksync_calculate_account(creator.data, codehash.data, saltarg, pubkeyhash + 12, wallet.account);
  bytes_t message = wallet_to_bytes(&wallet);
  b_to_stack(message);

  // we will sign the message and send it to the server
  TRY_CATCH(wallet_sign_and_send(conf, ctx->ctx, &wallet, message), _free(setup_data.data))

  // cal txdata for deployment
  bytes_t deploy_tx = encode_deploy_data(mastercopy.data, setup_data, pubkeyhash);

  // prepare  output
  sb_t* sb = in3_rpc_handle_start(ctx);
  sb_add_rawbytes(sb, "{\"account\":\"0x", bytes(wallet.account, 20), 0);
  sb_add_rawbytes(sb, "\",\"deploy_tx\":{\"data\":\"0x", deploy_tx, 0);
  sb_add_rawbytes(sb, "\",\"to\":\"0x", creator, 0);
  sb_add_rawbytes(sb, "\"},\"create2\":{\"creator\":\"0x", creator, 0);
  sb_add_rawbytes(sb, "\",\"saltarg\":\"0x", bytes(saltarg, 32), 0);
  sb_add_rawbytes(sb, "\",\"codehash\":\"0x", codehash, 0);
  sb_add_rawbytes(sb, "\"},\"musig_pub_keys\":\"0x", bytes(pubkeys, 64), 0);
  sb_add_chars(sb, "\",\"musig_urls\":[null,\"");
  sb_add_chars(sb, conf->cosign_rpc);
  sb_add_chars(sb, "\"],\"wallet\":");
  wallet_to_json(&wallet, sb);
  sb_add_chars(sb, "}");

  // cleanup
  _free(setup_data.data);
  _free(deploy_tx.data);

  // if we made it here, signatures are valid
  return in3_rpc_handle_finish(ctx);
}

in3_ret_t iamo_zk_get_config(iamo_zk_config_t* conf, in3_rpc_handle_ctx_t* ctx) {
  zksync_config_t* zconf;
  bytes32_t        pubkey;
  ONLY_SERVER(ctx)
  TRY(iamo_zk_check_rpc(conf, ctx))
  if (conf->cosign_rpc) return ctx_set_error(ctx->ctx, "getting config only works for server without a cosign_rpc ", IN3_ECONFIG);

  //TODO read from config
  address_t master_copy = {0};
  address_t creator     = {0}; // factory
  bytes32_t codehash    = {0};

  TRY(find_zksync_conf(ctx->ctx, &zconf))
  TRY(zksync_get_user_pubkey(zconf, ctx->ctx, pubkey))

  sb_t* sb = in3_rpc_handle_start(ctx);
  sb_add_rawbytes(sb, "{\"pubkey\":\"0x", bytes(pubkey, 32), 0);
  sb_add_rawbytes(sb, "\",\"mastercopy\":\"0x", bytes(master_copy, 20), 0);
  sb_add_rawbytes(sb, "\",\"creator\":\"0x", bytes(creator, 20), 0);
  sb_add_rawbytes(sb, "\",\"codehash\":\"0x", bytes(codehash, 32), 0);
  sb_add_chars(sb, "\"}");
  return in3_rpc_handle_finish(ctx);
}
