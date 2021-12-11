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
#include "../../signer/pk-signer/signer.h"
#include "../../third-party/crypto/bignum.h"
#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/rand.h"
#include "../../third-party/crypto/secp256k1.h"
#include "../../verifier/eth1/basic/eth_basic.h"
#include "../../verifier/eth1/nano/rlp.h"
#include "abi.h"
#include "ens.h"
#include "eth_api.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef ETH_FULL
#include "../../third-party/tommath/tommath.h"
#endif
#define ETH_SIGN_PREFIX "\x19" \
                        "Ethereum Signed Message:\n%u"

static in3_ret_t in3_abiEncode(in3_rpc_handle_ctx_t* ctx) {
  char* sig;
  TRY_PARAM_GET_REQUIRED_STRING(sig, ctx, 0)
  in3_ret_t  ret   = IN3_OK;
  bytes_t    data  = {0};
  char*      error = NULL;
  d_token_t* para  = d_get_at(ctx->params, 1);
  if (!sig) return req_set_error(ctx->req, "missing signature", IN3_EINVAL);
  if (!para) return req_set_error(ctx->req, "missing values", IN3_EINVAL);
  abi_sig_t* s = abi_sig_create(sig, &error);
  if (!error)
    data = abi_encode(s, para, &error);
  if (!error)
    ret = in3_rpc_handle_with_bytes(ctx, data);
  if (s) abi_sig_free(s);
  if (data.data) _free(data.data);
  return error ? req_set_error(ctx->req, error, IN3_EINVAL) : ret;
}

static in3_ret_t in3_abiDecode(in3_rpc_handle_ctx_t* ctx) {
  char*   sig;
  bytes_t data, topics;

  TRY_PARAM_GET_REQUIRED_STRING(sig, ctx, 0)
  TRY_PARAM_GET_REQUIRED_BYTES(data, ctx, 1, 0, 0)
  TRY_PARAM_GET_BYTES(topics, ctx, 2, 0, 0)
  CHECK_PARAM(ctx->req, ctx->params, 1, val->len % 32 == 0)
  char*       error = NULL;
  json_ctx_t* res   = NULL;
  abi_sig_t*  req   = abi_sig_create(sig, &error);
  if (!error) res = topics.data ? abi_decode_event(req, topics, data, &error) : abi_decode(req, data, &error);
  if (req) abi_sig_free(req);
  if (error) return req_set_error(ctx->req, error, IN3_EINVAL);
  char* result = d_create_json(res, res->result);
  in3_rpc_handle_with_string(ctx, result);
  _free(result);
  if (res) json_free(res);
  return IN3_OK;
}

static in3_ret_t rlp_decode_data(sb_t* sb, bytes_t data, int index) {
  bytes_t dst  = {0};
  int     type = rlp_decode(&data, index, &dst);
  if (type == 1) {
    if (index) sb_add_char(sb, ',');
    sb_add_bytes(sb, "", &dst, 1, false);
    return IN3_OK;
  }
  else if (type == 2) {
    if (index) sb_add_char(sb, ',');
    sb_add_char(sb, '[');
    data = dst;
    for (int i = 0; rlp_decode_data(sb, data, i) == IN3_OK; i++) {}
    sb_add_char(sb, ']');
    return IN3_OK;
  }
  return IN3_ELIMIT;
}

static in3_ret_t in3_rlpDecode(in3_rpc_handle_ctx_t* ctx) {
  bytes_t data = {0};
  TRY_PARAM_GET_REQUIRED_BYTES(data, ctx, 0, 1, 0)
  rlp_decode_data(in3_rpc_handle_start(ctx), data, 0);
  return in3_rpc_handle_finish(ctx);
}

static const char* TX_FIELDS_0[] = {"nonce", "gasPrice", "gas", "to", "value", "data", "v", "r", "s", 0};
static const char* TX_FIELDS_1[] = {"chainId", "nonce", "gasPrice", "gas", "to", "value", "data", "accessList", "v", "r", "s", 0};
static const char* TX_FIELDS_2[] = {"chainId", "nonce", "maxPriorityFeePerGas", "maxFeePerGas", "gas", "to", "value", "data", "accessList", "v", "r", "s", 0};

static in3_ret_t in3_decodeTx(in3_rpc_handle_ctx_t* ctx) {
  bytes_t      data = {0}, val;
  bytes32_t    hash;
  sb_t         sb    = {0};
  int64_t      type  = 0;
  const char** names = TX_FIELDS_0;
  TRY_PARAM_GET_REQUIRED_BYTES(data, ctx, 0, 1, 0)
  keccak(data, hash);

  if (data.data[0] < 0xc0) {
    type  = data.data[0];
    names = type == 1 ? TX_FIELDS_1 : TX_FIELDS_2;
    if (type > 2) return req_set_error(ctx->req, "Invalid TxType", IN3_EINVAL);
    data.len--;
    data.data++;
  }
  if (rlp_decode(&data, 0, &data) != 2) return req_set_error(ctx->req, "Invalid Tx-Data, must be a list", IN3_EINVAL);
  int len = rlp_decode_len(&data);

  sb_printx(&sb, "{\"type\":%d,\"hash\":\"%B\"", type, bytes(hash, 32));

  for (int i = 0; names[i] && i < len; i++) {
    int t = rlp_decode(&data, i, &val);
    if (strcmp(names[i], "accessList") == 0 && t == 2) {
      sb_printx(&sb, ",\"%s\":[", names[i]);
      bytes_t account, addr, storage;
      for (int a = 0;; a++) {
        t = rlp_decode(&val, a, &account);
        if (!t) break;
        if (t != 2 || rlp_decode(&account, 0, &addr) != 1 || rlp_decode(&account, 1, &storage) != 2) {
          _free(sb.data);
          return req_set_error(ctx->req, "Invalid Tx-Data, wrong accessList account", IN3_EINVAL);
        }
        if (a) sb_add_char(&sb, ',');
        sb_printx(&sb, "{\"address\":\"%B\",\"storageKeys\":[", addr);
        for (int b = 0;; b++) {
          t = rlp_decode(&storage, b, &addr);
          if (!t) break;
          if (t != 1) {
            _free(sb.data);
            return req_set_error(ctx->req, "Invalid Tx-Data, wrong accessList sotrage key", IN3_EINVAL);
          }
          if (b) sb_add_char(&sb, ',');
          sb_printx(&sb, "\"%B\"", addr);
        }
        sb_add_chars(&sb, "]}");
      }
      sb_add_chars(&sb, "]");
    }
    else if ((strlen(names[i]) < 3 || strcmp(names[i], "data") == 0) && t == 1)
      sb_printx(&sb, ",\"%s\":\"%B\"", names[i], val);
    else if (t == 1)
      sb_printx(&sb, ",\"%s\":\"%V\"", names[i], val);
    else {
      _free(sb.data);
      return req_set_error(ctx->req, "Invalid Tx-Data, wrong item", IN3_EINVAL);
    }
  }

  // determine from-address
  if (len && !names[len]) {
    uint8_t pub[65];
    bytes_t pubkey_bytes = {.len = 64, .data = ((uint8_t*) &pub) + 1};
    bytes_t last, v;
    rlp_decode(&data, len - 1, &v);
    if (v.len == 0) {
      rlp_decode(&data, len - 3, &v);
      sb_printx(&sb, ",\"chainId\":\"%V\"", v);
    }
    else {

      rlp_decode(&data, len - 3, &v);
      rlp_decode(&data, len - 4, &last);
      uint8_t          r   = v.len ? v.data[v.len - 1] : 0;
      uint8_t          tt  = (uint8_t) type;
      bytes_builder_t* rlp = bb_newl(data.len);
      bb_write_raw_bytes(rlp, data.data, last.data + last.len - data.data); // copy the existing data without signature
      if (type == 0 && (v.len > 1 || (v.len == 1 && r > 28))) {
        int c = bytes_to_int(v.data, v.len);
        r     = 1 - c % 2;
        c     = (c - (36 - c % 2)) / 2;
        uint8_t tmp[4];
        int_to_bytes((uint32_t) c, tmp);
        bytes_t bb = bytes(tmp, 4);
        b_optimize_len(&bb);
        sb_printx(&sb, ",\"chainId\":\"%V\"", bb);
        rlp_encode_item(rlp, &bb);
        bb.len = 0;
        rlp_encode_item(rlp, &bb);
        rlp_encode_item(rlp, &bb);
      }
      else if (type == 0 && r > 26)
        r -= 27;
      rlp_encode_to_list(rlp);
      if (type) bb_replace(rlp, 0, 0, &tt, 1); // we insert the type
      sb_printx(&sb, ",\"unsigned\":\"%B\"", rlp->b);
      keccak(rlp->b, hash);
      bb_free(rlp);
      uint8_t signature[65] = {0};
      rlp_decode(&data, len - 2, &v);
      memcpy(signature + 32 - v.len, v.data, v.len);
      rlp_decode(&data, len - 1, &v);
      memcpy(signature + 64 - v.len, v.data, v.len);
      signature[64] = r;
      sb_printx(&sb, ",\"signature\":\"%B\"", bytes(signature, 65));

      if (ecdsa_recover_pub_from_sig(&secp256k1, pub, signature, hash, r)) {
        _free(sb.data);
        return req_set_error(ctx->req, "Invalid Signature", IN3_EINVAL);
      }
      keccak(pubkey_bytes, hash);
      sb_printx(&sb, ",\"publicKey\":\"%B\",\"from\":\"%B\"", pubkey_bytes, bytes(hash + 12, 20));
    }
  }

  sb_add_char(&sb, '}');
  in3_rpc_handle_with_string(ctx, sb.data);
  _free(sb.data);
  return IN3_OK;
}

static in3_ret_t in3_checkSumAddress(in3_rpc_handle_ctx_t* ctx) {
  uint8_t* src;
  bool     use_chain_id;
  TRY_PARAM_GET_REQUIRED_ADDRESS(src, ctx, 0)
  TRY_PARAM_GET_BOOL(use_chain_id, ctx, 1, 0)
  char      result[45];
  in3_ret_t res = to_checksum(src, use_chain_id ? in3_chain_id(ctx->req) : 0, result + 1);
  if (res) return req_set_error(ctx->req, "Could not create the checksum address", res);
  result[0]  = '\'';
  result[43] = '\'';
  result[44] = 0;

  return in3_rpc_handle_with_string(ctx, result);
}

static in3_ret_t in3_ens(in3_rpc_handle_ctx_t* ctx) {
  char *  name, *type;
  bytes_t registry = bytes(NULL, 20);

  TRY_PARAM_GET_REQUIRED_STRING(name, ctx, 0)
  TRY_PARAM_GET_STRING(type, ctx, 1, "addr")
  TRY_PARAM_GET_ADDRESS(registry.data, ctx, 2, NULL)

  int          res_len  = 20;
  in3_ens_type ens_type = ENS_ADDR;
  bytes32_t    result;

  // verify input
  if (!strchr(name, '.')) return req_set_error(ctx->req, "the first param must be a valid domain name", IN3_EINVAL);
  if (strcmp(type, "addr") == 0)
    ens_type = ENS_ADDR;
  else if (strcmp(type, "resolver") == 0)
    ens_type = ENS_RESOLVER;
  else if (strcmp(type, "owner") == 0)
    ens_type = ENS_OWNER;
  else if (strcmp(type, "hash") == 0)
    ens_type = ENS_HASH;
  else
    return req_set_error(ctx->req, "currently only 'hash','addr','owner' or 'resolver' are allowed as type", IN3_EINVAL);

  TRY(ens_resolve(ctx->req, name, registry.data, ens_type, result, &res_len))

  return in3_rpc_handle_with_bytes(ctx, bytes(result, res_len));
}

static const char* UNITS[] = {
    "wei", "",
    "kwei", "\x03",
    "Kwei", "\x03",
    "babbage", "\x03",
    "femtoether", "\x03",
    "mwei", "\x06",
    "Mwei", "\x06",
    "lovelace", "\x06",
    "picoether", "\x06",
    "gwei", "\x09",
    "Gwei", "\x09",
    "shannon", "\x09",
    "nanoether", "\x09",
    "nano", "\x09",
    "szabo", "\x0c",
    "microether", "\x0c",
    "micro", "\x0c",
    "finney", "\x0f",
    "milliether", "\x0f",
    "milli", "\x0f",
    "ether", "\x12",
    "eth", "\x12",
    "kether", "\x15",
    "grand", "\x15",
    "mether", "\x18",
    "gether", "\x1b",
    "tether", "\x1e",
    NULL};

int string_val_to_bytes(char* val, char* unit, bytes32_t target) {
  if (!val) return IN3_EINVAL;
  int l = strlen(val), nl = l, exp = 0, p = 0;

  if (l == 1 && val[0] == '0') {
    *target = 0;
    return 1;
  }
  if (val[0] == '0' && val[1] == 'x') return unit == NULL ? hex_to_bytes(val + 2, l - 2, target, l) : IN3_EINVAL;
  if (unit == NULL) {
    while (nl && val[nl - 1] > '9') nl--;
    if (nl < l) unit = val + nl;
  }
  if (unit) {
    for (int i = 0; UNITS[i]; i += 2) {
      if (strcmp(UNITS[i], unit) == 0) {
        exp = *UNITS[i + 1];
        break;
      }
      else if (!UNITS[i + 2]) {
        if (unit[0] == 'e' && unit[1] >= '0' && unit[1] <= '9')
          exp = atoi(unit + 1);
        else
          return IN3_EINVAL;
      }
    }
  }
  if (!exp && l < 20) {
    bytes_t b = bytes(target, 8);
    long_to_bytes(atoll(nl < l ? strncpy(alloca(nl + 1), val, nl) : val), target);
    b_optimize_len(&b);
    if (b.len < 8) memmove(target, b.data, b.len);
    return (int) b.len;
  }
#ifdef ETH_FULL
  char* dst = alloca(l + exp + 10);
  char* dot = strchr(val, '.');
  if (!dot)
    memcpy(dst, val, (p = nl) + 1);
  else if (dot - val != 1 || *val != '0')
    memcpy(dst, val, (p = dot - val) + 1);
  dst[p + exp] = 0;
  if (exp) {
    if (dot) {
      dot++;
      for (int i = 0; i < exp; i++) {
        if (*dot && dot - val < nl) {
          dst[p + i] = *dot;
          dot++;
        }
        else
          dst[p + i] = '0';
      }
    }
    else
      memset(dst + p, '0', exp);
  }
  // remove leading zeros
  while (*dst == '0' && dst[1]) dst++;
  size_t s;
  mp_int d;
  mp_init(&d);
  mp_read_radix(&d, dst, 10);
  mp_export(target, &s, 1, sizeof(uint8_t), 1, 0, &d);
  mp_clear(&d);
  return (int) s;
#else
  UNUSED_VAR(p);
  bytes_t b = bytes(target, 8);
  long_to_bytes(parse_float_val(nl < l ? strncpy(alloca(nl + 1), val, nl) : val, exp), target);
  b_optimize_len(&b);
  if (b.len < 8) memmove(target, b.data, b.len);
  return (int) b.len;
#endif
}

static in3_ret_t in3_toWei(in3_rpc_handle_ctx_t* ctx) {
  if (!ctx->params || d_len(ctx->params) != 2 || d_type(ctx->params + 2) != T_STRING) return req_set_error(ctx->req, "must have 2 params as strings", IN3_EINVAL);
  char* val = d_get_string_at(ctx->params, 0);
  if (!val) {
    if (d_type(ctx->params + 1) == T_INTEGER) {
      val = alloca(20);
      sprintf(val, "%i", d_int(ctx->params + 1));
    }
    else
      return req_set_error(ctx->req, "the value must be a string", IN3_EINVAL);
  }
  bytes32_t tmp;
  int       s = string_val_to_bytes(val, d_get_string_at(ctx->params, 1), tmp);
  return s < 0
             ? req_set_error(ctx->req, "invalid number string", IN3_EINVAL)
             : in3_rpc_handle_with_bytes(ctx, bytes(tmp, (uint32_t) s));
}

char* bytes_to_string_val(bytes_t wei, int exp, int digits) {
  char      tmp[300];
  bytes32_t val = {0};
  memcpy(val + 32 - wei.len, wei.data, wei.len);
  bignum256 bn;
  bn_read_be(val, &bn);
  size_t l = bn_format(&bn, "", "", 0, 0, false, tmp, 300);
  if (exp) {
    if (l <= (size_t) exp) {
      memmove(tmp + exp - l + 1, tmp, l + 1);
      memset(tmp, '0', exp - l + 1);
      l += exp - l + 1;
    }
    memmove(tmp + l - exp + 1, tmp + l - exp, exp + 1);
    tmp[l - exp] = '.';
    l++;
  }
  if (digits == -1 && exp)
    for (int i = l - 1;; i--) {
      if (tmp[i] == '0')
        tmp[i] = 0;
      else if (tmp[i] == '.') {
        tmp[i] = 0;
        break;
      }
      else
        break;
    }
  else if (digits == 0)
    tmp[l - exp + digits - 1] = 0;
  else if (digits < exp)
    tmp[l - exp + digits] = 0;

  return _strdupn(tmp, -1);
}

static in3_ret_t in3_fromWei(in3_rpc_handle_ctx_t* ctx) {
  if (!ctx->params || d_len(ctx->params) < 1) return req_set_error(ctx->req, "must have 1 params as number or bytes", IN3_EINVAL);
  bytes_t    val  = d_to_bytes(ctx->params + 1);
  d_token_t* unit = d_get_at(ctx->params, 1);
  int        exp  = 0;
  if (d_type(unit) == T_STRING) {
    char* u = d_string(unit);
    for (int i = 0; UNITS[i]; i += 2) {
      if (strcmp(UNITS[i], u) == 0) {
        exp = *UNITS[i + 1];
        break;
      }
      else if (!UNITS[i + 2])
        return req_set_error(ctx->req, "the unit can not be found", IN3_EINVAL);
    }
  }
  else if (d_type(unit) == T_INTEGER)
    exp = d_int(unit);
  else
    return req_set_error(ctx->req, "the unit must be eth-unit or a exponent", IN3_EINVAL);

  int       digits = (unit = d_get_at(ctx->params, 2)) ? d_int(unit) : -1;
  char*     s      = bytes_to_string_val(val, exp, digits);
  in3_ret_t r      = in3_rpc_handle_with_string(ctx, s);
  _free(s);
  return r;
}

static in3_ret_t in3_pk2address(in3_rpc_handle_ctx_t* ctx) {
  bytes_t* pk = d_get_bytes_at(ctx->params, 0);
  if (!pk || pk->len != 32 || d_len(ctx->params) != 1) return req_set_error(ctx->req, "Invalid private key! must be 32 bytes long", IN3_EINVAL);

  uint8_t public_key[65], sdata[32];
  ecdsa_get_public_key65(&secp256k1, pk->data, public_key);

  if (strcmp(ctx->method, "in3_pk2address") == 0) {
    keccak(bytes(public_key + 1, 64), sdata);
    return in3_rpc_handle_with_bytes(ctx, bytes(sdata + 12, 20));
  }
  else
    return in3_rpc_handle_with_bytes(ctx, bytes(public_key + 1, 64));
}

static in3_ret_t parse_tx_param(in3_rpc_handle_ctx_t* ctx, char* params, sb_t* fn_args, sb_t* fn_sig, sb_t* sb) {
  while (*params) {
    char* eq = strchr(params, '=');
    if (!eq || eq == params) return req_set_error(ctx->req, "invalid params, missing =", IN3_EINVAL);
    char* n = strchr(params, '&');
    if (n == NULL) n = params + strlen(params);
    if (n <= eq) return req_set_error(ctx->req, "invalid params, missing value", IN3_EINVAL);
    if ((eq - params) > 30) return req_set_error(ctx->req, "invalid params, name too long", IN3_EINVAL);
    char* name  = params;
    char* value = eq + 1;
    *eq         = 0;
    params      = *n ? n + 1 : n;
    *n          = 0;
    if (strcmp(name, "value") == 0) {
      bytes32_t tmp = {0};
      string_val_to_bytes(value, NULL, tmp);
      bytes_t b = bytes(tmp, 32);
      b_optimize_len(&b);
      sb_add_bytes(sb, ",\"value\":", &b, 1, false);
    }
    else if (strcmp(name, "gas") == 0 || strcmp(name, "gasLimit") == 0) {
      sb_add_chars(sb, ",\"gas\":");
      sb_print(sb, (value[0] == '0' && value[1] == 'x') ? "\"%s\"" : "%s", value);
    }
    else if (strcmp(name, "gasPrice") == 0) {
      sb_add_chars(sb, ",\"gasPrice\":");
      sb_print(sb, (value[0] == '0' && value[1] == 'x') ? "\"%s\"" : "%s", value);
    }
    else if (strcmp(name, "data") == 0) {
      sb_add_chars(sb, ",\"data\":");
      sb_print(sb, (value[0] == '0' && value[1] == 'x') ? "\"%s\"" : "%s", value);
    }
    else if (strcmp(name, "data") == 0) {
      sb_add_chars(sb, ",\"data\":");
      sb_print(sb, (value[0] == '0' && value[1] == 'x') ? "\"%s\"" : "%s", value);
    }
    else if (strcmp(name, "from") == 0) {
      sb_add_chars(sb, ",\"from\":");
      sb_print(sb, (value[0] == '0' && value[1] == 'x') ? "\"%s\"" : "%s", value);
    }
    else if (strcmp(name, "nonce") == 0) {
      sb_add_chars(sb, ",\"nonce\":");
      sb_print(sb, (value[0] == '0' && value[1] == 'x') ? "\"%s\"" : "%s", value);
    }
    else if (strcmp(name, "layer") == 0) {
      sb_print(sb, ",\"layer\":\"%s\"", value);
    }
    else {
      if (fn_args->len) {
        sb_add_char(fn_args, ',');
        sb_add_char(fn_sig, ',');
      }
      sb_add_chars(fn_sig, name);
      sb_print(fn_args, *name == 's' || (value[0] == '0' && value[1] == 'x') ? "\"%s\"" : "%s", value);
    }
  }

  if (fn_sig->data) {
    sb_add_chars(sb, ",\"fn_sig\":\"");
    sb_add_chars(sb, fn_sig->data);
    sb_add_chars(sb, ")\",\"fn_args\":[");
    if (fn_args->data) sb_add_chars(sb, fn_args->data);
    sb_add_chars(sb, "]");
  }
  sb_add_chars(sb, "}");

  return IN3_OK;
}

static in3_ret_t parse_tx_url(in3_rpc_handle_ctx_t* ctx) {
  char*      aurl      = NULL;
  d_token_t* url_token = d_get_at(ctx->params, 0);
  if (d_type(url_token) != T_STRING) return req_set_error(ctx->req, "missing the url as first arg'", IN3_EINVAL);
  char* url = d_string(url_token);
  if (strncmp(url, "ethereum:", 9)) return req_set_error(ctx->req, "URL must start with 'ethereum:'", IN3_EINVAL);
  url += 9;
  sb_t  sb     = {0};
  int   l      = strlen(url);
  char* s1     = strchr(url, '/');
  char* q      = strchr(url, '?');
  int   to_len = s1 ? s1 - url : (q ? q - url : l);
  if (to_len == 42 && url[0] == '0' && url[1] == 'x') {
    sb_add_chars(&sb, "{\"to\":\"");
    sb_add_range(&sb, url, 0, 42);
    sb_add_chars(&sb, "\"");
  }
  else
    return req_set_error(ctx->req, "invalid address in url. Currently ENS names are notsupported yet!", IN3_EINVAL);
  url          = _strdupn(url + to_len, -1);
  q            = strchr(url, '?');
  aurl         = url;
  sb_t fn_args = {0};
  sb_t fn_sig  = {0};

  if (*url == '/') {
    l = q > url ? (int) (q - url) : (int) (strlen(url));
    if (l) {
      sb_add_range(&fn_sig, url, 1, l - 1);
      sb_add_char(&fn_sig, '(');
    }
    url += l + (q ? 1 : 0);
  }
  in3_ret_t res = IN3_OK;
  TRY_GOTO(parse_tx_param(ctx, url, &fn_args, &fn_sig, &sb))
  sb_add_chars(in3_rpc_handle_start(ctx), sb.data);
  res = in3_rpc_handle_finish(ctx);

clean:
  _free(sb.data);
  _free(fn_args.data);
  _free(fn_sig.data);
  _free(aurl);
  return res;
}

static in3_ret_t in3_calcDeployAddress(in3_rpc_handle_ctx_t* ctx) {
  bytes_t sender = bytes(NULL, 20), nonce = {0};
  TRY_PARAM_GET_REQUIRED_ADDRESS(sender.data, ctx, 0);
  TRY_PARAM_GET_BYTES(nonce, ctx, 1, 0, 0);
  if (!nonce.data) {
    d_token_t* result;
    TRY_SUB_REQUEST(ctx->req, "eth_getTransactionCount", &result, "\"%B\",\"latest\"", sender)
    nonce = d_to_bytes(result);
  }

  // handle nonce as number, which means no leading zeros and if 0 it should be an empty bytes-array
  b_optimize_len(&nonce);
  if (nonce.len == 1 && nonce.data[0] == 0) nonce.len = 0;

  bytes_builder_t* bb = bb_new();
  rlp_encode_item(bb, &sender);
  rlp_encode_item(bb, &nonce);
  rlp_encode_to_list(bb);
  bytes32_t hash;
  keccak(bb->b, hash);
  bb_free(bb);

  return in3_rpc_handle_with_bytes(ctx, bytes(hash + 12, 20));
}

static in3_ret_t in3_ecrecover(in3_rpc_handle_ctx_t* ctx) {
  bytes_t msg, signature;
  char*   sig_type;
  TRY_PARAM_GET_REQUIRED_BYTES(msg, ctx, 0, 0, 0)
  TRY_PARAM_GET_REQUIRED_BYTES(signature, ctx, 1, 65, 65)
  TRY_PARAM_GET_STRING(sig_type, ctx, 2, "raw")

  bytes32_t hash;
  uint8_t   pub[65];
  bytes_t   pubkey_bytes = {.len = 64, .data = ((uint8_t*) &pub) + 1};
  if (strcmp(sig_type, "eth_sign") == 0) {
    char*     tmp = alloca(msg.len + 30);
    const int l   = sprintf(tmp, ETH_SIGN_PREFIX, msg.len);
    memcpy(tmp + l, msg.data, msg.len);
    msg.data = (uint8_t*) tmp;
    msg.len += l;
  }
  if (strcmp(sig_type, "hash") == 0) {
    if (msg.len != 32) return req_set_error(ctx->req, "The message hash must be 32 byte", IN3_EINVAL);
    memcpy(hash, msg.data, 32);
  }
  else
    keccak(msg, hash);

  if (ecdsa_recover_pub_from_sig(&secp256k1, pub, signature.data, hash, signature.data[64] >= 27 ? signature.data[64] - 27 : signature.data[64]))
    return req_set_error(ctx->req, "Invalid Signature", IN3_EINVAL);

  // hash the pubkey
  keccak(pubkey_bytes, hash);
  sb_printx(in3_rpc_handle_start(ctx), "{\"publicKey\":\"%B\",\"address\":\"%B\"}", pubkey_bytes, bytes(hash + 12, 20));
  return in3_rpc_handle_finish(ctx);
}

static in3_ret_t in3_sign_data(in3_rpc_handle_ctx_t* ctx) {
  const bool is_eth_sign = strcmp(ctx->method, "eth_sign") == 0;
  bytes_t    data, signer = NULL_BYTES;
  char*      sig_type;
  //  const bytes_t* pk          = &signer;

  TRY_PARAM_GET_REQUIRED_BYTES(data, ctx, (is_eth_sign ? 1 : 0), 1, 0)
  TRY_PARAM_GET_BYTES(signer, ctx, (is_eth_sign ? 0 : 1), 20, 0)
  TRY_PARAM_GET_STRING(sig_type, ctx, 2, (is_eth_sign ? "eth_sign" : "raw"))

  if (strcmp(sig_type, "eth_sign") == 0) {
    char*     tmp = alloca(data.len + 30);
    const int l   = sprintf(tmp, ETH_SIGN_PREFIX, data.len);
    memcpy(tmp + l, data.data, data.len);
    data.data = (uint8_t*) tmp;
    data.len += l;
    sig_type = "raw";
  }

  in3_sign_ctx_t sc = {0};
  sc.req            = ctx->req;
  sc.message        = data;
  sc.account        = signer;
  sc.type           = strcmp(sig_type, "hash") == 0 ? SIGN_EC_RAW : SIGN_EC_HASH;
  if (strcmp(sig_type, "sign_ec_hash") == 0) sc.type = SIGN_EC_HASH;
  if (strcmp(sig_type, "sign_ec_raw") == 0) sc.type = SIGN_EC_RAW;
  if (strcmp(sig_type, "sign_ec_prefix") == 0) sc.type = SIGN_EC_PREFIX;
  if (strcmp(sig_type, "sign_ec_btc") == 0) sc.type = SIGN_EC_BTC;

  if ((sc.account.len == 20 || sc.account.len == 0) && in3_plugin_is_registered(ctx->req->client, PLGN_ACT_SIGN)) {
    TRY(in3_plugin_execute_first(ctx->req, PLGN_ACT_SIGN, &sc));
  }
  else if (sc.account.len == 32) {
    sc.signature = sign_with_pk(signer.data, data, sc.type);
    if (!sc.signature.data) return req_set_error(ctx->req, "unsupported sigType", IN3_EINVAL);
  }
  else
    return req_set_error(ctx->req, "Invalid private key! Must be either an address(20 byte) or an raw private key (32 byte)", IN3_EINVAL);

  bytes_t sig_bytes = sc.signature;

  // we only correct the v value, if the sig_type is a simple type. if it is a sign_ec- type, we don't
  if (strncmp(sig_type, "sign_ec_", 8) && sc.signature.len == 65 && sc.signature.data[64] < 2)
    sc.signature.data[64] += 27;

  sb_t* sb = in3_rpc_handle_start(ctx);
  if (is_eth_sign)
    sb_printx(sb, "\"%B\"", sig_bytes);
  else {
    sb_printx(sb, "{\"message\":\"%B\",", data);

    if (strcmp(sig_type, "raw") == 0) {
      bytes32_t hash_val;
      keccak(data, hash_val);
      sb_printx(sb, "\"messageHash\":\"%B\",", bytes(hash_val, 32));
    }
    else
      sb_printx(sb, "\"messageHash\":\"%B\",", data);
    int64_t v = sc.signature.data[64];
    sb_printx(sb, "\"signature\":\"%B\",\"r\":\"%B\",\"s\":\"%B\",\"v\":%d}",
              sig_bytes, bytes(sc.signature.data, 32), bytes(sc.signature.data + 32, 32), v);
  }

  _free(sc.signature.data);
  return in3_rpc_handle_finish(ctx);
}

static in3_ret_t in3_decryptKey(in3_rpc_handle_ctx_t* ctx) {
  d_token_t*  keyfile        = d_get_at(ctx->params, 0);
  bytes_t     password_bytes = d_to_bytes(d_get_at(ctx->params, 1));
  bytes32_t   dst;
  json_ctx_t* sctx = NULL;

  if (!password_bytes.data) return req_set_error(ctx->req, "you need to specify a passphrase", IN3_EINVAL);
  if (d_type(keyfile) == T_STRING) {
    sctx = parse_json(d_string(keyfile));
    if (!sctx) return req_set_error(ctx->req, "invalid keystore-json", IN3_EINVAL);
    keyfile = sctx->result;
  }

  if (!keyfile || d_type(keyfile) != T_OBJECT) {
    if (sctx) json_free(sctx);
    return req_set_error(ctx->req, "no valid key given", IN3_EINVAL);
  }
  char* passphrase = alloca(password_bytes.len + 1);
  memcpy(passphrase, password_bytes.data, password_bytes.len);
  passphrase[password_bytes.len] = 0;
  in3_ret_t res                  = decrypt_key(keyfile, passphrase, dst);
  if (sctx) json_free(sctx);
  if (res) return req_set_error(ctx->req, "Invalid key", res);
  return in3_rpc_handle_with_bytes(ctx, bytes(dst, 32));
}

static in3_ret_t in3_prepareTx(in3_rpc_handle_ctx_t* ctx) {
  d_token_t* tx;
  TRY_PARAM_GET_REQUIRED_OBJECT(tx, ctx, 0);
  bytes_t dst = {0};
  sb_t    sb  = {0};
#if defined(ETH_BASIC) || defined(ETH_FULL)
  bool write_debug = (d_len(ctx->params) == 2 && d_get_int_at(ctx->params, 1));
  if (write_debug) sb_add_char(&sb, '{');
  TRY_CATCH(eth_prepare_unsigned_tx(tx, ctx->req, &dst, write_debug ? &sb : NULL), _free(sb.data))
#else
  if (ctx->params || tx || ctx) return req_set_error(ctx->req, "eth_basic is needed in order to use eth_prepareTx", IN3_EINVAL);
#endif
  if (sb.data) {
    sb_add_chars(&sb, ",\"state\":\"unsigned\"}");
    in3_rpc_handle_with_string(ctx, sb.data);
  }
  else
    in3_rpc_handle_with_bytes(ctx, dst);
  _free(dst.data);
  _free(sb.data);
  return IN3_OK;
}

static in3_ret_t in3_signTx(in3_rpc_handle_ctx_t* ctx) {
  CHECK_PARAMS_LEN(ctx->req, ctx->params, 1)
  d_token_t* tx_data = ctx->params + 1;
  bytes_t    tx_raw  = NULL_BYTES;
  bytes_t*   from_b  = NULL;
  bytes_t*   data    = NULL;
  if (strcmp(ctx->method, "eth_signTransaction") == 0 || d_type(tx_data) == T_OBJECT) {
#if defined(ETH_BASIC) || defined(ETH_FULL)
    TRY(eth_prepare_unsigned_tx(tx_data, ctx->req, &tx_raw, NULL))
    from_b = d_get_bytes(tx_data, K_FROM);
    data   = &tx_raw;
#else
    return req_set_error(ctx->req, "eth_basic is needed in order to use eth_prepareTx", IN3_EINVAL);
#endif
  }
  else {
    data   = d_get_bytes_at(ctx->params, 0);
    from_b = d_get_bytes_at(ctx->params, 1);
  }

  address_t from;
  memset(from, 0, 20);
  if (from_b && from_b->data && from_b->len == 20) memcpy(from, from_b->data, 20);
  bytes_t dst = {0};
#if defined(ETH_BASIC) || defined(ETH_FULL)
  TRY_FINAL(eth_sign_raw_tx(*data, ctx->req, from, &dst), _free(tx_raw.data))
#else
  _free(tx_raw.data);
  if (data || ctx || from[0] || ctx->params) return req_set_error(ctx->req, "eth_basic is needed in order to use eth_signTx", IN3_EINVAL);
#endif
  in3_rpc_handle_with_bytes(ctx, dst);
  _free(dst.data);
  return IN3_OK;
}

static in3_ret_t handle_intern(void* pdata, in3_plugin_act_t action, void* plugin_ctx) {
  UNUSED_VAR(pdata);
  UNUSED_VAR(action);

  in3_rpc_handle_ctx_t* ctx = plugin_ctx;
#if !defined(RPC_ONLY) || defined(RPC_ETH_SIGN)
  TRY_RPC("eth_sign", in3_sign_data(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_ETH_SIGNTRANSACTION)
  TRY_RPC("eth_signTransaction", in3_signTx(ctx))
#endif

  if (strncmp(ctx->method, "in3_", 4)) return IN3_EIGNORE; // shortcut

#if !defined(RPC_ONLY) || defined(RPC_IN3_ABIENCODE)
  TRY_RPC("in3_abiEncode", in3_abiEncode(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_ABIDECODE)
  TRY_RPC("in3_abiDecode", in3_abiDecode(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_RLPDECODE)
  TRY_RPC("in3_rlpDecode", in3_rlpDecode(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_DECODETX)
  TRY_RPC("in3_decodeTx", in3_decodeTx(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_CHECKSUMADDRESS)
  TRY_RPC("in3_checksumAddress", in3_checkSumAddress(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_ENS)
  TRY_RPC("in3_ens", in3_ens(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_TOWEI)
  TRY_RPC("in3_toWei", in3_toWei(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_FROMWEI)
  TRY_RPC("in3_fromWei", in3_fromWei(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_PK2ADDRESS)
  TRY_RPC("in3_pk2address", in3_pk2address(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_PK2PUBLIC)
  TRY_RPC("in3_pk2public", in3_pk2address(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_ECRECOVER)
  TRY_RPC("in3_ecrecover", in3_ecrecover(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_SIGNDATA)
  TRY_RPC("in3_signData", in3_sign_data(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_DECRYPTKEY)
  TRY_RPC("in3_decryptKey", in3_decryptKey(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_PREPARETX)
  TRY_RPC("in3_prepareTx", in3_prepareTx(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_SIGNTX)
  TRY_RPC("in3_signTx", in3_signTx(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_CALCDEPLOYADDRESS)
  TRY_RPC("in3_calcDeployAddress", in3_calcDeployAddress(ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_IN3_PARSE_TX_URL)
  TRY_RPC("in3_parse_tx_url", parse_tx_url(ctx))
#endif

  return IN3_EIGNORE;
}

in3_ret_t in3_register_eth_api(in3_t* c) {
  return in3_plugin_register(c, PLGN_ACT_RPC_HANDLE, handle_intern, NULL, false);
}
