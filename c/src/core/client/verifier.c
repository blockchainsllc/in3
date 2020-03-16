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

#include "verifier.h"
#include "client.h"
#include "context_internal.h"
#include "keys.h"

static in3_verifier_t* verifiers = NULL;

void in3_register_verifier(in3_verifier_t* verifier) {
  in3_verifier_t* existing = in3_get_verifier(verifier->type);
  if (existing) {
    existing->pre_handle = verifier->pre_handle;
    existing->verify     = verifier->verify;
  } else {
    verifier->next = verifiers;
    verifiers      = verifier;
  }
}

in3_verifier_t* in3_get_verifier(in3_chain_type_t type) {
  in3_verifier_t* v = verifiers;
  while (v) {
    if (v->type == type) return v;
    v = v->next;
  }
  return NULL;
}

in3_ret_t vc_set_error(in3_vctx_t* vc, char* msg) {

#ifdef ERR_MSG
  sb_t* sb = sb_new("[");
  sb_add_hexuint(sb, d_get_intk(vc->request, K_ID));
  sb_add_chars(sb, "]:");
  sb_add_chars(sb, msg);
  (void) ctx_set_error(vc->ctx, sb->data, IN3_EUNKNOWN);
  sb_free(sb);
#else
  (void) msg;
  (void) vc;
#endif
  return IN3_EUNKNOWN;
}
