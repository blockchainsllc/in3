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
#include "../utils/api_utils_priv.h"

static void add_btc_hex(sb_t* sb, bytes_t data) {
  char* tmp = alloca(data.len * 2 + 1);
  sb_add_range(sb, tmp, 0, bytes_to_hex(data.data, data.len, tmp));
}

static bytes_t* hex_to_data(char* hex) {
  return hex_to_new_bytes(hex, strlen(hex));
}

bytes_t* btc_get_transaction_bytes(in3_t* in3, bytes32_t txid) {
  char hex[65];
  rpc_init;

  sb_add_char(params, '\"');
  add_btc_hex(params, bytes(txid, 32));
  sb_add_chars(params, "\",false");
  rpc_exec("getrawtransaction", bytes_t*, hex_to_data(d_string(result)));
}

btc_transaction_t* btc_get_transactio(in3_t* in3, bytes32_t txid) {
  char hex[65];
  rpc_init;

  sb_add_char(params, '\"');
  add_btc_hex(params, bytes(txid, 32));
  sb_add_chars(params, "\",false");
  //  rpc_exec("getrawtransaction", bytes_t*, hex_to_data(d_string(result)));
  return NULL;
}
