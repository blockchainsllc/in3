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

#include "eth_nano.h"
#include "../../../core/client/context.h"
#include "../../../core/client/keys.h"
#include "../../../core/client/verifier.h"
#include "../../../core/util/mem.h"
#include "../../../third-party/crypto/ecdsa.h"
#include "merkle.h"
#include "serialize.h"
#include <string.h>

// list of methods allowed withoput proof
#define MAX_METHODS 25
char* ALLOWED_METHODS[MAX_METHODS] = {"eth_chainId", "in3_stats", "eth_blockNumber", "web3_clientVersion", "web3_sha3", "net_version", "net_peerCount", "net_listening", "eth_protocolVersion", "eth_syncing", "eth_coinbase", "eth_mining", "eth_hashrate", "eth_gasPrice", "eth_accounts", "eth_sign", "eth_sendRawTransaction", "eth_estimateGas", "eth_getCompilers", "eth_compileLLL", "eth_compileSolidity", "eth_compileSerpent", "eth_getWork", "eth_submitWork", "eth_submitHashrate"};

in3_ret_t in3_verify_eth_nano(in3_vctx_t* vc) {
  char*      method = NULL;
  d_token_t* params = d_get(vc->request, K_PARAMS);
  int        i;

  if (vc->config->verification == VERIFICATION_NEVER)
    return IN3_OK;

  // do we have a result? if not it is a vaslid error-response
  if (!vc->result)
    return IN3_OK;

  // do we support this request?
  if (!(method = d_get_stringk(vc->request, K_METHOD)))
    return vc_err(vc, "No Method in request defined!");

  // check if this call is part of the not verifieable calls
  for (i = 0; i < MAX_METHODS; i++) {
    if (strcmp(ALLOWED_METHODS[i], method) == 0)
      return IN3_OK;
  }

  if (strcmp(method, "eth_getTransactionReceipt") == 0)
    // for txReceipt, we need the txhash
    return eth_verify_eth_getTransactionReceipt(vc, d_get_bytes_at(params, 0));
  else if (strcmp(method, "in3_nodeList") == 0)
    return eth_verify_in3_nodelist(vc, d_get_int_at(params, 0), d_get_bytes_at(params, 1), d_get_at(params, 2));
  else if (strcmp(method, "in3_whiteList") == 0)
    return eth_verify_in3_whitelist(vc);
  else
    return vc_err(vc, "The Method cannot be verified with eth_nano!");
}

void in3_register_eth_nano() {
  in3_verifier_t* v = _calloc(1, sizeof(in3_verifier_t));
  v->type           = CHAIN_ETH;
  v->verify         = in3_verify_eth_nano;
  in3_register_verifier(v);
}
