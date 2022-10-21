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
#include "../../core/util/crypto.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "../../verifier/eth1/nano/serialize.h"
#include "signer.h"
#include <string.h>

int eth_get_address(uint8_t* pk, uint8_t* address, in3_curve_type_t type) {
  in3_ret_t r;
  switch (type) {
    case ECDSA_SECP256K1: {
      uint8_t public_key[64];
      r = crypto_convert(ECDSA_SECP256K1, CONV_PK32_TO_PUB64, bytes(pk, 32), public_key, NULL);
      keccak(bytes(public_key, 64), public_key);
      memcpy(address, public_key + 12, 20);
      return r == IN3_OK ? 20 : r;
    }
    case EDDSA_ED25519:
      r = crypto_convert(EDDSA_ED25519, CONV_PK32_TO_PUB32, bytes(pk, 32), address, NULL);
      return r == IN3_OK ? 32 : r;
    default:
      return IN3_ENOTSUP;
  }
}

bool signer_add_key(in3_t* c, bytes32_t pk, in3_curve_type_t type) {
  uint8_t                pub[64];
  int                    l   = eth_get_address(pk, pub, type);
  in3_sign_account_ctx_t ctx = {0};
  in3_req_t              r   = {0};
  ctx.req                    = &r;
  r.client                   = c;
  bool is_same_address       = false;

  for (in3_plugin_t* p = c->plugins; p; p = p->next) {
    if ((p->acts & PLGN_ACT_SIGN_ACCOUNT) && (p->acts & PLGN_ACT_SIGN) && p->action_fn(p->data, PLGN_ACT_SIGN_ACCOUNT, &ctx) == IN3_OK && ctx.accounts_len) {
      is_same_address = type == ctx.curve_type && memcmp(ctx.accounts, pub, l) == 0;
      _free(ctx.accounts);
      if (is_same_address) break;
    }
  }

  if (r.required) TRY(req_remove_required(&r, r.required, true))
  if (is_same_address) return false;

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
      for (int i = 0; i < l; i++) eth_set_pk_signer(in3, pks + i * 32, ECDSA_SECP256K1, NULL);
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

  eth_set_pk_signer(in3, key_bytes, ECDSA_SECP256K1, NULL);
  return NULL;
}

void eth_set_pk_signer_hex(in3_t* in3, char* key) {
  eth_set_pk_signer_from_string(in3, key, NULL, NULL);
}
