// @PUBLIC_HEADER
/** @file
 * Bitcoin verification.
 * */

#ifndef in3_btc_h__
#define in3_btc_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "../../core/client/plugin.h"

/**
 * this function should only be called once and will register the bitcoin verifier.
 */
in3_ret_t in3_register_btc(in3_t* c);

#ifdef __cplusplus
}
#endif

#endif // in3_btc_h__