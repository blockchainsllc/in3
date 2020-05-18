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

// @PUBLIC_HEADER
/** @file
 * BTC API.
 *
 * This header-file defines easy to use function, which are preparing the JSON-RPC-Request, which is then executed and verified by the incubed-client.
 * */

#ifndef IN3_BTC_API_H
#define IN3_BTC_API_H

#include "../../core/client/client.h"
#include "../../core/util/bytes.h"
#include "../../verifier/btc/btc_types.h"

typedef struct btc_transaction_in {
  uint32_t  vout;
  bytes32_t txid;
  uint32_t  sequence;
  bytes_t   script;
  bytes_t   txinwitness;
} btc_transaction_in_t;

typedef struct btc_transaction_out {
  uint64_t value;
  uint32_t n;
  bytes_t  script_pubkey;
} btc_transaction_out_t;

typedef struct btc_transaction {
  bool                   in_active_chain;
  bytes_t                data;
  bytes32_t              txid;
  bytes32_t              hash;
  uint32_t               size;
  uint32_t               vsize;
  uint32_t               weight;
  uint32_t               version;
  uint32_t               locktime;
  btc_transaction_in_t*  vin;
  btc_transaction_out_t* vout;
  uint32_t               vin_len;
  uint32_t               vout_len;
  bytes32_t              blockhash;
  uint32_t               confirmations;
  uint32_t               time;
  uint32_t               blocktime;
} btc_transaction_t;

bytes_t*           btc_get_transaction_bytes(in3_t* in3, bytes32_t txid);
btc_transaction_t* btc_get_transaction_data(in3_t* in3, bytes32_t txid);

#endif //IN3_BTC_API_H
