#include <stdint.h>  
#include "../util/bytes.h"
#include "client.h"
#include "context.h"
#include <stdbool.h>

#ifndef CACHE_H
#define CACHE_H

int in3_cache_init(in3* c);

int in3_cache_update_nodelist(in3* c, in3_chain_t* chain);
int in3_cache_store_nodelist(in3_ctx_t* ctx, in3_chain_t* chain);

int in3_client_fill_chain(in3_chain_t* chain, in3_ctx_t* ctx,jsmntok_t* result);


#endif
