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
 * util function for crypto.
 * */

#ifndef ___CRYPTO_H
#define ___CRYPTO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bytes.h"
#include "error.h"
#include "mem.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum {
  DIGEST_KECCAK     = 1,
  DIGEST_SHA256     = 2,
  DIGEST_SHA256_BTC = 3
} in3_digest_type_t;

typedef struct {
  void*             ctx;
  in3_digest_type_t type;
} in3_digest_t;

/** writes the keccak hash of the data as 32 bytes to the dst pointer. */
in3_ret_t keccak(bytes_t data, void* dst);

/** create a hash. The supported types */
in3_digest_t crypto_create_hash(in3_digest_type_t type);
void         crypto_update_hash(in3_digest_t digest, bytes_t data);
void         crypto_finalize_hash(in3_digest_t digest, void* dst);

#ifdef __cplusplus
}
#endif
#endif
