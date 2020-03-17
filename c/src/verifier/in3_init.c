#include "in3_init.h"
#include "../api/eth1/eth_api.h"
#include "../transport/curl/in3_curl.h"
#include "../verifier/btc/btc.h"
#include "../verifier/eth1/basic/eth_basic.h"
#include "../verifier/eth1/full/eth_full.h"
#include "../verifier/eth1/nano/eth_nano.h"
#include "../verifier/ipfs/ipfs.h"

static bool initialized;

static void init_verifier() {
#ifdef ETH_FULL
  in3_register_eth_full();
#endif
#ifdef ETH_BASIC
  in3_register_eth_basic();
#endif
#ifdef ETH_NANO
  in3_register_eth_nano();
#endif
#ifdef ETH_API
  in3_register_eth_api();
#endif
#ifdef IPFS
  in3_register_ipfs();
#endif
#ifdef BTC
  in3_register_btc();
#endif
}

static void init_transport() {
#ifdef USE_CURL
  in3_register_curl();
#endif
}

in3_t* in3_for_chain_auto_init(chain_id_t chain_id) {
  if (!initialized) {
    initialized = true;
    init_verifier();
    init_transport();
  }
  return in3_for_chain_default(chain_id);
}
