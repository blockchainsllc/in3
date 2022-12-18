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
#include "../../../core/client/request.h"
#include "../../../core/util/crypto.h"
#include "../../../core/util/data.h"
#include "../../../core/util/mem.h"
#include "../../../verifier/eth1/nano/eth_nano.h"
#include <string.h>

bytes_t* ecrecover_signature(bytes_t* msg_hash, d_token_t* sig) {

  // check messagehash
  bytes_t sig_msg_hash = d_get_byteskl(sig, K_MSG_HASH, 32);
  if (sig_msg_hash.data && !bytes_cmp(sig_msg_hash, *msg_hash)) return NULL;

  uint8_t pubkey[64], sdata[65];
  bytes_t r = d_get_byteskl(sig, K_R, 32);
  bytes_t s = d_get_byteskl(sig, K_S, 32);
  int     v = d_get_int(sig, K_V);

  // correct v
  if (v >= 27) v -= 27;
  if (r.data == NULL || s.data == NULL || r.len + s.len != 64)
    return NULL;

  // concat r and s
  memcpy(sdata, r.data, r.len);
  memcpy(sdata + r.len, s.data, s.len);
  sdata[64] = v;

  // verify signature
  if (crypto_recover(ECDSA_SECP256K1, *msg_hash, bytes(sdata, 65), pubkey) == IN3_OK)
    // hash it and return the last 20 bytes as address
    return keccak(bytes(pubkey, 64), sdata) == 0 ? b_new(sdata + 12, 20) : NULL;
  else
    return NULL;
}

unsigned int eth_verify_signature(in3_vctx_t* vc, bytes_t* msg_hash, d_token_t* sig) {
  // recover the signature
  unsigned int res  = 0, i;
  bytes_t*     addr = ecrecover_signature(msg_hash, sig);

  // if we can not recover, we return 0, so no but set.
  if (addr == NULL) return 0 * vc_err(vc, "could not recover the signature");

  // try to find the signature requested
  for (i = 0; i < vc->req->in3_state->signers_length; i++) {
    if (memcmp(vc->req->in3_state->signers + i * 20, addr->data, 20) == 0) {
      // adn set the bit depending on the index.
      res = 1 << i;
      break;
    }
  }

  b_free(addr);

  return res;
}
