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

#ifndef TEST
#define TEST
#endif
#ifndef TEST
#define DEBUG
#endif

#include "../../src/core/client/cache.h"
#include "../../src/core/client/context.h"
#include "../../src/core/client/nodelist.h"
#include "../../src/core/util/data.h"
#include "../../src/core/util/log.h"
#include "../../src/verifier/eth1/basic/eth_basic.h"
#include "../../src/verifier/eth1/basic/signer.h"
#include "../test_utils.h"
#include <stdio.h>
#include <unistd.h>

typedef struct response_s {
  char*              request;
  char*              response;
  struct response_s* next;
} response_t;

response_t* responses = NULL;

void add_response(char* request, char* result, char* error, char* in3) {
  response_t* r = responses;
  while (r) {
    if (r->next)
      r = r->next;
    else
      break;
  }

  response_t* n = calloc(1, sizeof(response_t));
  n->request    = request;
  n->response   = malloc(40 + strlen(result ? result : error) + (in3 ? strlen(in3) + 10 : 0));
  if (in3)
    sprintf(n->response, "[{\"id\":1,\"jsonrpc\":\"2.0\",\"%s\":%s,\"in3\":%s}]", result ? "result" : "error", result ? result : error, in3);
  else
    sprintf(n->response, "[{\"id\":1,\"jsonrpc\":\"2.0\",\"%s\":%s}]", result ? "result" : "error", result ? result : error);

  if (r)
    r->next = n;
  else
    responses = n;
}

static in3_ret_t test_transport(char** urls, int urls_len, char* payload, in3_response_t* result) {
  TEST_ASSERT_NOT_NULL(responses);
  TEST_ASSERT_EQUAL_STRING(responses->request, payload);
  sb_add_chars(&result->result, responses->response);
  response_t* next = responses->next;
  free(responses->response);
  free(responses);
  responses = next;
  return IN3_OK;
}

static void test_sign() {

  in3_register_eth_basic();

  in3_t* c          = in3_new();
  c->transport      = test_transport;
  c->chainId        = 0x1;
  c->autoUpdateList = false;
  c->proof          = PROOF_NONE;
  c->signatureCount = 0;

  for (int i = 0; i < c->chainsCount; i++) c->chains[i].needsUpdate = false;

  bytes32_t pk;
  hex2byte_arr("0x34a314920b2ffb438967bcf423112603134a0cdef0ad0bf7ceb447067eced303", -1, pk, 32);
  eth_set_pk_signer(c, pk);
  in3_set_default_signer(c->signer);

  add_response("[{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_getTransactionCount\",\"params\":[\"0xb91bd1b8624d7a0a13f1f6ccb1ae3f254d3888ba\",\"latest\"]}]", "\"0x1\"", NULL, NULL);
  add_response("[{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_gasPrice\",\"params\":[]}]", "\"0xffff\"", NULL, NULL);
  add_response("[{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_sendRawTransaction\",\"params\":[\"0xf8620182ffff8252089445d45e6ff99e6c34a235d263965910298985fcfe81ff8025a0a0973de4296ec3507fb718e2edcbd226504a9b01680e2c974212dc03cdd2ab4da016b3a55129723ebde5dca4f761c2b48d798ec7fb597ae7d8e3905f66fe03d93a\"]}]",
               "\"0x812510201f48a86df62f08e4e6366a63cbcfba509897edcc5605917bc2bf002f\"", NULL, NULL);

  in3_ctx_t* ctx = in3_client_rpc_ctx(c, "eth_sendTransaction", "[{\"to\":\"0x45d45e6ff99e6c34a235d263965910298985fcfe\", \"value\":\"0xff\" }]");

  TEST_ASSERT_TRUE(ctx && ctx_get_error(ctx, 0) == IN3_OK);
  free_ctx(ctx);
}

/*
 * Main
 */
int main() {
  in3_log_set_level(LOG_ERROR);
  TESTS_BEGIN();
  RUN_TEST(test_sign);
  return TESTS_END();
}
