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

#include "multisig.h"
#include "../../core/client/client.h"
#include "../../core/client/context.h"
#include "../../core/client/context_internal.h"
#include "../../core/client/keys.h"
#include "../../core/util/mem.h"
#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/secp256k1.h"
#include "../../verifier/eth1/nano/rlp.h"
#include "../../verifier/eth1/nano/serialize.h"

bool ecrecover_sig(bytes32_t hash, uint8_t* sig, address_t result) {

  // check messagehash
  uint8_t pubkey[65], tmp[32];
  bytes_t pubkey_bytes = {.len = 64, .data = ((uint8_t*) &pubkey) + 1};
  int     v            = sig[64];

  // correct v
  if (v >= 27) v -= 27;

  // verify signature
  if (ecdsa_recover_pub_from_sig(&secp256k1, pubkey, sig, hash, v)) return false;
  sha3_to(&pubkey_bytes, tmp);
  memcpy(result, tmp + 12, 20);
  return true;
}

/*
*   rlp_add_bytes(rlp, nonce             , UINT);
  rlp_add_bytes(rlp, gas_price         , UINT);
  rlp_add_bytes(rlp, gas_limit         , UINT);
  rlp_add_bytes(rlp, to                , ADDRESS);
  rlp_add_bytes(rlp, value             , UINT);
  rlp_add_bytes(rlp, data              , BYTES);

 */
/*
*         to: '0x567B4485746CFE0bbd73aa04c818BB42B9A07C17',
        value: "0x00",
        data: '0x2cc9c4650000000000000000000000000000000000000000000000000000000000000001',
        safeTxGas: 400000,
        baseGas: 450000,
        gasPrice: 0,
        gasToken: '0x0000000000000000000000000000000000000000',
        refundReceiver: '0x0000000000000000000000000000000000000000'
*/
typedef struct {
  bytes_t nonce;
  bytes_t to;
  bytes_t value;
  bytes_t data;
  bytes_t gas;
  bytes_t gas_price;
  bytes_t v;
} tx_data_t;

typedef struct {
  uint8_t* address;
  uint8_t  sig[65];
  bytes_t  data;
} sig_data_t;

static in3_ret_t decode_tx(in3_ctx_t* ctx, bytes_t raw, tx_data_t* result) {
  if (rlp_decode_in_list(&raw, 0, &result->nonce) != 1) return ctx_set_error(ctx, "invalid nonce in txdata", IN3_EINVAL);
  if (rlp_decode_in_list(&raw, 1, &result->gas_price) != 1) return ctx_set_error(ctx, "invalid gasprice in txdata", IN3_EINVAL);
  if (rlp_decode_in_list(&raw, 2, &result->gas) != 1) return ctx_set_error(ctx, "invalid gas in txdata", IN3_EINVAL);
  if (rlp_decode_in_list(&raw, 3, &result->to) != 1) return ctx_set_error(ctx, "invalid to in txdata", IN3_EINVAL);
  if (rlp_decode_in_list(&raw, 4, &result->value) != 1) return ctx_set_error(ctx, "invalid value in txdata", IN3_EINVAL);
  if (rlp_decode_in_list(&raw, 5, &result->data) != 1) return ctx_set_error(ctx, "invalid data in txdata", IN3_EINVAL);
  if (rlp_decode_in_list(&raw, 6, &result->v) != 1) return ctx_set_error(ctx, "invalid v in txdata", IN3_EINVAL);
  return IN3_OK;
}

static in3_ret_t call(in3_ctx_t* parent, address_t ms, bytes_t data, bytes_t** result) {
  in3_ctx_t* ctx = parent;
  for (; ctx; ctx = ctx->required) {
    if (strcmp(d_get_stringk(ctx->requests[0], K_METHOD), "eth_call")) continue;
    d_token_t* t = d_get(ctx->requests[0], K_PARAMS);
    if (!t || d_type(t) != T_ARRAY || !d_len(t)) continue;
    t = t + 1;
    if (d_type(t) != T_OBJECT || !d_len(t)) continue;
    bytes_t tx_data = d_to_bytes(d_get(t, K_DATA));
    if (tx_data.len == data.len && memcmp(data.data, tx_data.data, data.len) == 0) break;
  }

  if (ctx)
    switch (in3_ctx_state(ctx)) {
      case CTX_ERROR:
        return ctx_set_error(parent, ctx->error, ctx->verification_state ? ctx->verification_state : IN3_ERPC);
      case CTX_SUCCESS:
        *result = d_get_bytesk(ctx->responses[0], K_RESULT);
        if (!*result) {
          char* s = d_get_stringk(d_get(ctx->responses[0], K_ERROR), K_MESSAGE);
          return ctx_set_error(parent, s ? s : "error executing eth_call", IN3_ERPC);
        }
        return IN3_OK;
      case CTX_WAITING_TO_SEND:
      case CTX_WAITING_FOR_RESPONSE:
        return IN3_WAITING;
    }

  // create the call
  bytes_t tmp = bytes(ms, 20);
  sb_t    sb  = {0};
  sb_add_bytes(&sb, "{\"method\":\"eth_call\",\"params\":[{\"to\":", &tmp, 1, false);
  sb_add_bytes(&sb, ",\"data\":", &data, 1, false);
  sb_add_chars(&sb, "},\"latest\"]}");
  return ctx_add_required(parent, ctx_new(parent->client, sb.data));
}

static void cpy_right(uint8_t* raw, int index, bytes_t data) {
  memcpy(raw + 4 + index * 32 + 32 - data.len, data.data, data.len);
}

static bytes_t create_signatures(sig_data_t* signatures, uint32_t sig_count) {
  bytes_builder_t bb = {.b = bytes(_malloc(sig_count * 65), 0), .bsize = sig_count * 65};
  for (unsigned int i = 0; i < sig_count; i++) bb_write_raw_bytes(&bb, signatures[i].sig, 65);
  for (unsigned int i = 0; i < sig_count; i++) {
    if (signatures[i].data.len) {
      memset(bb.b.data + (i * 65) + 32, 0, 32);               // clear
      int_to_bytes(bb.b.len, bb.b.data + (i * 65) + 32 + 28); // set the offset
      bb_write_fixed_bytes(&bb, &signatures[i].data);         // add the data
    }
  }
  return bb.b;
}

in3_ret_t get_tx_hash(in3_ctx_t* ctx, multisig_t* ms, tx_data_t* tx_data, bytes32_t result, uint64_t nonce) {
  /*
  d8d11f78: getTransactionHash(address,uint256,bytes,uint8,uint256,uint256,uint256,address,address,uint256)
'  address to,'+
    '  uint256 value,'+
    '  bytes memory data,'+
    '  uint8 operation,'+
    '  uint256 safeTxGas,'+
    '  uint256 baseGas,'+
    '  uint256 gasPrice,'+
    '  address gasToken,'+
    '  address refundReceiver,'+
    '  uint256 _nonce'+  * 
  */
  int      offset_data = 4 + 10 * 32;
  int      size        = offset_data + ((tx_data->data.len + 31) / 32 + 1) * 32;
  uint8_t* raw         = alloca(size);
  bytes_t* rpc_result  = NULL;
  memset(raw, 0, size);
  memcpy(raw, "\xd8\xd1\x1f\x78", 4);                                    // get TransactionHash functionhash
  cpy_right(raw, 0, tx_data->to);                                        // to
  cpy_right(raw, 1, tx_data->value);                                     // value
  int_to_bytes(offset_data, raw + 4 + 64 + 28);                          // offset for data bytes
  int_to_bytes(tx_data->data.len, raw + offset_data + 28);               // len for data bytes
  memcpy(raw + offset_data + 32, tx_data->data.data, tx_data->data.len); // copy data
  cpy_right(raw, 4, tx_data->gas);
  cpy_right(raw, 5, tx_data->gas);
  cpy_right(raw, 6, tx_data->gas_price);
  long_to_bytes(nonce, raw + 4 + 9 * 32 + 24);

  TRY(call(ctx, ms->address, bytes(raw, size), &rpc_result))
  if (rpc_result->len != 32) return ctx_set_error(ctx, "invalid getTransactionHash result!", IN3_EINVAL);
  memcpy(result, rpc_result->data, 32);
  return IN3_OK;
}

static bytes_t get_exec_tx_data(tx_data_t* tx_data, sig_data_t* signatures, uint32_t sig_count) {
  bytes_t sig_data = create_signatures(signatures, sig_count);
  // 6a761202: execTransaction(address,uint256,bytes,uint8,uint256,uint256,uint256,address,address,bytes)
  int offset_data = 4 + 10 * 32;
  int offset_sig  = offset_data + ((tx_data->data.len + 31) / 32 + 1) * 32;
  int size        = offset_sig + ((sig_data.len + 31) / 32 + 1) * 32;

  uint8_t* raw = _calloc(size, 1);
  memcpy(raw, "\x6a\x76\x12\x02", 4);                                    // get TransactionHash functionhash
  cpy_right(raw, 0, tx_data->to);                                        // to
  cpy_right(raw, 1, tx_data->value);                                     // value
  int_to_bytes(offset_data, raw + 4 + 64 + 28);                          // offset for data bytes
  int_to_bytes(tx_data->data.len, raw + offset_data + 28);               // len for data bytes
  memcpy(raw + offset_data + 32, tx_data->data.data, tx_data->data.len); // copy data
  cpy_right(raw, 4, tx_data->gas);
  cpy_right(raw, 5, tx_data->gas);
  cpy_right(raw, 6, tx_data->gas_price);
  int_to_bytes(offset_sig, raw + 4 + 9 * 32 + 28);            // offset for signatures bytes
  int_to_bytes(sig_data.len, raw + offset_sig + 28);          // len for sig bytes
  memcpy(raw + offset_sig + 32, sig_data.data, sig_data.len); // copy sigs
  _free(sig_data.data);

  return bytes(raw, size);
}

static bool is_valid(sig_data_t* data, uint8_t** owners, address_t new_sig, int sig_count, int owner_count) {
  bool valid = false;
  for (int i = 0; i < owner_count; i++) {
    if (memcmp(new_sig, owners[i], 20) == 0) {
      valid = true;
      break;
    }
  }

  if (!valid) return false;

  for (int i = 0; i < sig_count; i++) {
    if (memcmp(new_sig, data->address, 20) == 0) return false;
  }
  return true;
}

static in3_ret_t fill_signature(in3_ctx_t* ctx, bytes_t* signatures, uint32_t* sig_count, uint32_t threshold, sig_data_t* sig_data, bytes32_t tx_hash, uint8_t** owners, uint32_t owner_len) {
  // check passed signatures
  if (!signatures) return IN3_OK;
  uint32_t index = *sig_count;
  for (unsigned int i = 0; i < signatures->len && index < threshold; i += 65) {
    uint8_t v = signatures->data[i + 64];
    if (v == 0) { // contract signature
      uint32_t offset = bytes_to_int(signatures->data + i + 64 - 4, 4);
      memcpy(sig_data[index].sig, signatures->data + i, 65);
      sig_data[index].address = signatures->data + i + 12;
      sig_data[index].data    = bytes(signatures->data + offset, 32 + bytes_to_int(signatures->data + offset + 32 - 4, 4));
    } else if (v == 1) {
      memset(sig_data + index, 0, sizeof(sig_data_t));
      memcpy(sig_data[index].sig, signatures->data + i, 65);
      sig_data[index].address = signatures->data + i + 12;
      sig_data[index].data    = bytes(NULL, 0);
    } else if (v > 26) {
      if (!ecrecover_sig(tx_hash, signatures->data + i, sig_data[index].address)) return ctx_set_error(ctx, "could not recover the signature", IN3_EINVAL);
      memcpy(sig_data[index].sig, signatures->data + i, 65);
      sig_data[index].data = bytes(NULL, 0);
    } else
      return ctx_set_error(ctx, "invalid signature (v-value)", IN3_EINVAL);
    if (is_valid(sig_data, owners, sig_data[index].address, index, owner_len)) index++;
  }
  *sig_count = index;
  return IN3_OK;
}

static in3_ret_t add_approved(in3_ctx_t* ctx, uint8_t** owners, uint32_t owner_len, uint32_t* sig_count, uint32_t threshold, sig_data_t* sig_data, bytes32_t tx_hash, multisig_t* ms) {
  // we don't have enough signatures, so we need to check if owners have preapproved
  for (unsigned int i = 0; i < owner_len && *sig_count < threshold; i++) {
    if (is_valid(sig_data, owners, owners[i], *sig_count, owner_len)) {
      // we don't have a signature from this owner
      uint8_t  check_approved[68];
      bytes_t* result;
      memcpy(check_approved, "\x7d\x83\x29\x74", 4); // 7d832974: approvedHashes(address,bytes32)
      memset(check_approved + 4, 0, 12);
      memcpy(check_approved + 16, owners[i], 20);
      memcpy(check_approved + 36, tx_hash, 32);
      TRY(call(ctx, ms->address, bytes(check_approved, 68), &result))
      if (result->len != 32) return ctx_set_error(ctx, "invalid response for approved check", IN3_EINVAL);
      if (result->data[31]) {
        memset(sig_data + *sig_count, 0, sizeof(sig_data_t));
        memcpy(sig_data[*sig_count].sig + 12, owners[i], 20);
        sig_data[*sig_count].address = owners[i];
        sig_data[*sig_count].data    = bytes(NULL, 0);
        (*sig_count)++;
      }
    }
  }
  return IN3_OK;
}

static void rlp_encode_bytes(bytes_builder_t* bb, bytes_t tmp) {
  rlp_encode_item(bb, &tmp);
}

static void rlp_encode_uint(bytes_builder_t* bb, uint64_t val) {
  bytes_t tmp = {.data = alloca(8), .len = 8};
  long_to_bytes(val, tmp.data);
  b_optimize_len(&tmp);
  if (tmp.len == 1 && !*tmp.data) tmp.len = 0;
  rlp_encode_item(bb, &tmp);
}

static void exec_tx(bytes_t* target, tx_data_t* tx_data, sig_data_t* signatures, uint32_t sig_count, address_t ms) {
  bytes_builder_t bb   = {.b = bytes(_malloc(tx_data->data.len + 200), 0), .bsize = tx_data->data.len + 200};
  bytes_t         data = get_exec_tx_data(tx_data, signatures, sig_count);
  rlp_encode_item(&bb, &tx_data->nonce);                                             // we copy the signer nonce
  rlp_encode_item(&bb, &tx_data->gas_price);                                         // and current gas price
  rlp_encode_uint(&bb, bytes_to_long(tx_data->gas.data, tx_data->gas.len) + 300000); // we need to add some gas to the original because now we go through ms
  rlp_encode_bytes(&bb, bytes(ms, 20));                                              // but send it to the ms
  rlp_encode_bytes(&bb, bytes(NULL, 0));                                             // we don't send value since this will be done from the multisig
  rlp_encode_item(&bb, &data);                                                       // the functiondata
  rlp_encode_item(&bb, &tx_data->v);                                                 // v
  rlp_encode_bytes(&bb, bytes(NULL, 0));                                             // empty because signature
  rlp_encode_bytes(&bb, bytes(NULL, 0));                                             // is still missing
  *target = bb.b;
  _free(data.data);
}

static void approve_hash(bytes_t* target, tx_data_t* tx_data, bytes32_t hash, address_t ms) {
  bytes_builder_t bb = {.b = bytes(_malloc(tx_data->data.len + 200), 0), .bsize = tx_data->data.len + 200};
  uint8_t         data[4 + 32];
  memcpy(data, "\xd4\xd9\xbd\xcd", 4); // d4d9bdcd: approveHash(bytes32)
  memcpy(data + 4, hash, 32);
  rlp_encode_item(&bb, &tx_data->nonce);     // we copy the signer nonce
  rlp_encode_item(&bb, &tx_data->gas_price); // and current gas price
  rlp_encode_uint(&bb, 100000);              // we need only 100k
  rlp_encode_bytes(&bb, bytes(ms, 20));      // but send it to the ms
  rlp_encode_bytes(&bb, bytes(NULL, 0));     // we don't send value since this will be done from the multisig
  rlp_encode_bytes(&bb, bytes(data, 36));    // the functiondata
  rlp_encode_item(&bb, &tx_data->v);         // v
  rlp_encode_bytes(&bb, bytes(NULL, 0));     // empty because signature
  rlp_encode_bytes(&bb, bytes(NULL, 0));     // is still missing
  rlp_encode_to_list(&bb);
  *target = bb.b;
}

in3_ret_t gs_prepare_tx(in3_ctx_t* ctx, void* cptr, bytes_t raw_tx, bytes_t* new_raw_tx) {
  multisig_t* ms        = cptr;
  bytes_t*    tmp       = NULL;
  uint32_t    threshold = 0,
           sig_count    = 0,
           owner_len    = 0;
  uint8_t**   owners    = NULL;
  uint64_t    nonce     = 0;
  tx_data_t   tx_data   = {0};
  bytes32_t   tx_hash   = {0};
  sig_data_t* sig_data  = NULL;

  // get Threshold
  in3_ret_t ret = call(ctx, ms->address, bytes((uint8_t*) "\xe7\x52\x35\xb8", 4), &tmp);
  if (ret == IN3_OK) {
    if (tmp->len != 32) return ctx_set_error(ctx, "invalid threshold result", IN3_ERPC);
    threshold = bytes_to_int(tmp->data + 28, 4);
  } else if (ret != IN3_WAITING)
    return ret;

  // get nonce
  in3_ret_t ret2 = call(ctx, ms->address, bytes((uint8_t*) "\xaf\xfe\xd0\xe0", 4), &tmp);
  if (ret2 == IN3_OK) {
    if (tmp->len != 32) return ctx_set_error(ctx, "invalid nonce result", IN3_ERPC);
    nonce = bytes_to_long(tmp->data + 24, 8);
  } else if (ret2 != IN3_WAITING)
    return ret2;
  else
    ret = IN3_WAITING;

  // get Owners
  ret2 = call(ctx, ms->address, bytes((uint8_t*) "\xa0\xe6\x7e\x2b", 4), &tmp);
  if (ret2 != IN3_OK) ret = ret2;
  if (ret) return ret;

  // check the owners
  if (tmp->len < 64) return ctx_set_error(ctx, "invalid owner result", IN3_ERPC);
  owner_len = bytes_to_int(tmp->data + 32 + 28, 4);
  owners    = alloca(sizeof(void*) * owner_len);
  sig_data  = alloca(sizeof(sig_data_t) * threshold);
  if (tmp->len != 64 + 32 * owner_len) return ctx_set_error(ctx, "invalid owner result length", IN3_ERPC);
  for (unsigned int i = 0; i < owner_len; i++) owners[i] = tmp->data + i * 32 + 64 + 12;

  // if we are one of the owners, we add our signature first (maybe in the future we should check if we are a initiator)
  if (is_valid(sig_data, owners, ms->signer->default_address, sig_count, owner_len)) {
    memset(sig_data->sig, 0, 65);                                // clear
    memcpy(sig_data->sig + 12, ms->signer->default_address, 20); // we use the address as constant part
    sig_data->address = ms->signer->default_address;             // keep address of the owner
    sig_data->data    = bytes(NULL, 0);                          // no data needed for sig-type 1
    sig_data->sig[64] = 1;                                       // mark as pre approved
    sig_count++;                                                 // we have at least one signature now.
  }

  // get the tx values from the raw tx
  TRY(decode_tx(ctx, raw_tx, &tx_data))

  // run request to get the transactionhash
  TRY(get_tx_hash(ctx, ms, &tx_data, tx_hash, nonce))

  // verifiy and copy the passed signatures into sig_data
  TRY(fill_signature(ctx, d_get_bytes(d_get(ctx->requests[0], K_IN3), "msSigs"), &sig_count, threshold, sig_data, tx_hash, owners, owner_len))

  // look for already approved messages from owners where we don't have the signature yet.
  TRY(add_approved(ctx, owners, owner_len, &sig_count, threshold, sig_data, tx_hash, ms))

  if (sig_count >= threshold)
    // if we have enough signatures, we execute the the tx
    exec_tx(new_raw_tx, &tx_data, sig_data, sig_count, ms->address);
  else if (is_valid(sig_data, owners, ms->signer->default_address, 0, owner_len))
    // if not we simply approve it
    approve_hash(new_raw_tx, &tx_data, tx_hash, ms->address);
  else
    return ctx_set_error(ctx, "the account is not an owner and does not have enough signatures to exwecute the transaction!", IN3_EINVAL);

  return IN3_OK;
}
in3_ret_t delegate_sign(in3_sign_ctx_t* sc) {
  multisig_t* ms = sc->wallet;
  sc->wallet     = ms->signer->wallet;
  in3_ret_t res  = ms->signer->sign(sc);
  sc->wallet     = ms;

  return res;
}

in3_signer_t* create_gnosis_safe_signer(address_t adr, in3_signer_t* old_signer) {
  multisig_t* ms = _malloc(sizeof(multisig_t));
  ms->type       = MS_GNOSIS_SAFE;
  ms->signer     = old_signer;
  memcpy(ms->address, adr, 20);

  in3_signer_t* signer = _malloc(sizeof(in3_signer_t));
  signer->prepare_tx   = gs_prepare_tx;
  signer->sign         = delegate_sign;
  signer->wallet       = ms;
  memcpy(signer->default_address, old_signer->default_address, 20);
  return signer;
}

void add_gnosis_safe(in3_t* in3, address_t adr) {
  in3->signer = create_gnosis_safe_signer(adr, in3->signer);
}