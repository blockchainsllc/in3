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

// @PUBLIC_HEADER
/** @file
 * activate cryptocell-signer
 * */

#ifndef CRYPTOCELL_SIGNER_H
#define CRYPTOCELL_SIGNER_H
#ifdef __cplusplus
extern "C" {
#endif
#include "../../core/client/plugin.h"
#include "../../core/util/bytes.h"
#include "cryptocell.h"

typedef int (*generate_ek_cbk)(uint8_t* key_label, uint32_t label_size, uint8_t* key_out);
typedef int (*generate_pk_cbk)(uint8_t* pk_key_out, uint8_t* pub_key_out);
typedef int (*store_pk_kmu_cbk)(uint32_t slot, uint8_t* key);
typedef int (*load_pk_kmu_cbk)(uint32_t slot, uint8_t* key);
typedef int (*destroy_key_cbk)(uint8_t* key);
typedef int (*export_publickey_cbk)(uint8_t pk, uint8_t* public_key, size_t* key_out_len);
typedef int (*identity_key_is_stored_cbk)(uint32_t slot);

typedef struct {
  generate_pk_cbk            gen_pk_func;
  store_pk_kmu_cbk           str_pk_func;
  load_pk_kmu_cbk            ld_pk_func;
  destroy_key_cbk            des_pk_func;
  export_publickey_cbk       export_publickey_func;
  identity_key_is_stored_cbk identity_key_is_stored_func;
} cryptocell_cbks_t;

typedef enum {
  SIGN_CURVE_ECDSA,
  SIGN_CURVE_ECDH
} cryptocell_curve_type_t;

typedef struct
{
  bytes_t*                msg;
  uint8_t                 pk[32];
  uint8_t                 huk_slot;
  uint8_t                 ik_slot;
  cryptocell_curve_type_t curve_type;
  cryptocell_cbks_t*      cbks;
} cryptocell_signer_info_t;

in3_ret_t eth_set_cryptocell_signer(in3_t* in3, cryptocell_signer_info_t* signer_info);

/**
 * @brief Set the up cryptocell object
 *
 * @param info
 */
int register_cryptocell_cbk(cryptocell_cbks_t* cbks);

#ifdef __cplusplus
}
#endif
#endif // CRYPTOCELL_SIGNER_H