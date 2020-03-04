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

#include "scache.h"
#include "mem.h"

bytes_t* in3_cache_get_entry(cache_entry_t* cache, bytes_t* key) {
  for (; cache; cache = cache->next) {
    if (cache->key.data && b_cmp(key, &cache->key)) return &cache->value;
  }
  return NULL;
}
void in3_cache_free(cache_entry_t* cache) {
  cache_entry_t* p = NULL;
  while (cache) {
    if (cache->key.data) _free(cache->key.data);
    if (cache->must_free)
      _free(cache->value.data);
    p     = cache;
    cache = cache->next;
    _free(p);
  }
}

cache_entry_t* in3_cache_add_entry(cache_entry_t** cache, bytes_t key, bytes_t value) {
  cache_entry_t* entry = _malloc(sizeof(cache_entry_t));
  entry->key           = key;
  entry->value         = value;
  entry->must_free     = 1;
  entry->next          = cache ? *cache : NULL;
  if (cache) *cache = entry;
  return entry;
}
