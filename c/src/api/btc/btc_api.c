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

#include "btc_api.h"
#include "../../core/client/plugin.h"
#include "../../core/util/mem.h"
#include "../../verifier/btc/btc_serialize.h"
#include "../../verifier/btc/btc_types.h"
#include "../utils/api_utils_priv.h"

/**
 * executes the expression and expects the return value to be a int indicating the error. 
 * if the return value is negative it will stop and return this value otherwise continue. 
 */
#define TRY_OR_NULL(exp, msg) \
  {                           \
    int _r = (exp);           \
    if (_r < 0) {             \
      api_set_error(_r, msg); \
      if (res) _free(res);    \
      return NULL;            \
    }                         \
  }

#define RETURN_NULL_ERROR(code, msg) \
  {                                  \
    api_set_error(code, msg);        \
    return NULL;                     \
  }

// adds the data as hex to the string without the 0x-prefix
static void add_btc_hex(sb_t* sb, bytes_t data) {
  char* tmp = data.len > 500 ? _malloc(data.len * 2 + 1) : alloca(data.len * 2 + 1);
  sb_add_range(sb, tmp, 0, bytes_to_hex(data.data, data.len, tmp));
  if (data.len > 500) _free(tmp);
}

// converts a hex string without a 0x-prefix to a new bytes_t-pointer, which needs to be freed.
static bytes_t* hex_to_data(char* hex) {
  return hex_to_new_bytes(hex, strlen(hex));
}

// size of transactiondata
static size_t tx_data_size(d_token_t* t) {
  return d_len(d_get(t, key("vin"))) * sizeof(btc_transaction_in_t)     // vin size
         + d_len(d_get(t, key("vout"))) * sizeof(btc_transaction_out_t) // vout size
         + d_len(d_get(t, key("hex"))) / 2;                             // raw data size
}

// write into transaction-struct from json-token
static in3_ret_t fill_tx(d_token_t* t, btc_transaction_t* res, void* data, bytes32_t block_hash) {

  EXPECT_EQ(d_type(t), T_OBJECT)

  btc_tx_t   txdata;
  d_token_t* t_hex  = d_get(t, key("hex"));
  d_token_t* t_vin  = d_get(t, key("vin"));
  d_token_t* t_vout = d_get(t, key("vout"));
  if (!t_hex || !t_vin || !t_vout) return IN3_EFIND;

  res->in_active_chain = !!d_get_intd(t, key("in_active_chain"), 1);
  res->vin             = data;
  res->vout            = ((void*) res->vin) + d_len(t_vin) * sizeof(btc_transaction_in_t);
  res->data            = bytes(((void*) res->vout) + d_len(t_vout) * sizeof(btc_transaction_out_t), d_len(t_hex) / 2);
  res->vin_len         = d_len(t_vin),
  res->vout_len        = d_len(t_vout);
  res->size            = d_get_int(t, key("size"));
  res->vsize           = d_get_int(t, key("vsize"));
  res->weight          = d_get_int(t, key("weight"));
  res->version         = d_get_int(t, key("version"));
  res->locktime        = d_get_int(t, key("locktime"));
  res->time            = d_get_int(t, key("time"));
  res->blocktime       = d_get_int(t, key("blocktime"));
  res->confirmations   = d_get_int(t, key("confirmations"));

  TRY(hex_to_bytes(d_string(t_hex), -1, res->data.data, res->data.len))
  TRY(btc_parse_tx(res->data, &txdata));

  EXPECT_EQ(hex_to_bytes(d_get_string(t, key("txid")), -1, res->txid, 32), 32)
  EXPECT_EQ(hex_to_bytes(d_get_string(t, key("hash")), -1, res->hash, 32), 32)
  if (block_hash)
    memcpy(res->blockhash, block_hash, 32);
  else
    EXPECT_EQ(hex_to_bytes(d_get_string(t, key("blockhash")), -1, res->blockhash, 32), 32)

  // handle vin
  uint8_t* p     = txdata.input.data;
  uint8_t* limit = txdata.input.data + txdata.input.len;
  for (uint32_t i = 0; i < res->vin_len; i++) {
    btc_tx_in_t vin;
    p = btc_parse_tx_in(p, &vin, limit);
    if (!p) return IN3_EINVAL;

    btc_transaction_in_t* r = res->vin + i;
    r->script               = vin.script;
    r->sequence             = vin.sequence;
    r->txinwitness          = bytes(NULL, 0);
    r->vout                 = vin.prev_tx_index;
    memcpy(r->txid, vin.prev_tx_hash, 32);
  }

  // handle vout
  p     = txdata.output.data;
  limit = txdata.output.data + txdata.output.len;
  for (uint32_t i = 0; i < res->vout_len; i++) {
    btc_tx_out_t vout;
    p = btc_parse_tx_out(p, &vout);
    if (p > limit) return IN3_EINVAL;

    btc_transaction_out_t* r = res->vout + i;
    r->n                     = i;
    r->script_pubkey         = vout.script;
    r->value                 = vout.value;
  }

  return IN3_OK;
}

btc_transaction_t* btc_d_to_tx(d_token_t* t) {
  if (d_type(t) != T_OBJECT) RETURN_NULL_ERROR(IN3_EINVAL, "invalid json");
  void* res = _malloc(tx_data_size(t) + sizeof(btc_transaction_t));
  TRY_OR_NULL(fill_tx(t, res, res + sizeof(btc_transaction_t), NULL), "invalid transaction-data");
  return res;
}

static in3_ret_t fill_blockheader(d_token_t* t, btc_blockheader_t* res) {
  EXPECT_EQ(d_type(t), T_OBJECT)
  EXPECT_EQ(hex_to_bytes(d_get_string(t, K_HASH), 64, res->hash, 32), 32);
  EXPECT_EQ(hex_to_bytes(d_get_string(t, key("merkleroot")), 64, res->merkleroot, 32), 32);
  EXPECT_EQ(hex_to_bytes(d_get_string(t, key("bits")), 8, res->bits, 4), 4);
  EXPECT_EQ(hex_to_bytes(d_get_string(t, key("chainwork")), 64, res->chainwork, 32), 32);
  EXPECT_EQ(hex_to_bytes(d_get_string(t, key("previousblockhash")), 64, res->previous_hash, 32), 32);
  EXPECT_EQ(hex_to_bytes(d_get_string(t, key("nextblockhash")), 64, res->next_hash, 32), 32);

  TRY(btc_serialize_block_header(t, res->data))

  res->confirmations = d_get_int(t, key("confirmations"));
  res->height        = d_get_int(t, key("height"));
  res->version       = d_get_int(t, key("version"));
  res->time          = d_get_int(t, key("time"));
  res->nonce         = d_get_int(t, key("nonce"));
  res->n_tx          = d_get_int(t, key("nTx"));

  return IN3_OK;
}

btc_blockheader_t* btc_d_to_blockheader(d_token_t* t) {
  if (d_type(t) != T_OBJECT) RETURN_NULL_ERROR(IN3_EINVAL, "invalid json");
  btc_blockheader_t* res = _malloc(sizeof(btc_blockheader_t));
  TRY_OR_NULL(fill_blockheader(t, res), "invalid blockheader");
  return res;
}

btc_block_txids_t* btc_d_to_block_txids(d_token_t* t) {
  if (d_type(t) != T_OBJECT) RETURN_NULL_ERROR(IN3_EINVAL, "invalid json");

  d_token_t* tx = d_get(t, key("tx"));
  if (!tx) RETURN_NULL_ERROR(IN3_EINVAL, "no tx found");

  btc_block_txids_t* res = _malloc(sizeof(btc_block_txids_t) + d_len(tx) * 32);
  TRY_OR_NULL(fill_blockheader(t, &res->header), "invalid blockheader");

  // set the txids
  uint8_t* p  = ((void*) res) + sizeof(btc_block_txids_t);
  res->tx_len = d_len(tx);
  res->tx     = (void*) p;
  for (d_iterator_t iter = d_iter(tx); iter.left; d_iter_next(&iter), p += 32)
    TRY_OR_NULL(hex_to_bytes(d_string(iter.token), -1, p, 32), "invalid txid");

  return res;
}

btc_block_txdata_t* btc_d_to_block_txdata(d_token_t* t) {
  if (d_type(t) != T_OBJECT) RETURN_NULL_ERROR(IN3_EINVAL, "invalid json");

  d_token_t* tx = d_get(t, key("tx"));
  if (!tx) RETURN_NULL_ERROR(IN3_EINVAL, "no tx found");

  size_t total_data = 0;
  for (d_iterator_t iter = d_iter(tx); iter.left; d_iter_next(&iter))
    total_data += tx_data_size(iter.token);

  btc_block_txdata_t* res = _malloc(sizeof(btc_block_txdata_t) + d_len(tx) * sizeof(btc_transaction_t) + total_data);
  TRY_OR_NULL(fill_blockheader(t, &res->header), "invalid blockheader");

  btc_transaction_t* txp = ((void*) res) + sizeof(btc_block_txdata_t);
  uint8_t*           p   = ((void*) txp) + sizeof(btc_transaction_t) * d_len(tx);
  res->tx_len            = d_len(tx);
  res->tx                = txp;
  for (d_iterator_t iter = d_iter(tx); iter.left; d_iter_next(&iter), txp++) {
    TRY_OR_NULL(fill_tx(iter.token, txp, p, res->header.hash), "invalid txdata");
    p += tx_data_size(iter.token);
  }
  return res;
}

bytes_t* btc_get_transaction_bytes(in3_t* in3, bytes32_t txid) {
  rpc_init;
  sb_add_char(params, '\"');
  add_btc_hex(params, bytes(txid, 32));
  sb_add_chars(params, "\",false");
  rpc_exec("getrawtransaction", bytes_t*, hex_to_data(d_string(result)));
}

btc_transaction_t* btc_get_transaction(in3_t* in3, bytes32_t txid) {
  rpc_init;
  sb_add_char(params, '\"');
  add_btc_hex(params, bytes(txid, 32));
  sb_add_chars(params, "\",true");
  rpc_exec("getrawtransaction", btc_transaction_t*, btc_d_to_tx(result));
}

btc_blockheader_t* btc_get_blockheader(in3_t* in3, bytes32_t blockhash) {
  rpc_init;
  sb_add_char(params, '\"');
  add_btc_hex(params, bytes(blockhash, 32));
  sb_add_chars(params, "\",true");
  rpc_exec("getblockheader", btc_blockheader_t*, btc_d_to_blockheader(result));
}

bytes_t* btc_get_blockheader_bytes(in3_t* in3, bytes32_t blockhash) {
  rpc_init;
  sb_add_char(params, '\"');
  add_btc_hex(params, bytes(blockhash, 32));
  sb_add_chars(params, "\",false");
  rpc_exec("getblockheader", bytes_t*, hex_to_data(d_string(result)));
}

bytes_t* btc_get_block_bytes(in3_t* in3, bytes32_t blockhash) {
  rpc_init;
  sb_add_char(params, '\"');
  add_btc_hex(params, bytes(blockhash, 32));
  sb_add_chars(params, "\",false");
  rpc_exec("getblock", bytes_t*, hex_to_data(d_string(result)));
}

btc_block_txdata_t* btc_get_block_txdata(in3_t* in3, bytes32_t blockhash) {
  rpc_init;
  sb_add_char(params, '\"');
  add_btc_hex(params, bytes(blockhash, 32));
  sb_add_chars(params, "\",2");
  rpc_exec("getblock", btc_block_txdata_t*, btc_d_to_block_txdata(result));
}

btc_block_txids_t* btc_get_block_txids(in3_t* in3, bytes32_t blockhash) {
  rpc_init;
  sb_add_char(params, '\"');
  add_btc_hex(params, bytes(blockhash, 32));
  sb_add_chars(params, "\",1");
  rpc_exec("getblock", btc_block_txids_t*, btc_d_to_block_txids(result));
}
