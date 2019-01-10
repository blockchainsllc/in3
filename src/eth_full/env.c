

#include "big.h"
#include "client/keys.h"
#include "client/verifier.h"
#include "code.h"
#include "evm.h"

static d_token_t* get_account(in3_vctx_t* vc, d_token_t* accounts, uint8_t* address) {
  int        i;
  d_token_t* t;

  if (!accounts) {
    vc_err(vc, "no accounts");
    return NULL;
  }
  for (i = 0, t = accounts + 1; i < d_len(accounts); i++, t = d_next(t)) {
    if (memcmp(d_get_bytesk(t, K_ADDRESS)->data, address, 20) == 0)
      return t;
  }
  vc_err(vc, "The account could not be found!");
  return NULL;
}

int in3_get_env(void* evm_ptr, uint16_t evm_key, uint8_t* in_data, int in_len, uint8_t** out_data, int offset, int len) {
  bytes_t* res;

  d_token_t *t, *t2, *k;
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
      if (!(t = get_account(vc, d_get(vc->proof, K_ACCOUNTS), in_data)) || !(res = d_get_bytesk(t, K_BALANCE)))
        return EVM_ERROR_INVALID_ENV;
      *out_data = res->data;
      return res->len;

    case EVM_ENV_STORAGE:
      if (!(t = get_account(vc, d_get(vc->proof, K_ACCOUNTS), evm->address)) || !(t = d_get(t, K_STORAGE_PROOF)))
        return EVM_ERROR_INVALID_ENV;

      for (i = 0, t2 = t + 1; i < d_len(t); i++, t2 = d_next(t2)) {
        k = d_get(t2, K_KEY);
        if (!k) return EVM_ERROR_INVALID_ENV;
        // TODO check in_len>8 with integers
        if ((d_type(k) == T_BYTES && big_cmp(in_data, in_len, k->data, k->len) == 0) || (d_type(k) == T_INTEGER && in_len < 8 && bytes_to_long(in_data, in_len) == (uint64_t) d_int(k))) {
          k = d_get(t2, K_VALUE);
          if (!k) return EVM_ERROR_INVALID_ENV;
          if (d_type(k) == T_BYTES) {
            *out_data = k->data;
            return k->len;

          } else {
            // this is a bit dirty....
            // since we have to pass a pointer to a memory, we need to use a existing
            // and since a int-token does not use its data-pointer we use it to store the int-value
            // and return the pointer.
            int l     = d_bytes_to(k, (uint8_t*) &k->data, 4);
            *out_data = (uint8_t*) &k->data;
            *out_data += 4 - l;
            return l;
          }
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
