#include "in3_init.h"
#include "../core/client/plugin.h"
#ifndef IN3_AUTOINIT_PATH
#define IN3_AUTOINIT_PATH "autoregister.h"
#endif
#include IN3_AUTOINIT_PATH

void zkcrypto_initialize();

static bool initialized;

void in3_init() {
  if (!initialized) {
    initialized = true;
    auto_init();
#ifdef ZKSYNC
    zkcrypto_initialize();
#endif
  }
}

in3_t* in3_for_chain_auto_init(chain_id_t chain_id) {
  in3_init();
  return in3_for_chain_default(chain_id);
}
