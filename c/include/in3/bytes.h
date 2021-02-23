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

#ifndef BYTES_H
#define BYTES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mem.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
/** creates a new bytes_builder with a initial size of 32 bytes */
#define bb_new()                   bb_newl(32)
#define bb_read(_bb_, _i_, _vptr_) bb_readl((_bb_), (_i_), (_vptr_), sizeof(*_vptr_))
#define bb_read_next(_bb_, _iptr_, _vptr_)      \
  do {                                          \
    size_t _l_ = sizeof(*_vptr_);               \
    bb_readl((_bb_), *(_iptr_), (_vptr_), _l_); \
    *(_iptr_) += _l_;                           \
  } while (0)
#define bb_readl(_bb_, _i_, _vptr_, _l_) memcpy((_vptr_), (_bb_)->b.data + (_i_), _l_)
#define b_read(_b_, _i_, _vptr_)         b_readl((_b_), (_i_), _vptr_, sizeof(*_vptr_))
#define b_readl(_b_, _i_, _vptr_, _l_)   memcpy(_vptr_, (_b_)->data + (_i_), (_l_))

typedef uint8_t address_t[20]; /**< pointer to a 20byte address */
typedef uint8_t bytes32_t[32]; /**< pointer to a 32byte word */
#ifdef ESP_IDF
typedef uint8_t wlen_t; /**< number of bytes within a word (min 1byte but usually a uint) */
#else
typedef uint_fast8_t wlen_t; /**< number of bytes within a word (min 1byte but usually a uint) */
#endif
/** a byte array */
typedef struct bytes {
  uint8_t* data; /**< the byte-data  */
  uint32_t len;  /**< the length of the array ion bytes */
} bytes_t;

/** a byte-buffer to attach byte-functions. */
typedef struct {
  size_t  bsize; /**< size of the currently allocated bytes */
  bytes_t b;     /**< the bytes struct */
} bytes_builder_t;

RETURNS_NONULL bytes_t* b_new(const uint8_t* data, uint32_t len);                                               /**< allocates a new byte array with 0 filled */
NONULL uint8_t* b_get_data(const bytes_t* b);                                                                   /**< gets the data field from an input byte array */
NONULL uint32_t b_get_len(const bytes_t* b);                                                                    /**< gets the len field from an input byte array */
NONULL void     b_print(const bytes_t* a);                                                                      /**< prints a the bytes as hex to stdout */
NONULL void     ba_print(const uint8_t* a, size_t l);                                                           /**< prints a the bytes as hex to stdout */
NONULL int      b_cmp(const bytes_t* a, const bytes_t* b);                                                      /**< compares 2 byte arrays and returns 1 for equal and 0 for not equal*/
int             bytes_cmp(const bytes_t a, const bytes_t b);                                                    /**< compares 2 byte arrays and returns 1 for equal and 0 for not equal*/
void            b_free(bytes_t* a);                                                                             /**< frees the data */
bytes_t         b_concat(int cnt, ...);                                                                         /**< duplicates the content of bytes*/
NONULL bytes_t* b_dup(const bytes_t* a);                                                                        /**< clones a byte array*/
NONULL bytes_t  bytes_dup(const bytes_t a);                                                                     /**< clones a byte array*/
NONULL uint8_t  b_read_byte(bytes_t* b, size_t* pos);                                                           /**< reads a byte on the current position and updates the pos afterwards. */
NONULL uint32_t b_read_int(bytes_t* b, size_t* pos);                                                            /**< reads a integer on the current position and updates the pos afterwards. */
NONULL uint64_t b_read_long(bytes_t* b, size_t* pos);                                                           /**< reads a long on the current position and updates the pos afterwards. */
NONULL char*    b_new_chars(bytes_t* b, size_t* pos);                                                           /**< creates a new string (needs to be freed) on the current position and updates the pos afterwards. */
NONULL bytes_t*       b_new_fixed_bytes(bytes_t* b, size_t* pos, int len);                                      /**< reads bytes with a fixed length on the current position and updates the pos afterwards. */
bytes_builder_t*      bb_newl(size_t l);                                                                        /**< creates a new bytes_builder */
NONULL void           bb_free(bytes_builder_t* bb);                                                             /**< frees a bytebuilder and its content. */
NONULL int            bb_check_size(bytes_builder_t* bb, size_t len);                                           /**< internal helper to increase the buffer if needed */
NONULL void           bb_write_chars(bytes_builder_t* bb, char* c, int len);                                    /**< writes a string to the builder. */
NONULL void           bb_write_dyn_bytes(bytes_builder_t* bb, const bytes_t* src);                              /**< writes bytes to the builder with a prefixed length. */
NONULL void           bb_write_fixed_bytes(bytes_builder_t* bb, const bytes_t* src);                            /**< writes fixed bytes to the builder. */
NONULL void           bb_write_int(bytes_builder_t* bb, uint32_t val);                                          /**< writes a ineteger to the builder. */
NONULL void           bb_write_long(bytes_builder_t* bb, uint64_t val);                                         /**< writes s long to the builder. */
NONULL void           bb_write_long_be(bytes_builder_t* bb, uint64_t val, int len);                             /**< writes any integer value with the given length of bytes */
NONULL void           bb_write_byte(bytes_builder_t* bb, uint8_t val);                                          /**< writes a single byte to the builder. */
NONULL void           bb_write_raw_bytes(bytes_builder_t* bb, void* ptr, size_t len);                           /**< writes the bytes to the builder. */
NONULL void           bb_clear(bytes_builder_t* bb);                                                            /**< resets the content of the builder. */
NONULL void           bb_replace(bytes_builder_t* bb, int offset, int delete_len, uint8_t* data, int data_len); /**< replaces or deletes a part of the content. */
RETURNS_NONULL NONULL bytes_t* bb_move_to_bytes(bytes_builder_t* bb);                                           /**< frees the builder and moves the content in a newly created bytes struct (which needs to be freed later). */
NONULL uint64_t                bb_read_long(bytes_builder_t* bb, size_t* i);                                    /**< reads a long from the builder */
NONULL uint32_t                bb_read_int(bytes_builder_t* bb, size_t* i);                                     /**< reads a int from the builder */

static inline bytes_t     bytes(uint8_t* a, uint32_t len) { return (bytes_t){.data = a, .len = len}; } /**< converts the given bytes to a bytes struct */
bytes_t                   cloned_bytes(bytes_t data);                                                  /**< cloned the passed data*/
NONULL static inline void b_optimize_len(bytes_t* b) {                                                 /**< changed the data and len to remove leading 0-bytes */
  while (b->len > 1 && *b->data == 0) {
    b->data++;
    b->len--;
  }
}

#define b_to_stack(d)              \
  {                                \
    bytes_t o = d;                 \
    d.data    = alloca(d.len);     \
    memcpy(d.data, o.data, o.len); \
    _free(o.data);                 \
  } /**< converts bytes from heap to stack */

#ifdef __cplusplus
}
#endif
#endif
