#include "../../../../src/signer/pk-signer/rpcs.h"
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "cryptocell.h"
#include "cryptocell_signer.h"
#include "in3/client.h"
#include "in3/eth_api.h"
#include "in3/eth_basic.h"
#include "in3/nodeselect_def.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

int main(void) {
  int status = 0;

  status = platform_init();
  if (status == ERROR) {
    LOG_INF("platform init failed.");
  }
  // generate random Hardware Unique Key (including MKEK) and write it in KMU
  status = write_random_key_kmu();
  if (status == ERROR) {
    LOG_INF("KMU slot/slots writing failed.");
  }

  const char* raw_tx = {"0xf86c808504e3b2920082524c94c390cc49a32736a58733cf46be42f734dd4f53cb880de0b6b3a76400000125a05ab2f48bdc6752191440ce62088b9e42f20215ee4305403579aa2e1eba615ce8a03b172e53874422756d48b449438407e5478c985680d4aaa39d762fe0d1a11683"};

  cryptocell_signer_info_t* signer_info = k_malloc(sizeof(cryptocell_signer_info_t));
  signer_info->huk_slot                 = HUK_KEYSLOT;
  signer_info->ik_slot                  = CRYPTOCELL_SIGNER_PK_SLOT;
  signer_info->curve_type               = SIGN_CURVE_ECDSA;

  // raw eth transaction message set
  signer_info->msg = hex_to_new_bytes(raw_tx, strlen(raw_tx));

  in3_ret_t ret;

  // create new incubed client
  in3_t* in3 = in3_for_chain(CHAIN_ID_MAINNET);

  ret = in3_register_nodeselect_def(in3);
  if (ret != IN3_OK) {
    LOG_INF("nodeselect register failed %d", ret);
  }

  ret = in3_register_eth_basic(in3);
  if (ret != IN3_OK) {
    LOG_INF("eth basic register failed %d", ret);
  }

  ret = in3_register_eth_api(in3);
  if (ret != IN3_OK) {
    LOG_INF("eth api register failed %d", ret);
  }

  char* jobstatus = in3_configure(in3, "{\"proof\":\"none\",\"autoUpdateList\":false,\"debug\":true,\"nodeRegistry\":{\"needsUpdate\":false}}");
  if (jobstatus) {
    LOG_INF("IN3 client configure failed %s", jobstatus);
  }

  signer_info->curve_type = SIGN_CURVE_ECDSA;
  signer_info->cbks       = NULL;

  // Invoke the cryptocell signer api
  ret = eth_set_cryptocell_signer(in3, signer_info);
  if (ret != IN3_OK) {
    LOG_INF("cryptocell signing operation failed, error:%d", ret);
    goto memory_free;
  }

  char *result, *error;

  // send raw rpc-request, which is then verified
  ret = in3_client_rpc(
      in3,             //  the configured client
      FN_IN3_SIGNDATA, // the rpc-method you want to call.
      "[\"Hello World\"]",
      &result,         // the reference to a pointer which will hold the result
      &error);
  // check and print the result or error
  if (ret == IN3_OK) {
    LOG_INF("Result: %s", result);
    k_free(result);
  }
  else {
    LOG_INF("Error sending rpc request: %s", error);
    k_free(error);
  }

memory_free:
  k_free(signer_info);
  return 0;
}