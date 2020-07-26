/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-example-espidf
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

#include "block_number.h"
#include "cJSON.h"
#include "esp_system.h"
#include "eth_call.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include <esp_event.h>
#include <esp_log.h>
#include <in3/client.h>        // the core client
#include <in3/eth_api.h>       // functions for direct api-access
#include <in3/in3_init.h>      // if included the verifier will automaticly be initialized.
#include <in3/log.h>           // logging functions
#include <in3/plugin.h>        // the context
#include <in3/signer.h>        // default signer implementation
#include <in3/stringbuilder.h> // stringbuilder tool for dynamic memory string handling
#include <in3/utils.h>
#include <sys/param.h>

static const char* REST_TAG = "esp-rest";
// in3 client
static in3_t* c;

void init_in3(void);
void in3_task_evm(void* pvParameters);
void in3_task_blk_number(void* pvParameters);
void eth_call(void);

static const char* TAG = "IN3";

in3_ret_t local_transport_func(char** urls, int urls_len, char* payload, in3_response_t* result) {
  for (int i = 0; i < urls_len; i++) {
    result[i].state = IN3_OK;
    if (strstr(payload, "eth_call") != NULL) {
      ESP_LOGI(TAG, "eth_call ...\n");
      sb_add_range(&(result[i].data), eth_call_res, 0, eth_call_res_len);
    }
    else if (strstr(payload, "eth_blockNumber") != NULL) {
      printf("Returning eth_blockNumber ...\n");
      sb_add_range(&(result[i].data), block_number_res, 0, block_number_res_len);
    }
    else {
      in3_log_debug("Not supported for this mock\n");
    }
  }
  return IN3_OK;
}
in3_ret_t transport_mock(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  in3_request_t* req = plugin_ctx;
  return local_transport_func((char**) req->urls, req->urls_len, req->payload, req->ctx->raw_response);
}
/* Setup and init in3 */
void init_in3(void) {
  c = in3_for_chain(CHAIN_ID_GOERLI);
  in3_log_set_quiet(false);
  in3_log_set_level(LOG_TRACE);
  in3_plugin_register(c, PLGN_ACT_TRANSPORT, transport_mock, NULL, true);
  c->request_count = 1; // number of requests to sendp
  c->max_attempts  = 1;
  c->flags         = FLAGS_STATS | FLAGS_INCLUDE_CODE | FLAGS_BINARY; // no autoupdate nodelist
  // c->flags         = FLAGS_STATS ; // no autoupdate nodelist
  for (int i = 0; i < c->chains_length; i++) c->chains[i].nodelist_upd8_params = NULL;
}

void eth_call(void) {
  init_in3();
  address_t contract;
  // setup lock access contract address to be excuted with eth_call
  hex_to_bytes("0x36643F8D17FE745a69A2Fd22188921Fade60a98B", -1, contract, 20);
  //ask for the access to the lock
  json_ctx_t* response = eth_call_fn(c, contract, BLKNUM_LATEST(), "hasAccess():bool");
  if (!response) {
    ESP_LOGI(REST_TAG, "Could not get the response: %s", eth_last_error());
    ESP_LOGI(REST_TAG, "IN3 TEST FAILED !\n");
    exit(1);
  }
  else {
    // convert the response to a uint32_t,
    uint8_t access = d_int(response->result);
    ESP_LOGI(TAG, "Access granted Success ? : %d \n", access);
    ESP_LOGI(REST_TAG, "IN3 TEST PASSED OK !\n");
    json_free(response);
    exit(0);
  }
}

/**
 * FreeRTOS Tasks
 * **/
/* Freertos task for evm call requests */
void in3_task_evm(void* pvParameters) {
  eth_call();
  vTaskDelete(NULL);
}

/**
 * Application main entry point
 * **/
void app_main() {
  // esp_err_t error = heap_caps_register_failed_alloc_callback(heap_caps_alloc_failed_hook);

  nvs_flash_init();
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  // execute evm call for in3
  xTaskCreate(in3_task_evm, "uTask", 1024 * 100, NULL, 7, NULL);
}
