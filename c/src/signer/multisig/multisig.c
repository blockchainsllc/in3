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
in3_ret_t gs_sign_tx(void* ctx, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst) {

  return IN3_OK;
}

void add_gnosis_safe(in3_t* in3, address_t adr) {
  multisig_t*   ms         = _malloc(sizeof(multisig_t));
  in3_signer_t* signer     = _malloc(sizeof(in3_signer_t));
  in3_signer_t* old_signer = in3->signer;
  ms->signer               = old_signer;
  signer->prepare_tx       = gs_prepare_tx;
  signer->sign             = gs_sign_tx;
  signer->wallet           = ms;
  memcpy(signer->default_address, old_signer->default_address, 20);
}
/** sets the signer and a pk to the client*/
in3_ret_t eth_set_pk_signer(in3_t* in3, bytes32_t pk) {
  if (in3->signer) _free(in3->signer);
  in3->signer             = _malloc(sizeof(in3_signer_t));
  in3->signer->sign       = eth_sign_pk_ctx;
  in3->signer->prepare_tx = NULL;
  in3->signer->wallet     = pk;

  // generate the address from the key
  uint8_t public_key[65], sdata[32];
  bytes_t pubkey_bytes = {.data = public_key + 1, .len = 64};

  ecdsa_get_public_key65(&secp256k1, pk, public_key);
  sha3_to(&pubkey_bytes, sdata);
  memcpy(in3->signer->default_address, sdata + 12, 20);
  return IN3_OK;
}
