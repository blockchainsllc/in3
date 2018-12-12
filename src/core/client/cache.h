#include <stdint.h>  
#include "../util/bytes.h"
#include "client.h"
#include "context.h"
#include <stdbool.h>

#ifndef CACHE_H
#define CACHE_H

int in3_cache_update_nodelist(in3* c, in3_ctx_t* ctx, uint64_t chain);
int in3_cache_store_nodelist(in3* c, in3_ctx_t* ctx, uint64_t chain);



#endif
