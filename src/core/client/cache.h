// @PUBLIC_HEADER
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
 * This function should be called after creating a new incubed instance.
 * 
 * example
 * ```c
 * // register verifiers
 * in3_register_eth_full();
 * 
 * // create new client
 * in3_t* client = in3_new();
 * 
 * // configure storage...
 * in3_storage_handler_t storage_handler;
 * storage_handler.get_item = storage_get_item;
 * storage_handler.set_item = storage_set_item;
 *
 * // configure transport
 * client->transport    = send_curl;
 *
 * // configure storage
 * client->cacheStorage = &storage_handler;
 * 
 * // init cache
 * in3_cache_init(client);
 * 
 * // ready to use ...
 * ```
 */
in3_ret_t in3_cache_init(in3_t* c /**< the incubed client */);

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
