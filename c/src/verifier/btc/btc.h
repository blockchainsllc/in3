// @PUBLIC_HEADER
/** @file
 * Bitcoin verification.
 * */

#ifndef in3_btc_h__
#define in3_btc_h__

#include "../../core/client/verifier.h"

/**
 * this function should only be called once and will register the bitcoin verifier.
 */
in3_ret_t in3_register_btc(in3_t* c);

#endif // in3_btc_h__