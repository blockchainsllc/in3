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
// @PUBLIC_HEADER
#ifndef IN3_SWIFT_H
#define IN3_SWIFT_H
#ifdef __clang__
#define _NONULL _Nonnull
#else
#define _NONULL
#endif
#include "../../core/client/plugin.h"

typedef struct in3_swift_cb {
  in3_ret_t (*_NONULL cache_get)(in3_cache_ctx_t* _Nonnull ctx);
  in3_ret_t (*_NONULL cache_set)(in3_cache_ctx_t* _Nonnull ctx);
  in3_ret_t (*_NONULL cache_clear)();
  char* (*_NONULL sign_accounts)(in3_sign_account_ctx_t* _Nonnull ctx);
} swift_cb_t;

in3_ret_t in3_register_swift(in3_t* _NONULL c, swift_cb_t* _NONULL cbs);


char* sign_get_method(in3_req_t* r);
bytes_t sign_get_message(in3_req_t* r);
uint8_t* sign_get_from(in3_req_t* r);
int sign_get_payload_type(in3_req_t* r);
char* sign_get_metadata(in3_req_t* r);
#endif // IN3_SWIFT_H
