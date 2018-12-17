/** @file 
 * handles caching and storage.
 * 
 * storing nodelists and other caches with the storage handler as specified in the client.
 * If no storage handler is specified nothing will be cached.
 * */ 


#include "../util/bytes.h"
#include "client.h"
#include "context.h"

#ifndef CACHE_H
#define CACHE_H

/**
 * inits the client.
 * 
 * This is done by checking the cache and updating the local storage.
 */
int in3_cache_init(in3_t* c);

/**
 * reads the nodelist from cache. 
 */
int in3_cache_update_nodelist(in3_t* c, in3_chain_t* chain);

/**
 * stores the nodelist to thes cache. 
 */
int in3_cache_store_nodelist(in3_ctx_t* ctx, in3_chain_t* chain);


#endif
