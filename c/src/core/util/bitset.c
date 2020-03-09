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

#include "bitset.h"
#include "log.h"
#include "mem.h"
#include "utils.h"
#include <assert.h>
#include <limits.h>

#define UDIV_UP(a, b) (((a) + (b) -1) / (b))
#define ALIGN_UP(a, b) (UDIV_UP(a, b) * (b))
#define ALIGN_UP8(a) (ALIGN_UP(a, 8))
#define IS_ON_HEAP(_bs_) (_bs_->len > BS_MAX)

static bool bs_growp(bitset_t* bs, size_t pos) {
  if (bs->len > pos) return true;
  size_t   nl = ALIGN_UP8(pos + 1) / 8;
  size_t   ol = bs->len / 8;
  uint8_t* p_ = _realloc(bs->bits.p, nl, ol);
  if (p_ == NULL) return false;
  memset(p_ + ol, 0, nl - ol);
  bs->bits.p = p_;
  bs->len    = nl * 8;
  return true;
}

in3_ret_t bs_modify(bitset_t* bs, size_t pos, bs_op_t op) {
  if (pos >= BS_MAX) {
    if (IS_ON_HEAP(bs)) {
      if (!bs_growp(bs, pos)) return IN3_ENOMEM;
    } else {
      uintbs_t cpy = bs->bits.b;
      bs->bits.p   = NULL;
      if (!bs_growp(bs, pos)) return IN3_ENOMEM;
      assert(bs->bits.p != NULL);
      for (size_t i = 0; i < BS_MAX; ++i) {
        BIT_CHECK(cpy, i) ? BIT_SET(bs->bits.p[i / 8], i % 8)
                          : BIT_CLEAR(bs->bits.p[i / 8], i % 8);
      }
    }
  }
  if (IS_ON_HEAP(bs)) {
    size_t d = pos / 8;
    size_t r = pos % 8;
    switch (op) {
      case BS_SET:
        BIT_SET(bs->bits.p[d], r);
        break;
      case BS_CLEAR:
        BIT_CLEAR(bs->bits.p[d], r);
        break;
      case BS_TOGGLE:
        BIT_FLIP(bs->bits.p[d], r);
        break;
      default:
        return IN3_ENOTSUP;
    }
  } else {
    switch (op) {
      case BS_SET:
        BIT_SET(bs->bits.b, pos);
        break;
      case BS_CLEAR:
        BIT_CLEAR(bs->bits.b, pos);
        break;
      case BS_TOGGLE:
        BIT_FLIP(bs->bits.b, pos);
        break;
      default:
        return IN3_ENOTSUP;
    }
  }
  return IN3_OK;
}

bitset_t* bs_new(size_t len) {
  bitset_t* bs = _malloc(sizeof(*bs));
  if (bs == NULL) return NULL;
  if (len > BS_MAX) {
    bs->len    = ALIGN_UP8(len);
    bs->bits.p = _calloc(1, max(bs->len / 8, 1));
    if (bs->bits.p == NULL) {
      _free(bs);
      return NULL;
    }
  } else {
    bs->len    = BS_MAX;
    bs->bits.b = 0ULL;
  }
  return bs;
}

void bs_free(bitset_t* bs) {
  if (bs && IS_ON_HEAP(bs))
    _free(bs->bits.p);
  _free(bs);
}

bool bs_isset(bitset_t* bs, size_t pos) {
  if (pos >= bs->len) return false;
  if (IS_ON_HEAP(bs)) {
    return BIT_CHECK(bs->bits.p[pos / 8], pos % 8);
  } else {
    return BIT_CHECK(bs->bits.b, pos);
  }
}

bool bs_isempty(bitset_t* bs) {
  if (IS_ON_HEAP(bs)) {
    for (size_t i = 0; i < (bs->len / 8); ++i)
      if (bs->bits.p[i] != 0) return false;
    return true;
  } else {
    return bs->bits.b == 0;
  }
}

bitset_t* bs_clone(bitset_t* bs) {
  bitset_t* nbs = _malloc(sizeof(*bs));
  if (nbs == NULL) return NULL;
  if (IS_ON_HEAP(bs)) {
    nbs->bits.p = _malloc(bs->len / 8);
    if (nbs->bits.p == NULL) {
      _free(nbs);
      return NULL;
    }
    memcpy(nbs->bits.p, bs->bits.p, bs->len / 8);
  } else {
    nbs->bits.b = bs->bits.b;
  }
  nbs->len = bs->len;
  return nbs;
}

bitset_t* bs_from_ull(unsigned long long u, size_t l) {
  bitset_t* bs = bs_new(l);
  if (bs) {
    for (unsigned int j = 0; j < sizeof(u) * CHAR_BIT; ++j)
      if (BIT_CHECK(u, j))
        bs_set(bs, j);
  }
  return bs;
}
