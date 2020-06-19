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

#include "multisig.h"
#include "../../core/client/client.h"
#include "../../core/client/keys.h"
#include "../../core/util/mem.h"
#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/secp256k1.h"
#include "../../verifier/eth1/nano/rlp.h"
#include "../../verifier/eth1/nano/serialize.h"

in3_ret_t gs_prepare_tx(void* ctx, bytes_t raw_tx, bytes_t* new_raw_tx) {

  return IN3_OK;
}
in3_ret_t delegate_sign(in3_sign_ctx_t* sc) {
  multisig_t* ms = sc->wallet;
  sc->wallet     = ms->signer->wallet;
  in3_ret_t res  = ms->signer->sign(sc);
  sc->wallet     = ms;

  return res;
}

in3_signer_t* create_gnosis_safe_signer(address_t adr, in3_signer_t* old_signer) {
  multisig_t* ms = _malloc(sizeof(multisig_t));
  ms->type       = MS_GNOSIS_SAFE;
  ms->signer     = old_signer;
  memcpy(ms->address, adr, 20);

  in3_signer_t* signer = _malloc(sizeof(in3_signer_t));
  signer->prepare_tx   = gs_prepare_tx;
  signer->sign         = delegate_sign;
  signer->wallet       = ms;
  memcpy(signer->default_address, old_signer->default_address, 20);
  return signer;
}
