/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
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
// @PUBLIC_HEADER
/** @file
 * this file contains the json binary configuration for different environment.
 *
 *
 * */

#ifndef PRE_CONFIG_H
#define PRE_CONFIG_H

#ifdef IN3_PRE_CFG
const char* default_cfg_json = "{\"autoUpdateList\":true,\"chainId\":1,\"signatureCount\":0,\"finality\":0,\"includeCode\":false,\"bootWeights\":true,\"maxAttempts\":7,\"keepIn3\":false,\"stats\":true,\"useBinary\":false,\"experimental\":false,\"useHttp\":false,\"maxVerifiedHashes\":5,\"timeout\":10000,\"proof\":\"standard\"}";
const char* dev_cfg_json     = "{\"autoUpdateList\":true,\"chainId\":1,\"signatureCount\":0,\"finality\":0,\"includeCode\":false,\"bootWeights\":true,\"maxAttempts\":7,\"keepIn3\":false,\"stats\":true,\"useBinary\":false,\"experimental\":false,\"useHttp\":false,\"maxVerifiedHashes\":5,\"timeout\":10000,\"proof\":\"standard\"}";
const char* test_cfg_json    = "{\"autoUpdateList\":true,\"chainId\":1,\"signatureCount\":0,\"finality\":0,\"includeCode\":false,\"bootWeights\":true,\"maxAttempts\":7,\"keepIn3\":false,\"stats\":true,\"useBinary\":false,\"experimental\":false,\"useHttp\":false,\"maxVerifiedHashes\":5,\"timeout\":10000,\"proof\":\"standard\"}";
const char* prod_cfg_json    = "{\"autoUpdateList\":true,\"chainId\":1,\"signatureCount\":0,\"finality\":0,\"includeCode\":false,\"bootWeights\":true,\"maxAttempts\":7,\"keepIn3\":false,\"stats\":true,\"useBinary\":false,\"experimental\":false,\"useHttp\":false,\"maxVerifiedHashes\":5,\"timeout\":10000,\"proof\":\"standard\"}";

static inline json_ctx_t*
in3_get_preconfig(d_token_t* env) {
  if (strcmp(d_string(env), "default") == 0)
    return parse_json(default_cfg_json);
  else if (strcmp(d_string(env), "dev") == 0)
    return parse_json(dev_cfg_json);
  else if (strcmp(d_string(env), "test") == 0)
    return parse_json(test_cfg_json);
  else if (strcmp(d_string(env), "prod") == 0)
    return parse_json(prod_cfg_json);
  return NULL;
}
#endif

#endif // PRE_CONFIG_H