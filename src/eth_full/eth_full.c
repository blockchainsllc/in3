#include "eth_full.h"
#include "../eth_basic/eth_basic.h"
#include "../eth_nano/eth_nano.h"
#include "../eth_nano/merkle.h"
#include "../eth_nano/rlp.h"
#include "../eth_nano/serialize.h"
#include "evm.h"
#include <client/context.h>
#include <client/keys.h>
#include <crypto/ecdsa.h>
#include <crypto/secp256k1.h>
#include <string.h>
#include <util/data.h>
#include <util/mem.h>
#include <util/utils.h>

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
    bytes_t*   address = d_get_bytesk(tx, K_TO);
    uint8_t    zeros[20];
    memset(zeros, 0, 20);
    int      res       = 0;
    bytes_t* from      = d_get_bytesk(tx, K_FROM);
    bytes_t* value     = d_get_bytesk(tx, K_VALUE);
    bytes_t* data      = d_get_bytesk(tx, K_DATA);
    bytes_t  gas       = d_to_bytes(d_get(tx, K_GAS_LIMIT));
    bytes_t* result    = NULL;
    uint64_t gas_limit = bytes_to_long(gas.data, gas.len);
    if (!gas_limit) gas_limit = 0xFFFFFFFFFFFFFF;

    switch (evm_call(vc, address ? address->data : zeros, value ? value->data : zeros, value ? value->len : 1, data ? data->data : zeros, data ? data->len : 0, from ? from->data : zeros, gas_limit, &result)) {
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

      default:
        return vc_err(vc, "Unknown return-code");
    }
  } else
    return in3_verify_eth_basic(vc);
}

void in3_register_eth_full() {
  in3_verifier_t* v = _calloc(1, sizeof(in3_verifier_t));
  v->type           = CHAIN_ETH;
  v->verify         = in3_verify_eth_full;
  in3_register_verifier(v);
}
