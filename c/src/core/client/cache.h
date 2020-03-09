/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 * 
 * 
 * COMMERCIAL LICENSE USAGE
 * 
 * Licensees holding a valid commercial license may use this file in accordance 
 * with the commercial license agreement provided with the Software or, alternatively, 
 * in accordance with the terms contained in a written agreement between you and 
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
 * information please contact slock.it at in3@slock.it.
 * 	
 * Alternatively, this file may be used under the AGPL license as follows:
 *    
 * AGPL LICENSE USAGE
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

/**
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
 * It will be automatically called if the nodelist has changed and read from the nodes or the weight of a node changed.
 * 
 */
in3_ret_t in3_cache_store_nodelist(
    in3_ctx_t*   ctx, /**< the current incubed context */
    in3_chain_t* chain /**< the chain upating to cache */);

/**
 * reads the whitelist from cache.
 *
 * This function is usually called internally to fill the weights
 * and whitelist from the the cache.
 * If you call `in3_cache_init` there is no need to call this explicitly.
 */
in3_ret_t in3_cache_update_whitelist(
    in3_t*       c, /**< the incubed client */
    in3_chain_t* chain /**< chain to configure */);

/**
 * stores the whitelist to thes cache.
 *
 * It will be automatically called if the whitelist has changed.
 *
 */
in3_ret_t in3_cache_store_whitelist(
    in3_ctx_t*   ctx, /**< the current incubed context */
    in3_chain_t* chain /**< the chain upating to cache */);

#endif
