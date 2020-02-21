/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 *
 * Copyright (C) 2018-2019 slock.it GmbH, Blockchains LLC
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
#include "../api_utils.h"

static const char* enc_to_str(ipfs_enc_t enc) {
  switch (enc) {
    case IPFS_ENC_HEX: return "hex";
    case IPFS_ENC_UTF8: return "utf8";
    case IPFS_ENC_B64: return "base64";
    default: return NULL;
  }
}

char* ipfs_put(in3_t* in3, const char* content, ipfs_enc_t encoding) {
  rpc_init;
  sb_add_char(params, '\"');
  sb_add_chars(params, content);
  sb_add_chars(params, "\",\"");
  sb_add_chars(params, enc_to_str(encoding));
  sb_add_char(params, '\"');
  rpc_exec("ipfs_put", char*, _strdupn(d_string(result), -1));
}

char* ipfs_put_bytes(in3_t* in3, const bytes_t* content, ipfs_enc_t encoding) {
  rpc_init;
  sb_add_char(params, '\"');
  sb_add_bytes(params, NULL, content, 1, false);
  sb_add_chars(params, "\",\"");
  sb_add_chars(params, enc_to_str(encoding));
  sb_add_char(params, '\"');
  rpc_exec("ipfs_put", char*, _strdupn(d_string(result), -1));
}

char* ipfs_get(in3_t* in3, const char* multihash, ipfs_enc_t encoding) {
  rpc_init;
  sb_add_char(params, '\"');
  sb_add_chars(params, multihash);
  sb_add_chars(params, "\",\"");
  sb_add_chars(params, enc_to_str(encoding));
  sb_add_char(params, '\"');
  rpc_exec("ipfs_get", char*, _strdupn(d_string(result), -1));
}
