/*
 * Copyright (c) 2017 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include "eth_api.h"   //wrapper for easier use
#include "eth_basic.h" // the full ethereum verifier containing the EVM
#include "util/debug.h"
#include "util/log.h"
#include "util/mem.h"
#include "block_number.h"
#include "receipt.h"
/**
 * In3 Setup and usage
 * **/
/* Perform in3 requests for http transport */
in3_ret_t local_transport_func(char** urls, int urls_len, char* payload, in3_response_t* result) {
  for (int i = 0; i < urls_len; i++) {    
    if (strstr(payload, "eth_getTransactionReceipt") != NULL) {
      in3_log_debug("Returning eth_getTransactionReceipt ...\n");
      sb_add_range(&(result[i].result), mock_tx_receipt, 0, mock_tx_receipt_len);
    } else if (strstr(payload, "eth_blockNumber") != NULL) {
      in3_log_debug("Returning eth_blockNumber ...\n");
      sb_add_range(&(result[i].result), block_number_res, 0, block_number_res_len);
    } else {
      in3_log_debug("Not supported for this mock\n");
    }
  }
  return IN3_OK;
}

in3_ret_t transport_mock(in3_request_t* req) {
  return local_transport_func((char**) req->urls, req->urls_len, req->payload, req->results);
}

in3_t* init_in3(in3_transport_send custom_transport, chain_id_t chain) {
  in3_t* in3 = NULL;
  //int    err;
  in3_log_set_quiet(0);
  in3_log_set_level(LOG_DEBUG);
  in3_register_eth_basic();
  in3 = in3_for_chain(0);
  if (custom_transport)
    in3->transport = custom_transport; // use curl to handle the requests
  in3->request_count    = 1;           // number of requests to sendp
  in3->include_code     = 1;
  in3->max_attempts     = 1;
  in3->request_count    = 1; // number of requests to sendp
  in3->chain_id         = chain;
  in3->auto_update_list = false;
  in3->use_binary       = true;
  for (int i = 0; i < in3->chains_length; i++) in3->chains[i].nodelist_upd8_params = NULL;
  return in3;
}

//this instruction makes the qemu exit, not in a clean way yet but it works 
static inline void _exit_qemu(){
	register u32_t r0 __asm__("r0");
  r0 = 0x18;
	register u32_t r1 __asm__("r1"); 
  r1 = 0x20026;
   __asm__ volatile("bkpt #0xAB"); 
}

void main() {
  // the hash of transaction whose receipt we want to get
  in3_t*    in3 = init_in3(transport_mock, 0x5);
  bytes32_t tx_hash;
  hex_to_bytes("0x8e7fb87e95c69a780490fce3ea14b44c78366fc45baa6cb86a582166c10c6d9d", -1, tx_hash, 32);
  // get the tx receipt by hash
  eth_tx_receipt_t* txr = eth_getTransactionReceipt(in3, tx_hash);
  in3_log_debug("status %d\n", txr->status);
  in3_log_debug("gas %d\n", txr->gas_used);
  in3_log_debug("1/1 IN3 TEST PASSED !\n");
  eth_tx_receipt_free(txr);
  in3_free(in3);
  _exit_qemu();
}
