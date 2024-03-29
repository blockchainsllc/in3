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
 * IN3 init module for auto initializing verifiers and transport based on build config.
 * */
#ifndef IN3_IN3_INIT_H
#define IN3_IN3_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "client.h"

#ifdef in3_for_chain
#undef in3_for_chain
#endif

#define in3_for_chain(chain_id) in3_for_chain_auto_init(chain_id)

/**
 * Global initialization for the in3 lib.
 * Note: This function is not MT-safe and is expected to be called early during
 *       during program startup (i.e. in main()) before other threads are spawned.
 */
void in3_init();

/**
 * Auto-init fallback for easy client initialization meant for single-threaded apps.
 * This function automatically calls `in3_init()` before calling `in3_for_chain_default()`.
 * To enable this feature, make sure you include this header file (i.e. `in3_init.h`)
 * before `client.h`. Doing so will replace the call to `in3_for_chain()` with this
 * function.
 */
in3_t* in3_for_chain_auto_init(chain_id_t chain_id);

#ifdef __cplusplus
}
#endif

#endif // IN3_IN3_INIT_H
