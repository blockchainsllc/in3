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

#include "eth_full.h"
#include "../../../core/client/context_internal.h"
#include "../../../core/client/keys.h"
#include "../../../core/util/data.h"
#include "../../../core/util/log.h"
#include "../../../core/util/mem.h"
#include "../../../third-party/crypto/ecdsa.h"
#include "../../../verifier/eth1/basic/eth_basic.h"
#include "../../../verifier/eth1/nano/merkle.h"
#include "../../../verifier/eth1/nano/serialize.h"
#include "../evm/evm.h"
#include <string.h>

int in3_verify_eth_full(in3_vctx_t* vc) {
  char* method = d_get_stringk(vc->request, K_METHOD);
  if (vc->config->verification == VERIFICATION_NEVER)
    return 0;

  // do we have a result? if not it is a vaslid error-response
  if (!vc->result)
    return 0;

  // do we support this request?
  if (!method)
    return vc_err(vc, "No Method in request defined!");

  if (strcmp(method, "eth_call") == 0) {
    if (eth_verify_account_proof(vc) < 0) return vc_err(vc, "proof could not be validated");
    d_token_t* tx      = d_get_at(d_get(vc->request, K_PARAMS), 0);
    bytes_t*   address = d_get_byteskl(tx, K_TO, 20);
    address_t  zeros;
    memset(zeros, 0, 20);
    int      res       = 0;
    bytes_t* from      = d_get_byteskl(tx, K_FROM, 20);
    bytes_t* value     = d_get_bytesk(tx, K_VALUE);
    bytes_t* data      = d_get_bytesk(tx, K_DATA);
    bytes_t  gas       = d_to_bytes(d_get(tx, K_GAS_LIMIT));
    bytes_t* result    = NULL;
    uint64_t gas_limit = bytes_to_long(gas.data, gas.len);
    if (!gas_limit) gas_limit = 0xFFFFFFFFFFFFFF;
#ifdef DEBUG
    in3_log_level_t old = in3_log_get_level();
    in3_log_set_level(LOG_ERROR);
#endif

    int ret = evm_call(vc, address ? address->data : zeros, value ? value->data : zeros, value ? value->len : 1, data ? data->data : zeros, data ? data->len : 0, from ? from->data : zeros, gas_limit, vc->chain->chain_id, &result);
#ifdef DEBUG
    in3_log_set_level(old);
#endif

    switch (ret) {
      case EVM_ERROR_BUFFER_TOO_SMALL:
        return vc_err(vc, "Memory or Buffer too small!");
      case EVM_ERROR_EMPTY_STACK:
        return vc_err(vc, "The Stack is empty");
      case EVM_ERROR_ILLEGAL_MEMORY_ACCESS:
        return vc_err(vc, "There is no Memory allocated at this position.");
      case EVM_ERROR_INVALID_ENV:
        return vc_err(vc, "The env could not deliver the requested value.");
      case EVM_ERROR_INVALID_JUMPDEST:
        return vc_err(vc, "Invalid jump destination.");
      case EVM_ERROR_INVALID_OPCODE:
        return vc_err(vc, "Invalid op code.");
      case EVM_ERROR_INVALID_PUSH:
        return vc_err(vc, "Invalid push");
      case EVM_ERROR_TIMEOUT:
        return vc_err(vc, "timeout running the call");
      case EVM_ERROR_UNSUPPORTED_CALL_OPCODE:
        return vc_err(vc, "This op code is not supported with eth_call!");
      case EVM_ERROR_OUT_OF_GAS:
        return vc_err(vc, "Ran out of gas.");
      case 0:
        if (!result) return vc_err(vc, "no result");
        res = b_cmp(d_bytes(vc->result), result);
        b_free(result);
        return res ? 0 : vc_err(vc, "The result does not match the proven result");
      case IN3_WAITING:
        return IN3_WAITING;
      default:
        return ctx_set_error(vc->ctx, "General Error during execution", (in3_ret_t) ret);
    }
  } else
    return in3_verify_eth_basic(vc);
}

void in3_register_eth_full() {
  in3_verifier_t* v = _calloc(1, sizeof(in3_verifier_t));
  v->type           = CHAIN_ETH;
  v->pre_handle     = eth_handle_intern;
  v->verify         = (in3_verify) in3_verify_eth_full;
  in3_register_verifier(v);
}
