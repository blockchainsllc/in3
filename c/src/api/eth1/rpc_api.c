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
#include "../../signer/pk-signer/signer.h"
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
  bytes_t    data      = {0};                      // resulting data
  char*      error     = NULL;                     // error message
  d_token_t* arguments = d_get_at(ctx->params, 1); // the array of arguments
  char*      method_sig;                           // method signature

  // get and check arguments
  TRY_PARAM_GET_REQUIRED_STRING(method_sig, ctx, 0)
  if (!arguments) return req_set_error(ctx->req, "missing values", IN3_EINVAL);

  // encode
  abi_sig_t* abi_signature = abi_sig_create(method_sig, &error);   // parse the signature
  if (!error) data = abi_encode(abi_signature, arguments, &error); // encode the arguments

  // create response
  if (!error) in3_rpc_handle_with_bytes(ctx, data); // create response

  // cleanup
  if (abi_signature) abi_sig_free(abi_signature); // cleanup
  _free(data.data);                               // free encoded data
  return error ? req_set_error(ctx->req, error, IN3_EINVAL) : IN3_OK;
}

static in3_ret_t in3_abiDecode(in3_rpc_handle_ctx_t* ctx) {
  char*       method_sig;    // method signature
  char*       error  = NULL; // error message
  json_ctx_t* result = NULL; // decoded data
  bytes_t     data, topics;  // input data

  // get and check arguments
  TRY_PARAM_GET_REQUIRED_STRING(method_sig, ctx, 0)
  TRY_PARAM_GET_REQUIRED_BYTES(data, ctx, 1, 0, 0)
  TRY_PARAM_GET_BYTES(topics, ctx, 2, 0, 0)
  CHECK_PARAM(ctx->req, ctx->params, 1, d_bytes(val).len % 32 == 0)

  // decode
  abi_sig_t* abi_signature = abi_sig_create(method_sig, &error);
  if (!error) result = topics.data ? abi_decode_event(abi_signature, topics, data, &error) : abi_decode(abi_signature, data, &error);

  // clean up
  if (abi_signature) abi_sig_free(abi_signature);
  if (error) return req_set_error(ctx->req, error, IN3_EINVAL);

  // create response
  char* json = d_create_json(result, result->result);
  in3_rpc_handle_with_string(ctx, json);
  _free(json);
  if (result) json_free(result);
  return IN3_OK;
}

// recursive function decoding and writing the result
static in3_ret_t rlp_decode_data(sb_t* sb, bytes_t data, int index) {
  bytes_t dst = {0}; // the item data

  // decode
  switch (rlp_decode(&data, index, &dst)) {
    case 1: // data item
      if (index) sb_add_char(sb, ',');
      sb_add_bytes(sb, "", &dst, 1, false);
      return IN3_OK;

    case 2: // list
      if (index) sb_add_char(sb, ',');
      sb_add_char(sb, '[');
      data = dst;
      for (int i = 0; rlp_decode_data(sb, data, i) == IN3_OK; i++) {}
      sb_add_char(sb, ']');
      return IN3_OK;

    default:
      return IN3_ELIMIT;
  }
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
  bytes_t      data = {0}, val;        // rlp decoded data
  bytes32_t    hash;                   // tx hash
  sb_t         response = {0};         // temp json-result buffer
  uint32_t     type     = 0;           // tx type
  const char** fields   = TX_FIELDS_0; // field names depending on the type

  // we only require bytes as input
  TRY_PARAM_GET_REQUIRED_BYTES(data, ctx, 0, 1, 0)

  // create the transactionhash
  keccak(data, hash);

  // deterine the tx type.
  if (data.data[0] < 0xc0) {                                                    // first byte of a legacy (type=0) tx will always be higher then 0c0, since it would be a list
    type   = data.data[0];                                                      // after EIP-1559 the type is the first byte
    fields = type == 1 ? TX_FIELDS_1 : TX_FIELDS_2;                             // assign the fields depending on the type
    if (type > 2) return req_set_error(ctx->req, "Invalid TxType", IN3_EINVAL); // currently there is only 0,1,2 as supported type
    data.len--;                                                                 // we are removing the first byte, since the rest is just rlp-endocded data
    data.data++;
  }

  // rlp decode the data
  if (rlp_decode(&data, 0, &data) != 2)                                            // data is encoded as list,
    return req_set_error(ctx->req, "Invalid Tx-Data, must be a list", IN3_EINVAL); // which we need to decode first
  int len = rlp_decode_len(&data);                                                 // now we can count the elements in the tx

  sb_printx(&response, "{\"type\":%d,\"hash\":\"%B\"", type, bytes(hash, 32));

  // loop throught the fields
  for (int i = 0; fields[i] && i < len; i++) {
    int field_type = rlp_decode(&data, i, &val);

    // handle the fields. accessList is handled differently
    if (strcmp(fields[i], "accessList") == 0 && field_type == 2) {
      sb_printx(&response, ",\"%s\":[", fields[i]);
      bytes_t account, addr, storage;

      // for each account...
      for (int a = 0;; a++) {
        field_type = rlp_decode(&val, a, &account);
        if (!field_type) break;
        if (field_type != 2 || rlp_decode(&account, 0, &addr) != 1 || rlp_decode(&account, 1, &storage) != 2) {
          _free(response.data);
          return req_set_error(ctx->req, "Invalid Tx-Data, wrong accessList account", IN3_EINVAL);
        }
        if (a) sb_add_char(&response, ',');
        sb_printx(&response, "{\"address\":\"%B\",\"storageKeys\":[", addr);

        // for each storage key
        for (int b = 0;; b++) {
          field_type = rlp_decode(&storage, b, &addr);
          if (!field_type) break;
          if (field_type != 1) {
            _free(response.data);
            return req_set_error(ctx->req, "Invalid Tx-Data, wrong accessList sotrage key", IN3_EINVAL);
          }
          if (b) sb_add_char(&response, ',');
          sb_printx(&response, "\"%B\"", addr);
        }
        sb_add_chars(&response, "]}");
      }
      sb_add_chars(&response, "]");
    }
    // for to, r,s,v and data we treat them as full bytes
    else if ((strlen(fields[i]) < 3 || strcmp(fields[i], "data") == 0) && field_type == 1)
      sb_printx(&response, ",\"%s\":\"%B\"", fields[i], val);
    // for other fields, we treat them as number ignoring the leading zeros
    else if (field_type == 1)
      sb_printx(&response, ",\"%s\":\"%V\"", fields[i], val);
    // if we end up here something went wrong
    else {
      _free(response.data);
      return req_set_error(ctx->req, "Invalid Tx-Data, wrong item", IN3_EINVAL);
    }
  }

  // determine from-address, but only if we have all the fields or a signed tx
  if (len && !fields[len]) {
    uint8_t pub[65];                                  // pub key-data
    bytes_t last, v;                                  // bytes structs
    rlp_decode(&data, len - 1, &v);                   // get the s-field,
    if (v.len == 0) {                                 // and check if this is empty, since it means it is a raw tx without a signature
      rlp_decode(&data, len - 3, &v);                 // but in this case the v-value
      sb_printx(&response, ",\"chainId\":\"%V\"", v); // is the chain_id
    }
    else {

      // create the unsigned raw tx in order to recover
      rlp_decode(&data, len - 3, &v);                                       // get the v-value
      rlp_decode(&data, len - 4, &last);                                    // and the last element before the signature
      uint8_t          r   = v.len ? v.data[v.len - 1] : 0;                 // we also need the recovery-byte, which we find in v
      uint8_t          tt  = (uint8_t) type;                                // and later we need the type as uint8 in order to insert it.
      bytes_builder_t* rlp = bb_newl(data.len);                             //
      bb_write_raw_bytes(rlp, data.data, last.data + last.len - data.data); // copy the existing data without signature
      if (type == 0 && (v.len > 1 || (v.len == 1 && r > 28))) {             // if v contains a the chain_id, we need to decode it
        int c = bytes_to_int(v.data, v.len);
        r     = 1 - c % 2;
        c     = (c - (36 - c % 2)) / 2;
        uint8_t tmp[4];                                  // we put it in the tmp,
        int_to_bytes((uint32_t) c, tmp);                 // so we can create the bytes needed for the unsgined tx
        bytes_t bb = bytes(tmp, 4);                      // assign the bytes
        b_optimize_len(&bb);                             // and remove the leading zeros
        sb_printx(&response, ",\"chainId\":\"%V\"", bb); // this is for the output
        rlp_encode_item(rlp, &bb);                       // but now we add the chain_id
        bb.len = 0;                                      // clear it
        rlp_encode_item(rlp, &bb);                       // and add a empty r
        rlp_encode_item(rlp, &bb);                       // and an empty s
      }
      else if (type == 0 && r > 26) // this is for the Legacy-transaction
        r -= 27;                    // adding 27 to the recovery-byte,

      rlp_encode_to_list(rlp);                              // we convert the rlp-data ( without the signature ) to a list
      if (type) bb_replace(rlp, 0, 0, &tt, 1);              // we insert the type (execpt legacy tx)
      sb_printx(&response, ",\"unsigned\":\"%B\"", rlp->b); // ad it tot eh output
      keccak(rlp->b, hash);                                 // calculate the hash used for the signature
      bb_free(rlp);                                         // done with the unsiged data
      uint8_t signature[65] = {0};                          // prepare the 65 bytes signature
      rlp_decode(&data, len - 2, &v);                       // by copying the
      memcpy(signature + 32 - v.len, v.data, v.len);        // r
      rlp_decode(&data, len - 1, &v);                       // and the
      memcpy(signature + 64 - v.len, v.data, v.len);        // s
      signature[64] = r;                                    // and the recovery-byte
      sb_printx(&response, ",\"signature\":\"%B\"", bytes(signature, 65));

      // now we recover. returning a none zero value means an invalid signature
      if (crypto_recover(ECDSA_SECP256K1, bytes(hash, 32), bytes(signature, 65), pub)) {
        _free(response.data);
        return req_set_error(ctx->req, "Invalid Signature", IN3_EINVAL);
      }

      // calculate the address from the pubkey
      keccak(bytes(pub, 64), hash);                                                                        //  by hashing it
      sb_printx(&response, ",\"publicKey\":\"%B\",\"from\":\"%B\"", bytes(pub, 64), bytes(hash + 12, 20)); // and taking the last 20 bytes
    }
  }

  // finish up
  sb_add_char(&response, '}');
  in3_rpc_handle_with_string(ctx, response.data);
  _free(response.data);
  return IN3_OK;
}

static in3_ret_t in3_checkSumAddress(in3_rpc_handle_ctx_t* ctx) {
  uint8_t* src;          // input address
  bool     use_chain_id; // encode chain_id

  // fetch arguments
  TRY_PARAM_GET_REQUIRED_ADDRESS(src, ctx, 0)
  TRY_PARAM_GET_BOOL(use_chain_id, ctx, 1, 0)

  // encode
  char      result[45];
  in3_ret_t res = to_checksum(src, use_chain_id ? in3_chain_id(ctx->req) : 0, result + 1);
  if (res) return req_set_error(ctx->req, "Could not create the checksum address", res);
  result[0]  = '"';
  result[43] = '"';
  result[44] = 0;

  return in3_rpc_handle_with_string(ctx, result);
}

static in3_ret_t in3_ens(in3_rpc_handle_ctx_t* ctx) {
  char *         name, *type;                // input data
  bytes_t        registry = bytes(NULL, 20); // registry address
  int            res_len  = 20;              // len of the result
  in3_ens_type_t ens_type = ENS_ADDR;        // requesting type
  bytes32_t      result;                     // resulting buffer

  // get arguments
  TRY_PARAM_GET_REQUIRED_STRING(name, ctx, 0)
  TRY_PARAM_GET_STRING(type, ctx, 1, "addr")
  TRY_PARAM_GET_ADDRESS(registry.data, ctx, 2, NULL)

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

  // execute
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
  if (!ctx->params || d_len(ctx->params) != 2 || d_type(d_get_at(ctx->params, 1)) != T_STRING) return req_set_error(ctx->req, "must have 2 params as strings", IN3_EINVAL);
  char* val = d_get_string_at(ctx->params, 0);
  if (!val) {
    d_token_t* t = d_get_at(ctx->params, 0);
    if (d_type(t) == T_INTEGER) {
      val = alloca(20);
      sprintf(val, "%i", d_int(t));
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
  char tmp[300];
  int  l = encode(ENC_DECIMAL, wei, tmp);
  if (l < 0)
    sprintf(tmp, "<not supported>");
  else {
    if (exp) {
      if (l <= exp) {
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
  }

  return _strdupn(tmp, -1);
}

static in3_ret_t in3_fromWei(in3_rpc_handle_ctx_t* ctx) {
  if (!ctx->params || d_len(ctx->params) < 1) return req_set_error(ctx->req, "must have 1 params as number or bytes", IN3_EINVAL);
  bytes_t    val  = d_get_bytes_at(ctx->params, 0);
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
  bytes_t   private_key;
  uint8_t   public_key[64];
  bytes32_t hash;

  // fetch arguments
  TRY_PARAM_GET_REQUIRED_BYTES(private_key, ctx, 0, 32, 32);

  // extract public key
  TRY(crypto_convert(ECDSA_SECP256K1, CONV_PK32_TO_PUB64, private_key, public_key, NULL))

  // hash it
  if (strcmp(ctx->method, "in3_pk2address") == 0) {
    keccak(bytes(public_key, 64), hash);
    return in3_rpc_handle_with_bytes(ctx, bytes(hash + 12, 20));
  }
  else
    return in3_rpc_handle_with_bytes(ctx, bytes(public_key, 64));
}

static in3_ret_t in3_calcDeployAddress(in3_rpc_handle_ctx_t* ctx) {
  bytes_t sender = bytes(NULL, 20), nonce = {0};

  // fetch arguments
  TRY_PARAM_GET_REQUIRED_ADDRESS(sender.data, ctx, 0);
  TRY_PARAM_GET_BYTES(nonce, ctx, 1, 0, 0);

  // fetch nonce
  if (!nonce.data) {
    d_token_t* result;
    TRY_SUB_REQUEST(ctx->req, "eth_getTransactionCount", &result, "\"%B\",\"latest\"", sender)
    nonce = d_bytes(result);
  }

  // handle nonce as number, which means no leading zeros and if 0 it should be an empty bytes-array
  b_optimize_len(&nonce);
  if (nonce.len == 1 && nonce.data[0] == 0) nonce.len = 0;

  // encode
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
  bytes32_t hash;
  uint8_t   pub[64];
  bytes_t   msg, signature;
  char*     sig_type;

  // get arguments
  TRY_PARAM_GET_REQUIRED_BYTES(msg, ctx, 0, 0, 0)
  TRY_PARAM_GET_REQUIRED_BYTES(signature, ctx, 1, 65, 65)
  TRY_PARAM_GET_STRING(sig_type, ctx, 2, "raw")

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

  TRY(req_set_error(ctx->req, "Invalid Signature", crypto_recover(ECDSA_SECP256K1, bytes(hash, 32), signature, pub)))

  // hash the pubkey
  keccak(bytes(pub, 64), hash);
  sb_printx(in3_rpc_handle_start(ctx), "{\"publicKey\":\"%B\",\"address\":\"%B\"}", bytes(pub, 64), bytes(hash + 12, 20));
  return in3_rpc_handle_finish(ctx);
}

static in3_ret_t in3_sign_data(in3_rpc_handle_ctx_t* ctx) {
  const bool is_eth_sign = strcmp(ctx->method, "eth_sign") == 0;
  bytes_t    data, signer;
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
  sc.curve_type     = SIGN_CURVE_ECDSA;
  sc.message        = data;
  sc.account        = signer;
  sc.digest_type    = strcmp(sig_type, "hash") == 0 ? SIGN_EC_RAW : SIGN_EC_HASH;
  if (strcmp(sig_type, "sign_ec_hash") == 0) sc.digest_type = SIGN_EC_HASH;
  if (strcmp(sig_type, "sign_ec_raw") == 0) sc.digest_type = SIGN_EC_RAW;
  if (strcmp(sig_type, "sign_ec_prefix") == 0) sc.digest_type = SIGN_EC_PREFIX;
  if (strcmp(sig_type, "sign_ec_btc") == 0) sc.digest_type = SIGN_EC_BTC;
  if (strcmp(sig_type, "sign_ed25519") == 0) sc.curve_type = SIGN_CURVE_ED25519;

  if ((sc.account.len == 20 || sc.account.len == 0) && in3_plugin_is_registered(ctx->req->client, PLGN_ACT_SIGN)) {
    TRY(in3_plugin_execute_first(ctx->req, PLGN_ACT_SIGN, &sc));
  }
  else if (sc.account.len == 32) {
    sc.signature = sign_with_pk(signer.data, data, sc.digest_type);
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
    sb_printx(sb, "\"signature\":\"%B\",\"r\":\"%B\",\"s\":\"%B\",\"v\":%d}",
              sig_bytes, bytes(sc.signature.data, 32), bytes(sc.signature.data + 32, 32), sc.signature.data[64]);
  }

  _free(sc.signature.data);
  return in3_rpc_handle_finish(ctx);
}

static in3_ret_t in3_decryptKey(in3_rpc_handle_ctx_t* ctx) {
  d_token_t*  keyfile        = d_get_at(ctx->params, 0);
  bytes_t     password_bytes = d_bytes(d_get_at(ctx->params, 1));
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
  bytes_t    dst           = {0};
  sb_t       debug_payload = {0};

  // fetch arguments
  TRY_PARAM_GET_REQUIRED_OBJECT(tx, ctx, 0);

#if defined(ETH_BASIC) || defined(ETH_FULL)
  bool write_debug = (d_len(ctx->params) == 2 && d_get_int_at(ctx->params, 1));
  if (write_debug) sb_add_char(&debug_payload, '{');
  TRY_CATCH(eth_prepare_unsigned_tx(tx, ctx->req, &dst, write_debug ? &debug_payload : NULL), _free(debug_payload.data))
#else
  if (ctx->params || tx || ctx) return req_set_error(ctx->req, "eth_basic is needed in order to use eth_prepareTx", IN3_EINVAL);
#endif
  if (debug_payload.data) {
    sb_add_chars(&debug_payload, ",\"state\":\"unsigned\"}");
    in3_rpc_handle_with_string(ctx, debug_payload.data);
  }
  else
    in3_rpc_handle_with_bytes(ctx, dst);
  _free(dst.data);
  _free(debug_payload.data);
  return IN3_OK;
}

static in3_ret_t eth_getInternalTx(in3_rpc_handle_ctx_t* ctx) {
  bytes_t tx_hash;
  TRY_PARAM_GET_REQUIRED_BYTES(tx_hash, ctx, 0, 32, 32)
  d_token_t*  result;
  const char* tracer = "{data:[], fault:function(l) {},step:function(l) { var op = this.ops[l.op.toString()]; if (op)  this.data.push({op:l.op.toString(16), to: op[0]!=-1 ? \"0x\"+l.stack.peek(op[0]).toString(16) : null, value: op[1]!=-1 ? \"0x\"+l.stack.peek(op[1]).toString(): null, from:this.hex(l.contract.getAddress()),depth: l.getDepth(), gas:  op[2]!=-1 ? \"0x\"+l.stack.peek(op[2]).toString(): null }) },result:function(){return this.data},ops:{CALL:[1,2,0],CALLCODE:[1,2,0],DELEGATECALL:[1,-1,0],STATICCALL:[1,-1,0],CREATE:[-1,0,-1],CREATE2:[-1,0,-1],SELFDESTRUCT:[0,-1,-1]},hex:function(_) { var s=\"0x\";for (var i=0;i<_.length;i++) {s+= (\"0\"+_[i].toString(16)).slice(-2)} return s}}";
  char*       params = sprintx("\"%B\",{\"tracer\":\"%S\"}", tx_hash, tracer);
  TRY_FINAL(req_send_sub_request(ctx->req, "debug_traceTransaction", params, NULL, &result, NULL), _free(params))
  sb_printx(in3_rpc_handle_start(ctx), "%j", result);
  return in3_rpc_handle_finish(ctx);
}

static in3_ret_t in3_signTx(in3_rpc_handle_ctx_t* ctx) {
  CHECK_PARAMS_LEN(ctx->req, ctx->params, 1)
  d_token_t* tx_data = d_get_at(ctx->params, 0);
  bytes_t    tx_raw  = NULL_BYTES;
  bytes_t    from_b;
  bytes_t    data;
  if (strcmp(ctx->method, "eth_signTransaction") == 0 || d_type(tx_data) == T_OBJECT) {
#if defined(ETH_BASIC) || defined(ETH_FULL)
    TRY(eth_prepare_unsigned_tx(tx_data, ctx->req, &tx_raw, NULL))
    from_b = d_get_bytes(tx_data, K_FROM);
    data   = tx_raw;
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
  if (from_b.data && from_b.len == 20) memcpy(from, from_b.data, 20);
  bytes_t dst = {0};
#if defined(ETH_BASIC) || defined(ETH_FULL)
  TRY_FINAL(eth_sign_raw_tx(data, ctx->req, from, &dst), _free(tx_raw.data))
#else
  _free(tx_raw.data);
  if (data.data || ctx || from[0] || ctx->params) return req_set_error(ctx->req, "eth_basic is needed in order to use eth_signTx", IN3_EINVAL);
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
#if !defined(RPC_ONLY) || defined(RPC_IN3_PGET_INTERNAL_TX)
  TRY_RPC("in3_get_internal_tx", eth_getInternalTx(ctx))
#endif

  return IN3_EIGNORE;
}

in3_ret_t in3_register_eth_api(in3_t* c) {
  return in3_plugin_register(c, PLGN_ACT_RPC_HANDLE, handle_intern, NULL, false);
}
