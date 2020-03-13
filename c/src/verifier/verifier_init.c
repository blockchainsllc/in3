#include "verifier_init.h"
#include "../api/eth1/eth_api.h"
#include "../verifier/eth1/basic/eth_basic.h"
#include "../verifier/eth1/full/eth_full.h"
#include "../verifier/eth1/nano/eth_nano.h"
#include "../verifier/ipfs/ipfs.h"

static bool initialized;

static void verifier_init() {
  if (initialized) return;
  initialized = true;

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
}

in3_t* in3_for_chain_auto_init(chain_id_t chain_id) {
  verifier_init();
  return in3_for_chain_default(chain_id);
}
