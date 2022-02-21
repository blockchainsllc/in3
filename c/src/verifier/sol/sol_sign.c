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

#include "../../core/client/keys.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/data.h"
#include "../../core/util/debug.h"
#include "../../core/util/log.h"
#include "../../core/util/mem.h"
#include "sol.h"
#include <string.h>

int sol_encode_len(uint32_t len, uint8_t* dst) {
  for (uint32_t val = len, c = 0;;) {
    uint32_t b = val & 0x7f;
    val >>= 7;
    if (val == 0) {
      dst[c++] = b;
      return (int) c;
    }
    else
      dst[c++] = b | 0x80;
  }
}

static in3_ret_t set_recent_blockhash(in3_rpc_handle_ctx_t* ctx, d_token_t* tx, bytes32_t recentBlockHash) {
  bytes_t b;
  if ((b = d_bytes_enc(d_get(tx, key("recentBlockHash")), ENC_BASE58)).len == 32)
    memcpy(recentBlockHash, b.data, 32);
  else {
    d_token_t* tmp;
    TRY(sol_send_req(ctx, "getLatestBlockhash", "", &tmp))
    if ((b = d_bytes_enc(d_get(d_get(tmp, K_VALUE), key("blockhash")), ENC_BASE58)).len == 32)
      memcpy(recentBlockHash, b.data, 32);
    else
      return req_set_error(ctx->req, "Invalid recent blockhash", IN3_EINVAL);
  }
  return IN3_OK;
}

static uint8_t* get_base58_bytes(d_token_t* t, unsigned int len) {
  bytes_t b = d_bytes_enc(t, ENC_BASE58);
  return b.len == len ? b.data : NULL;
}

#define FLAG_WRITE 1
#define FLAG_SIGN  2

typedef struct {
  uint8_t* pubkey;
  uint32_t flags;
} sol_account_t;

typedef struct {
  sol_account_t* data;
  int            len;
  int            allocated;
} sol_account_list_t;

static int ac_index_of(uint8_t* start, int len, uint8_t* pubkey) {
  for (int i = 0; i < len; i++) {
    if (memcmp(start + i * 32, pubkey, 32) == 0) return i;
  }
  return -1;
}

static bool ac_add_pubkey(sol_account_list_t* list, sol_account_t ac) {
  for (int i = 0; i < list->len; i++) {
    if (memcmp(list->data[i].pubkey, ac.pubkey, 32) == 0) {
      list->data[i].flags |= ac.flags;
      return false;
    }
  }

  if (!list->data) {
    list->allocated = 3;
    list->data      = _malloc(sizeof(sol_account_t) * list->allocated);
  }
  else if (list->len + 1 >= list->allocated) {
    list->data = _realloc(list->data, sizeof(sol_account_t) * list->allocated * 2, sizeof(sol_account_t) * list->allocated);
    list->allocated *= 2;
  }
  list->data[list->len++] = ac;
  return true;
}

static int ac_filter(sol_account_list_t* list, bytes_builder_t* bb, uint32_t flag) {
  int c = 0;
  for (int i = 0; i < list->len; i++) {
    if (list->data[i].flags == flag) {
      if (bb) bb_write_raw_bytes(bb, list->data[i].pubkey, 32);
      c++;
    }
  }
  return c;
}

static in3_ret_t sol_sign_message(in3_rpc_handle_ctx_t* ctx, uint8_t* pubkey, bytes_t msg, uint8_t* dst) {
  bytes_t sig;
  TRY(req_require_signature(ctx->req, SIGN_EC_RAW, SIGN_CURVE_ED25519, PL_SIGN_ANY, &sig, msg, bytes(pubkey, 32), ctx->request))
  if (sig.len != 64) RPC_THROW(ctx, "Invalid signature!", IN3_EINVAL);
  memcpy(dst, sig.data, sig.len);
  return IN3_OK;
}

in3_ret_t sol_send_tx(in3_rpc_handle_ctx_t* ctx) {
  d_token_t* tx;
  bytes32_t  recentBlockHash = {0};
  uint8_t    buf[32];
  uint32_t   instructions_len = 0;
  int        total_signatures = 0;
  TRY_PARAM_GET_REQUIRED_OBJECT(tx, ctx, 0);
  TRY(set_recent_blockhash(ctx, tx, recentBlockHash))

  sol_account_list_t accounts = {0};
  bytes_builder_t    bb       = {0};
  for (d_iterator_t it = d_iter(d_get(tx, key("instructions"))); it.left; d_iter_next(&it), instructions_len++) {
    sol_account_t pa = {
        .flags  = 0,
        .pubkey = get_base58_bytes(d_get(it.token, key("programId")), 32)};
    RPC_ASSERT_CATCH(pa.pubkey, "Missing valid programId in instruction", _free(accounts.data))
    ac_add_pubkey(&accounts, pa);

    for (d_iterator_t ac = d_iter(d_get(it.token, key("accounts"))); ac.left; d_iter_next(&ac)) {
      sol_account_t sac = {
          .flags  = (d_get_int(ac.token, key("write")) ? FLAG_WRITE : 0) | (d_get_int(ac.token, key("sign")) ? FLAG_SIGN : 0),
          .pubkey = get_base58_bytes(d_get(ac.token, key("pubkey")), 32)};
      RPC_ASSERT_CATCH(sac.pubkey, "invalid account in instruction", _free(accounts.data))
      if (ac_add_pubkey(&accounts, sac) && (sac.flags & FLAG_SIGN)) total_signatures++;
    }
  }

  int sig_len_bytes            = sol_encode_len(total_signatures, buf + 8);
  int acheader_len             = sol_encode_len(accounts.len, buf);
  int start_message            = 64 * total_signatures + sig_len_bytes;
  int start_accounts           = 3 + acheader_len + start_message;
  bb.bsize                     = accounts.len * 32 + 32 + start_accounts + instructions_len * 64;
  bb.b.data                    = _malloc(bb.bsize);
  bb.b.len                     = start_accounts;
  int signatures               = ac_filter(&accounts, &bb, FLAG_SIGN | FLAG_WRITE);
  int ro_signed                = ac_filter(&accounts, &bb, FLAG_SIGN);
  int writable                 = ac_filter(&accounts, &bb, FLAG_WRITE);
  int ro_unsigned              = ac_filter(&accounts, &bb, 0);
  bb.b.data[start_message]     = signatures + ro_signed;
  bb.b.data[start_message + 1] = ro_signed;
  bb.b.data[start_message + 2] = ro_unsigned;
  memcpy(bb.b.data + start_message + 3, buf, acheader_len);
  memcpy(bb.b.data, buf + 8, sig_len_bytes);
  bb_write_raw_bytes(&bb, recentBlockHash, 32);
  UNUSED_VAR(writable);

  // add the instructions
  bb_write_raw_bytes(&bb, buf, sol_encode_len(instructions_len, buf));
  for (d_iterator_t it = d_iter(d_get(tx, key("instructions"))); it.left; d_iter_next(&it)) {
    d_token_t* accounts_idxs = d_get(it.token, key("accounts"));
    bytes_t    data          = d_bytes_enc(d_get(it.token, key("data")), ENC_BASE58);
    bb_write_byte(&bb, ac_index_of(bb.b.data + start_accounts, accounts.len, d_get_bytes(it.token, key("programId")).data));
    bb_write_raw_bytes(&bb, buf, sol_encode_len(d_len(accounts_idxs), buf));
    for (d_iterator_t ac = d_iter(accounts_idxs); ac.left; d_iter_next(&ac))
      bb_write_byte(&bb, ac_index_of(bb.b.data + start_accounts, accounts.len, d_get_bytes(ac.token, key("pubkey")).data));
    bb_write_raw_bytes(&bb, buf, sol_encode_len(data.len, buf));
    if (data.data) bb_write_raw_bytes(&bb, data.data, data.len);
  }

  // create signatures
  for (int i = 0, sign_idx = 0; i < accounts.len; i++) {
    uint8_t*       pubkey = bb.b.data + start_accounts + (i * 32);
    sol_account_t* ac     = NULL;
    for (int n = 0; n < accounts.len; n++) {
      if (memcmp(accounts.data[n].pubkey, pubkey, 32) == 0) {
        ac = accounts.data + n;
        break;
      }
    }
    if (ac && ac->flags & FLAG_SIGN) {
      TRY_CATCH(sol_sign_message(ctx, pubkey, bytes(bb.b.data + start_message, bb.b.len - start_message), bb.b.data + sig_len_bytes + 64 * sign_idx),
                _free(accounts.data);
                _free(bb.b.data))
      sign_idx++;
    }
  }

  in3_rpc_handle_with_bytes(ctx, bb.b);
  _free(accounts.data);
  _free(bb.b.data);

  return IN3_OK;
}