/** @file 
 * code cache.
 * */

#ifndef in3_codecache_h__
#define in3_codecache_h__

#include "../../../core/client/verifier.h"

cache_entry_t* in3_get_code(in3_vctx_t* vc, uint8_t* address);

#endif