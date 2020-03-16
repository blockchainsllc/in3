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

#include "../../../core/client/context.h"
#include "../../../core/client/keys.h"
#include "../../../core/util/data.h"
#include "../../../core/util/mem.h"
#include "../../../third-party/crypto/ecdsa.h"
#include "../../../third-party/crypto/secp256k1.h"
#include "../../../verifier/eth1/nano/eth_nano.h"
#include <string.h>

bytes_t* ecrecover_signature(bytes_t* msg_hash, d_token_t* sig) {

  // check messagehash
  bytes_t* sig_msg_hash = d_get_byteskl(sig, K_MSG_HASH, 32);
  if (sig_msg_hash && !b_cmp(sig_msg_hash, msg_hash)) return NULL;

  uint8_t  pubkey[65], sdata[64];
  bytes_t  pubkey_bytes = {.len = 64, .data = ((uint8_t*) &pubkey) + 1};
  bytes_t* r            = d_get_byteskl(sig, K_R, 32);
  bytes_t* s            = d_get_byteskl(sig, K_S, 32);
  int      v            = d_get_intk(sig, K_V);

  // correct v
  if (v >= 27) v -= 27;
  if (r == NULL || s == NULL || r->len + s->len != 64)
    return NULL;

  // concat r and s
  memcpy(sdata, r->data, r->len);
  memcpy(sdata + r->len, s->data, s->len);

  // verify signature
  if (ecdsa_recover_pub_from_sig(&secp256k1, pubkey, sdata, msg_hash->data, v) == 0)
    // hash it and return the last 20 bytes as address
    return sha3_to(&pubkey_bytes, sdata) == 0 ? b_new((char*) sdata + 12, 20) : NULL;
  else
    return NULL;
}

int eth_verify_signature(in3_vctx_t* vc, bytes_t* msg_hash, d_token_t* sig) {
  // recover the signature
  int      res  = 0, i;
  bytes_t* addr = ecrecover_signature(msg_hash, sig);

  // if we can not recover, we return 0, so no but set.
  if (addr == NULL) return 0 * vc_err(vc, "could not recover the signature");

  // try to find the signature requested
  for (i = 0; i < vc->config->signers_length; i++) {
    if (b_cmp(vc->config->signers + i, addr)) {
      // adn set the bit depending on the index.
      res = 1 << i;
      break;
    }
  }

  b_free(addr);

  return res;
}
