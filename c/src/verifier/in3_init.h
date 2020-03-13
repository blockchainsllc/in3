#ifndef IN3_IN3_INIT_H
#define IN3_IN3_INIT_H

#include "../core/client/client.h"

#ifdef in3_for_chain
#undef in3_for_chain
#define in3_for_chain(chain_id) in3_for_chain_auto_init(chain_id)
#endif

in3_t* in3_for_chain_auto_init(chain_id_t chain_id);

#endif //IN3_IN3_INIT_H
