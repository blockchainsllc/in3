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

#ifndef IN3_BITSET_H
#define IN3_BITSET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "error.h"

#ifndef BS_MAX
#define BS_MAX 64
#endif

#if BS_MAX == 64
typedef uint64_t uintbs_t;
#elif BS_MAX == 32
typedef uint32_t uintbs_t;
#else
#error "Unsupported value for BS_MAX"
#endif

// Helper macros
#define BIT_SET(_a_, _b_) ((_a_) |= (1ULL << (_b_)))
#define BIT_CLEAR(_a_, _b_) ((_a_) &= ~(1ULL << (_b_)))
#define BIT_FLIP(_a_, _b_) ((_a_) ^= (1ULL << (_b_)))
#define BIT_CHECK(_a_, _b_) (!!((_a_) & (1ULL << (_b_))))
#define BITMASK_SET(_x_, _y_) ((_x_) |= (_y_))
#define BITMASK_CLEAR(_x_, _y_) ((_x_) &= (~(_y_)))
#define BITMASK_FLIP(_x_, _y_) ((_x_) ^= (_y_))
#define BITMASK_CHECK_ALL(_x_, _y_) (((_x_) & (_y_)) == (_y_))
#define BITMASK_CHECK_ANY(_x_, _y_) ((_x_) & (_y_))
#define BITMASK_SET_BOOL(_x_, _y_, _v_) \
  if ((((_x_) & (_y_)) != 0) != (_v_)) ((_x_) ^= (_y_))
#define BITS_MSB(x, b) ((x) >> (b))
#define BITS_LSB(x, b) ((x) << (b))

#define bs_set(_bs_, _pos_) bs_modify(_bs_, _pos_, BS_SET)
#define bs_clear(_bs_, _pos_) bs_modify(_bs_, _pos_, BS_CLEAR)
#define bs_toggle(_bs_, _pos_) bs_modify(_bs_, _pos_, BS_TOGGLE)

typedef struct {
  union {
    uintbs_t b;
    uint8_t* p;
  } bits;
  size_t len; // length in bits, guaranteed to be multiple of 8
} bitset_t;

typedef enum {
  BS_SET = 0,
  BS_CLEAR,
  BS_TOGGLE
} bs_op_t;

bitset_t* bs_new(size_t len);
void      bs_free(bitset_t* bs);
bool      bs_isset(bitset_t* bs, size_t pos);
in3_ret_t bs_modify(bitset_t* bs, size_t pos, bs_op_t op); // will reallocate if pos is greater than BS_MAX and initial length
bool      bs_isempty(bitset_t* bs);
bitset_t* bs_clone(bitset_t* bs);
bitset_t* bs_from_ull(unsigned long long u, size_t l);

#endif //IN3_BITSET_H
