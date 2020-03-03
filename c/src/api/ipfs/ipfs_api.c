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

#include "ipfs_api.h"
#include "../../core/util/mem.h"
#include "../../third-party/libb64/cdecode.h"
#include "../../third-party/libb64/cencode.h"
#include "../utils/api_utils_priv.h"

static bytes_t* b64_to_bytes(const char* b64) {
  size_t   l    = 0;
  uint8_t* data = base64_decode(b64, &l);
  bytes_t* b    = b_new((char*) data, l);
  free(data);
  return b;
}

char* ipfs_put(in3_t* in3, const bytes_t* content) {
  char* b64 = base64_encode(content->data, content->len);
  rpc_init;
  sb_add_char(params, '\"');
  sb_add_chars(params, b64);
  sb_add_chars(params, "\",\"base64\"");
  free(b64);
  rpc_exec("ipfs_put", char*, _strdupn(d_string(result), -1));
}

bytes_t* ipfs_get(in3_t* in3, const char* multihash) {
  rpc_init;
  sb_add_char(params, '\"');
  sb_add_chars(params, multihash);
  sb_add_chars(params, "\",\"base64\"");
  rpc_exec("ipfs_get", bytes_t*, b64_to_bytes(d_string(result)));
}
