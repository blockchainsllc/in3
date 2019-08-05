#include "../core/client/keys.h"
#include "../core/client/verifier.h"
#include "big.h"
#include "code.h"
#include "evm.h"

static d_token_t* get_account(in3_vctx_t* vc, d_token_t* accounts, uint8_t* address) {
  int        i;
  d_token_t* t = NULL;

  if (!accounts) {
    vc_err(vc, "no accounts");
    return NULL;
  }
  for (i = 0, t = accounts + 1; i < d_len(accounts); i++, t = d_next(t)) {
    if (memcmp(d_get_byteskl(t, K_ADDRESS, 20)->data, address, 20) == 0)
      return t;
  }
  vc_err(vc, "The account could not be found!");
  return NULL;
}

int in3_get_env(void* evm_ptr, uint16_t evm_key, uint8_t* in_data, int in_len, uint8_t** out_data, int offset, int len) {
  bytes_t* res = NULL;

  d_token_t *t, *t2;
  int        i;

  evm_t* evm = evm_ptr;
  if (!evm) return EVM_ERROR_INVALID_ENV;
  in3_vctx_t* vc = evm->env_ptr;
  if (!vc) return EVM_ERROR_INVALID_ENV;

  switch (evm_key) {
    case EVM_ENV_BLOCKHEADER:
      if (!(res = d_get_bytesk(vc->proof, K_BLOCK)))
        return EVM_ERROR_INVALID_ENV;
      *out_data = res->data;
      return res->len;

    case EVM_ENV_BALANCE:
      if (!(t = get_account(vc, d_get(vc->proof, K_ACCOUNTS), in_data)) || !(t = d_get(t, K_BALANCE)))
        return EVM_ERROR_INVALID_ENV;
      bytes_t b1 = d_to_bytes(t);
      *out_data  = b1.data;
      return b1.len;

    case EVM_ENV_NONCE:
      if (!(t = get_account(vc, d_get(vc->proof, K_ACCOUNTS), in_data)) || !(t = d_get(t, K_NONCE)))
        return EVM_ERROR_INVALID_ENV;
      bytes_t b2 = d_to_bytes(t);
      *out_data  = b2.data;
      return b2.len;

    case EVM_ENV_STORAGE:
      if (!(t = get_account(vc, d_get(vc->proof, K_ACCOUNTS), evm->address)) || !(t = d_get(t, K_STORAGE_PROOF)))
        return EVM_ERROR_INVALID_ENV;

      for (i = 0, t2 = t + 1; i < d_len(t); i++, t2 = d_next(t2)) {
        bytes_t k = d_to_bytes(d_get(t2, K_KEY));
        if (!k.data) return EVM_ERROR_INVALID_ENV;

        if (big_cmp(in_data, in_len, k.data, k.len) == 0) {
          k = d_to_bytes(d_get(t2, K_VALUE));
          if (!k.data) return EVM_ERROR_INVALID_ENV;
          *out_data = k.data;
          return k.len;
        }
      }
      return EVM_ERROR_INVALID_ENV;

    case EVM_ENV_BLOCKHASH:
      return EVM_ERROR_UNSUPPORTED_CALL_OPCODE;

    case EVM_ENV_CODE_SIZE: {
      if (in_len != 20) return EVM_ERROR_INVALID_ENV;
      cache_entry_t* entry = in3_get_code(vc, in_data);
      if (!entry) return EVM_ERROR_INVALID_ENV;
      *out_data = entry->buffer;
      return 4;
    }
    case EVM_ENV_CODE_HASH: {
      if (in_len != 20) return EVM_ERROR_INVALID_ENV;
      if (!(t = get_account(vc, d_get(vc->proof, K_ACCOUNTS), evm->address)) || !(t = d_get(t, K_STORAGE_PROOF)))
        return EVM_ERROR_INVALID_ENV;
      t = d_getl(t, K_CODE_HASH, 32);
      if (!t) return EVM_ERROR_INVALID_ENV;
      *out_data = t->data;
      return 32;
    }
    case EVM_ENV_CODE_COPY: {
      if (in_len != 20) return EVM_ERROR_INVALID_ENV;
      cache_entry_t* entry = in3_get_code(vc, in_data);
      if (!entry) return EVM_ERROR_INVALID_ENV;
      *out_data = entry->value.data + offset;
      if (len && (uint32_t) len + offset > entry->value.len) return EVM_ERROR_INVALID_ENV;
      return entry->value.len;
    }
  }
  return -2;
}
