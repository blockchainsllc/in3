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
 * reads the nodelist from cache. 
 * 
 * This function is usually called internally to fill the weights 
 * and nodelist from the the cache.
 * If you call `in3_cache_init` there is no need to call this explicitly.
 */
in3_ret_t in3_cache_update_nodelist(
    in3_t*       c, /**< the incubed client */
    in3_chain_t* chain /**< chain to configure */);

/**
 * stores the nodelist to thes cache. 
 * 
 * It will automaticly called if the nodelist has changed and read from the nodes or the wirght of a node changed.
 * 
 */
in3_ret_t in3_cache_store_nodelist(
    in3_ctx_t*   ctx, /**< the current incubed context */
    in3_chain_t* chain /**< the chain upating to cache */);

#endif
