#include "in3_init.h"
#include "../api/eth1/eth_api.h"
#include "../core/client/plugin.h"
#include "../pay/eth/pay_eth.h"
#include "../pay/zksync/zksync.h"
#ifdef USE_CURL
#include "../transport/curl/in3_curl.h"
#elif USE_WINHTTP
#include "../transport/winhttp/in3_winhttp.h"
#else
#include "../transport/http/in3_http.h"
#endif
#include "../verifier/btc/btc.h"
#include "../verifier/eth1/basic/eth_basic.h"
#include "../verifier/eth1/full/eth_full.h"
#include "../verifier/eth1/nano/eth_nano.h"
#include "../verifier/ipfs/ipfs.h"
#ifdef SENTRY
#include "../utils/sentry.h"
#endif

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
#ifdef ZKSYNC
  in3_register_default(in3_register_zksync);
#endif
#ifdef SENTRY
  in3_register_default(in3_register_sentry);
#endif
}
static void init_transport() {
#ifdef TRANSPORTS
#ifdef USE_CURL
  in3_register_default(in3_register_curl);
#elif USE_WINHTTP
  in3_register_default(in3_register_winhttp);
#else
  in3_register_default(in3_register_http);
#endif /* USE_CURL */
#endif /* TRANSPORTS */
}
void in3_init() {
  if (!initialized) {
    initialized = true;
    init_transport();
    init_verifier();
  }
}
in3_t* in3_for_chain_auto_init(chain_id_t chain_id) {
  in3_init();
  return in3_for_chain_default(chain_id);
}
