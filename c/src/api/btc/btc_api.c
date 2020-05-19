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

#include "btc_api.h"
#include "../../core/util/mem.h"
#include "../../verifier/btc/btc_types.h"
#include "../utils/api_utils_priv.h"

static void add_btc_hex(sb_t* sb, bytes_t data) {
  char* tmp = alloca(data.len * 2 + 1);
  sb_add_range(sb, tmp, 0, bytes_to_hex(data.data, data.len, tmp));
}

static bytes_t* hex_to_data(char* hex) {
  return hex_to_new_bytes(hex, strlen(hex));
}

bytes_t* btc_get_transaction_bytes(in3_t* in3, bytes32_t txid) {
  rpc_init;
  sb_add_char(params, '\"');
  add_btc_hex(params, bytes(txid, 32));
  sb_add_chars(params, "\",false");
  rpc_exec("getrawtransaction", bytes_t*, hex_to_data(d_string(result)));
}

static btc_transaction_t* to_tx(d_token_t* t) {
  if (t == NULL || d_type(t) == T_NULL) return NULL;
  btc_tx_t   txdata;
  d_token_t* t_hex  = d_get(t, key("hex"));
  d_token_t* t_vin  = d_get(t, key("vin"));
  d_token_t* t_vout = d_get(t, key("vout"));
  if (!t_hex || !t_vin || !t_vout) return NULL;
  size_t             size = sizeof(btc_transaction_t) + d_len(t_vin) * sizeof(btc_transaction_in_t) + d_len(t_vout) * sizeof(btc_transaction_out_t) + d_len(t_hex) / 2;
  btc_transaction_t* res  = _malloc(size);
  res->in_active_chain    = !!d_get_intkd(t, key("in_active_chain"), 1);
  res->vin                = ((void*) res) + sizeof(btc_transaction_t);
  res->vout               = ((void*) res->vin) + d_len(t_vin) * sizeof(btc_transaction_in_t);
  res->data               = bytes(((void*) res->vout) + d_len(t_vout) * sizeof(btc_transaction_out_t), d_len(t_hex) / 2);
  res->vin_len            = d_len(t_vin),
  res->vout_len           = d_len(t_vout);
  res->size               = d_get_intk(t, key("size"));
  res->vsize              = d_get_intk(t, key("vsize"));
  res->weight             = d_get_intk(t, key("weight"));
  res->version            = d_get_intk(t, key("version"));
  res->locktime           = d_get_intk(t, key("locktime"));
  res->time               = d_get_intk(t, key("time"));
  res->blocktime          = d_get_intk(t, key("blocktime"));
  res->confirmations      = d_get_intk(t, key("confirmations"));
  hex_to_bytes(d_string(t_hex), -1, res->data.data, res->data.len);

  btc_parse_tx(res->data, &txdata);
  hex_to_bytes(d_get_stringk(t, key("txid")), -1, res->txid, 32);
  hex_to_bytes(d_get_stringk(t, key("hash")), -1, res->hash, 32);
  hex_to_bytes(d_get_stringk(t, key("blockhash")), -1, res->blockhash, 32);

  uint8_t* p = txdata.input.data;
  for (uint32_t i = 0; i < res->vin_len; i++) {
    btc_tx_in_t vin;
    p                       = btc_parse_tx_in(p, &vin);
    btc_transaction_in_t* r = res->vin + i;
    r->script               = vin.script;
    r->sequence             = vin.sequence;
    r->txinwitness          = bytes(NULL, 0);
    r->vout                 = vin.prev_tx_index;
    memcpy(r->txid, vin.prev_tx_hash, 32);
  }

  p = txdata.output.data;
  for (uint32_t i = 0; i < res->vout_len; i++) {
    btc_tx_out_t vout;
    p                        = btc_parse_tx_out(p, &vout);
    btc_transaction_out_t* r = res->vout + i;
    r->n                     = i;
    r->script_pubkey         = vout.script;
    r->value                 = vout.value;
  }

  return res;
}

btc_transaction_t* btc_get_transaction_data(in3_t* in3, bytes32_t txid) {
  rpc_init;
  sb_add_char(params, '\"');
  add_btc_hex(params, bytes(txid, 32));
  sb_add_chars(params, "\",true");
  rpc_exec("getrawtransaction", btc_transaction_t*, to_tx(result));
  return NULL;
}
