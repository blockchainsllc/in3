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

#ifndef IN3_VHIST_H
#define IN3_VHIST_H

#include "../../../core/client/client.h"
#include "../../../core/util/bytes.h"
#include "../../../core/util/data.h"
#include <stdint.h>

typedef enum {
  ENGINE_UNKNOWN,
  ENGINE_AURA,
  ENGINE_CLIQUE
} vhist_engine_t;

typedef struct {
  bytes_builder_t* diffs;
  bytes_builder_t* vldtrs;
  uint64_t         last_change_block;
} vhist_t;

vhist_t*         vh_new();
vhist_t*         vh_init_spec(d_token_t* spec);
vhist_t*         vh_init_nodelist(d_token_t* nodelist);
void             vh_free(vhist_t* vh);
bytes_builder_t* vh_get_validators_for_block(vhist_t* vh, uint64_t block);
vhist_engine_t   vh_get_engine_for_block(vhist_t* vh, uint64_t block);
void             vh_add_state(vhist_t* vh, d_token_t* state, bool is_spec);
void             vh_cache_save(vhist_t* vh, in3_t* c);
vhist_t*         vh_cache_retrieve(in3_t* c);

#endif // IN3_VHIST_H
