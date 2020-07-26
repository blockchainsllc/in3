#include "in3_init.h"
#include "../core/client/plugin.h"
#include "../api/eth1/eth_api.h"
#include "../pay/eth/pay_eth.h"
#include "../transport/curl/in3_curl.h"
#include "../transport/http/in3_http.h"
#include "../verifier/btc/btc.h"
#include "../verifier/eth1/basic/eth_basic.h"
#include "../verifier/eth1/full/eth_full.h"
#include "../verifier/eth1/nano/eth_nano.h"
#include "../verifier/ipfs/ipfs.h"

static bool initialized;

static void init_verifier() {
#ifdef ETH_FULL
  in3_register_default(in3_register_eth_full);
#endif
#ifdef ETH_BASIC
  in3_register_default(in3_register_eth_basic);
#endif
#ifdef ETH_NANO
  in3_register_default(in3_register_eth_nano);
#endif
#ifdef ETH_API
  in3_register_default(in3_register_eth_api);
#endif
#ifdef IPFS
  in3_register_default(in3_register_ipfs);
#endif
#ifdef BTC
  in3_register_default(in3_register_btc);
#endif
#ifdef PAY_ETH
  in3_register_default(in3_register_pay_eth);
#endif
}
static void init_transport() {
#ifdef TRANSPORTS
#ifdef USE_CURL
  in3_register_default(in3_register_curl);
#else
  in3_register_default(in3_register_http);
#endif /* USE_CURL */
#endif /* TRANSPORTS */
}

in3_t* in3_for_chain_auto_init(chain_id_t chain_id) {
  if (!initialized) {
    initialized = true;
    init_transport();
    init_verifier();
  }
  return in3_for_chain_default(chain_id);
}
