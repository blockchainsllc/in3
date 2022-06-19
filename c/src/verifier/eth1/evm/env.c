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

#include "../../../core/client/keys.h"
#include "../../../core/client/plugin.h"
#include "big.h"
#include "code.h"
#include "evm.h"

static d_token_t* get_account(in3_vctx_t* vc, d_token_t* accounts, uint8_t* address) {

  if (!accounts) {
    vc_err(vc, "no accounts");
    return NULL;
  }
  for_children_of(it, accounts) {
    if (memcmp(d_get_byteskl(it.token, K_ADDRESS, 20).data, address, 20) == 0)
      return it.token;
  }
  vc_err(vc, "The account could not be found!");
  return NULL;
}
#ifdef LOGGING
#define INVALID(msg)              \
  {                               \
    vc_err(vc, msg);              \
    return EVM_ERROR_INVALID_ENV; \
  }
#else
#define INVALID(msg)              \
  {                               \
    vc_err(vc, "E");              \
    return EVM_ERROR_INVALID_ENV; \
  }
#endif

int in3_get_env(void* evm_ptr, uint16_t evm_key, uint8_t* in_data, int in_len, uint8_t** out_data, int offset, int len) {
  bytes_t*  res = NULL;
  in3_ret_t ret = IN3_OK;

  d_token_t *t, *t2;
  int        i;

  evm_t* evm = evm_ptr;
  if (!evm) return EVM_ERROR_INVALID_ENV;
  in3_vctx_t* vc = evm->env_ptr;
  if (!vc) return EVM_ERROR_INVALID_ENV;

  switch (evm_key) {
    case EVM_ENV_BLOCKHEADER:
      if (!(res = d_as_bytes(d_get(vc->proof, K_BLOCK))))
        INVALID("no blockheader found")
      *out_data = res->data;
      return res->len;

    case EVM_ENV_BALANCE:
      if (!(t = get_account(vc, d_get(vc->proof, K_ACCOUNTS), in_data)) || !(t = d_get(t, K_BALANCE)))
        INVALID("account not found in proof")
      bytes_t b1 = d_bytes(t);
      *out_data  = b1.data;
      return b1.len;

    case EVM_ENV_NONCE:
      if (!(t = get_account(vc, d_get(vc->proof, K_ACCOUNTS), in_data)) || !(t = d_get(t, K_NONCE)))
        INVALID("account not found in proof")
      bytes_t b2 = d_bytes(t);
      *out_data  = b2.data;
      return b2.len;

    case EVM_ENV_STORAGE:
      if (!(t = get_account(vc, d_get(vc->proof, K_ACCOUNTS), evm->address)) || !(t = d_get(t, K_STORAGE_PROOF)))
        INVALID("account not found in proof")

      for (i = 0, t2 = d_get_at(t, 0); i < d_len(t); i++, t2 = d_next(t2)) {
        bytes_t k = d_bytes(d_get(t2, K_KEY));
        if (!k.data) INVALID("no data on storage")

        if (big_cmp(in_data, in_len, k.data, k.len) == 0) {
          k = d_bytes(d_get(t2, K_VALUE));
          if (!k.data) INVALID("no data on storage")
          *out_data = k.data;
          return k.len;
        }
      }
      INVALID("storage not found in proof");

    case EVM_ENV_BLOCKHASH:
      return EVM_ERROR_UNSUPPORTED_CALL_OPCODE;

    case EVM_ENV_CODE_SIZE: {
      if (in_len != 20) return EVM_ERROR_INVALID_ENV;
      cache_entry_t* entry = NULL;
      ret                  = in3_get_code(vc, in_data, &entry);
      if (ret < 0) return ret;
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
      *out_data = d_bytes(t).data;
      return 32;
    }
    case EVM_ENV_CODE_COPY: {
      if (in_len != 20) return EVM_ERROR_INVALID_ENV;
      cache_entry_t* entry = NULL;
      ret                  = in3_get_code(vc, in_data, &entry);
      if (ret < 0) return ret;
      if (!entry) return EVM_ERROR_INVALID_ENV;
      *out_data = entry->value.data + offset;
      if (len && (uint32_t) len + offset > entry->value.len) return EVM_ERROR_INVALID_ENV;
      return entry->value.len;
    }
  }
  return -2;
}
