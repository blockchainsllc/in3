/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
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

// @PUBLIC_HEADER
/** @file
 * util helper on byte arrays.
 * */

#ifndef UTIL_SCACHE_H
#define UTIL_SCACHE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bytes.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum cache_props {
  CACHE_PROP_MUST_FREE         = 0x1,  /**< indicates the content must be freed*/
  CACHE_PROP_SRC_REQ           = 0x2,  /**< the value holds the src-request */
  CACHE_PROP_ONLY_EXTERNAL     = 0x4,  /**< should only be freed if the context is external */
  CACHE_PROP_JSON              = 0x8,  /**< indicates the content is a json_ctxt and must be freed as such*/
  CACHE_PROP_INHERIT           = 0x10, /**< indicates the content will be inherited when creating sub_request*/
  CACHE_PROP_ONLY_NOT_EXTERNAL = 0x20, /**< should only be freed if the context is not external */
  CACHE_PROP_CHAIN_ID          = 0x40, /**< this cache entry holds a chain_id */
  CACHE_PROP_PAYMENT           = 0x80  /**< This cache-entry is a payment.data */
} cache_props_t;
/**
 * represents a single cache entry in a linked list.
 * These are used within a request context to cache values and automaticly free them.
 */
typedef struct cache_entry {
  bytes_t             key;       /**<  an optional key of the entry*/
  bytes_t             value;     /**< the value */
  uint8_t             buffer[4]; /**< the buffer is used to store extra data, which will be cleaned when freed. */
  cache_props_t       props;     /**< if true, the cache-entry will be freed when the request context is cleaned up. */
  struct cache_entry* next;      /**< pointer to the next entry.*/
} cache_entry_t;

/**
 * get the entry for a given key.
 */
bytes_t* in3_cache_get_entry(
    cache_entry_t* cache, /**< the root entry of the linked list. */
    bytes_t*       key    /**< the key to compare with */
);

/**
 * get the entry for a given property.
 */
cache_entry_t* in3_cache_get_entry_by_prop(
    cache_entry_t* cache, /**< the root entry of the linked list. */
    cache_props_t  prop   /**< the prop, which must match exactly */
);

/**
 * adds an entry to the linked list.
 */
cache_entry_t* in3_cache_add_entry(
    cache_entry_t** cache, /**< the root entry of the linked list. */
    bytes_t         key,   /**< an optional key */
    bytes_t         value  /**< the value of the entry */
);

/**
 * clears all entries in the linked list.
 */
void in3_cache_free(
    cache_entry_t* cache,      /**< the root entry of the linked list. */
    bool           is_external /**< true if this is the root context or an external. */
);

/**
 * adds a pointer, which should be freed when the context is freed.
 */
NONULL static inline cache_entry_t* in3_cache_add_ptr(
    cache_entry_t** cache, /**< the root entry of the linked list. */
    void*           ptr    /**< pointer to memory which shold be freed. */
) {
  return in3_cache_add_entry(cache, NULL_BYTES, bytes((uint8_t*) ptr, 1));
}

#ifdef __cplusplus
}
#endif
#endif
