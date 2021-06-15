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

#include "eth_nano.h"
#include "../../../core/client/keys.h"
#include "../../../core/client/plugin.h"
#include "../../../core/client/request.h"
#include "../../../core/util/debug.h"
#include "../../../core/util/mem.h"
#include "../../../third-party/crypto/ecdsa.h"
#include "merkle.h"
#include "serialize.h"
#include <string.h>

// list of methods allowed withoput proof
#define MAX_METHODS 26
const char* ALLOWED_METHODS[MAX_METHODS] = {"eth_chainId", "in3_stats", "eth_blockNumber", "web3_clientVersion", "web3_sha3", "net_version", "net_peerCount", "net_listening", "eth_protocolVersion", "eth_syncing", "eth_coinbase", "eth_mining", "eth_hashrate", "eth_gasPrice", "eth_accounts", "eth_sign", "eth_sendRawTransaction", "eth_estimateGas", "eth_getCompilers", "eth_compileLLL", "eth_compileSolidity", "eth_compileSerpent", "eth_getWork", "eth_submitWork", "eth_submitHashrate", "eth_getProof"};

in3_ret_t in3_verify_eth_nano(void* p_data, in3_plugin_act_t action, void* pctx) {
  UNUSED_VAR(p_data);
  UNUSED_VAR(action);
  in3_vctx_t* vc = pctx;
  // do we support this request?
  if (in3_req_get_proof(vc->req, vc->index) == PROOF_NONE) return IN3_OK;

  // do we have a result? if not it is a vaslid error-response
  if (!vc->result) return IN3_OK;
  // check if this call is part of the not verifieable calls
  for (int i = 0; i < MAX_METHODS; i++) {
    if (strcmp(ALLOWED_METHODS[i], vc->method) == 0)
      return IN3_OK;
  }

  if (VERIFY_RPC("eth_getTransactionReceipt"))
    // for txReceipt, we need the txhash
    return eth_verify_eth_getTransactionReceipt(vc, d_get_bytes_at(d_get(vc->request, K_PARAMS), 0));
  else
    return IN3_EIGNORE;
}

in3_ret_t in3_register_eth_nano(in3_t* c) {
  return in3_plugin_register(c, PLGN_ACT_RPC_VERIFY, in3_verify_eth_nano, NULL, false);
}
