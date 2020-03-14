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

#include "block_number.h"
#include "eth_api.h"   //wrapper for easier use
#include "eth_basic.h" // the full ethereum verifier containing the EVM
#include "receipt.h"
#include "util/log.h"
#include "util/mem.h"
/**
 * In3 Setup and usage
 * **/
/* Perform in3 requests for http transport */
in3_ret_t local_transport_func(char** urls, int urls_len, char* payload, in3_response_t* result) {
  for (int i = 0; i < urls_len; i++) {
    if (strstr(payload, "eth_getTransactionReceipt") != NULL) {
      printk("Returning eth_getTransactionReceipt ...\n");
      sb_add_range(&(result[i].result), mock_tx_receipt, 0, mock_tx_receipt_len);
    } else if (strstr(payload, "eth_blockNumber") != NULL) {
      printk("Returning eth_blockNumber ...\n");
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
  in3 = in3_for_chain(chain);
  if (custom_transport)
    in3->transport = custom_transport; // use curl to handle the requests
  in3->request_count = 1;              // number of requests to sendp
  in3->max_attempts  = 1;
  in3->request_count = 1; // number of requests to sendp
  in3->chain_id      = chain;
  in3->flags         = FLAGS_STATS | FLAGS_INCLUDE_CODE | FLAGS_BINARY;
  for (int i = 0; i < in3->chains_length; i++) {
    _free(in3->chains[i].nodelist_upd8_params);
    in3->chains[i].nodelist_upd8_params = NULL;
  }
  return in3;
}

//this instruction makes the qemu exit, not in a clean way yet but it works
static inline void _exit_qemu() {
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
  printk("status: %d ", txr->status);
  printk("gas: %lld ", txr->gas_used);
  printk("IN3 TEST PASSED !\n");
  eth_tx_receipt_free(txr);
  in3_free(in3);
  _exit_qemu();
}
