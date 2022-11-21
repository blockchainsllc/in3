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
#ifndef TEST
#define TEST
#endif
#ifndef TEST
#define DEBUG
#endif

#include "../../src/core/client/keys.h"
#include "../../src/core/client/plugin.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/log.h"
#include "../../src/verifier/eth1/full/eth_full.h"
#include "../test_utils.h"
#include "nodeselect/full/nodeselect_def.h"
#include <stdio.h>
#include <unistd.h>

static in3_ret_t test_bulk_transport(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  in3_http_request_t* req    = plugin_ctx;
  char*               buffer = NULL;
  long                length;
  FILE*               f = fopen("../c/test/testdata/mock/get_blocks.json", "r");
  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = _malloc(length + 1);
    fread(buffer, 1, length, f);
    buffer[length] = 0;
    fclose(f);
  }
  else {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      printf("Current working dir: %s\n", cwd);
    }
    else {
      perror("getcwd() error");
      return 1;
    }
    printf("Error coudl not find the testdata 'c/test/testdata/mock/get_blocks.json'\n");
    exit(1);
  }

  // now parse the json
  json_ctx_t* res  = parse_json(buffer);
  str_range_t json = d_to_json(d_get(d_get_at(res->result, 0), key("response")));
  in3_ctx_add_response(req->req, 0, false, json.data, json.len, 0);

  json_free(res);
  if (buffer) _free(buffer);
  return IN3_OK;
}

static void test_context_bulk() {
  in3_t* in3 = in3_for_chain(CHAIN_ID_MAINNET);
  in3_plugin_register(in3, PLGN_ACT_TRANSPORT, test_bulk_transport, NULL, true);
  in3->flags = FLAGS_STATS;
  TEST_ASSERT_NULL(in3_configure(in3, "{\"autoUpdateList\":false,\"nodeRegistry\":{\"needsUpdate\":false}}"));

  uint64_t blkno      = 5;
  sb_t     req        = SB_NULL;
  char     params[62] = {0};
  sb_add_char(&req, '[');
  for (uint64_t i = 0; i < blkno; i++) {
    sprintf(params, "{\"method\":\"eth_getBlockByNumber\",\"params\":[\"0x%" PRIx64 "\", false]}", i);
    sb_add_chars(&req, params);
    if (i + 1 < blkno)
      sb_add_chars(&req, ",");
  }
  sb_add_char(&req, ']');

  in3_req_t* block_ctx = req_new(in3, req.data);
  in3_ret_t  ret       = in3_send_req(block_ctx);
  for (uint64_t i = 0; i < blkno; i++) {
    d_token_t* hash = d_getl(d_get(block_ctx->responses[i], K_RESULT), K_HASH, 32);
    TEST_ASSERT_NOT_NULL(hash);
    char h[67] = "0x";
    bytes_to_hex(d_bytes(hash).data, 32, h + 2);
    in3_log_trace("HASH %s\n", h);
  }
  req_free(block_ctx);
  _free(req.data);
  in3_free(in3);
}

/*
 * Main
 */
int main() {
  in3_log_set_quiet(false);
  in3_log_set_level(LOG_TRACE);
  in3_register_default(in3_register_eth_full);
  in3_register_default(in3_register_nodeselect_def);

  // now run tests
  TESTS_BEGIN();
  // PASSING..
  //   RUN_TEST(test_context_bulk);
  return TESTS_END();
}
