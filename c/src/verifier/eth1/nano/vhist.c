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
#ifdef POA
#include "vhist.h"
#include "../../../core/client/keys.h"
#include "../../../core/util/log.h"
#include "../../../core/util/mem.h"
#include "../../../core/util/utils.h"
#include "../../../verifier/eth1/nano/rlp.h"
#include "../../../verifier/eth1/nano/serialize.h"
#include <stdbool.h>
#include <string.h>

#define VALIDATOR_LIST_KEY "validatorlist_%d"

static in3_ret_t bb_find(bytes_builder_t* bb, uint8_t* v, size_t l) {
  if (v) {
    for (size_t i = 0; i < bb->b.len; i += l)
      if (!memcmp(bb->b.data + i, v, l))
        return (i / l);
  }
  return IN3_EFIND;
}

static vhist_engine_t stoengine(const char* str) {
  vhist_engine_t vhe = ENGINE_UNKNOWN;
  if (!str)
    vhe = ENGINE_UNKNOWN;
  else if (!strcmp(str, "authorityRound"))
    vhe = ENGINE_AURA;
  else if (!strcmp(str, "clique"))
    vhe = ENGINE_CLIQUE;
  return vhe;
}

vhist_t* vh_new() {
  vhist_t* vh = _malloc(sizeof(*vh));
  if (vh == NULL) return NULL;
  vh->vldtrs = bb_new();
  vh->diffs  = bb_new();
  if (!vh->vldtrs || !vh->diffs) {
    _free(vh);
    _free(vh->vldtrs);
    _free(vh->diffs);
    return NULL;
  }
  return vh;
}

vhist_t* vh_init_spec(d_token_t* spec) {
  if (spec == NULL || d_type(spec) != T_ARRAY) return NULL;
  vhist_t* vh = vh_new();
  if (vh == NULL) return NULL;

  d_iterator_t sitr;
  for (sitr = d_iter(spec); sitr.left; d_iter_next(&sitr)) {
    vh->last_change_block = d_get_longk(sitr.token, K_BLOCK);
    vh_add_state(vh, sitr.token, true);
  }
  return vh;
}

vhist_t* vh_init_nodelist(d_token_t* nodelist) {
  if (nodelist == NULL) return NULL;
  d_token_t* ss = d_get(nodelist, K_STATES);
  if (ss == NULL) return NULL;

  vhist_t* vh = vh_new();
  if (vh == NULL) return NULL;

  d_iterator_t sitr;
  for (sitr = d_iter(ss); sitr.left; d_iter_next(&sitr)) {
    vh->last_change_block = d_get_longk(sitr.token, K_BLOCK);
    vh_add_state(vh, sitr.token, false);
  }
  return vh;
}

void vh_free(vhist_t* vh) {
  if (vh) {
    bb_free(vh->diffs);
    bb_free(vh->vldtrs);
  }
  _free(vh);
}

uint32_t vh_find_block(vhist_t* vh, uint64_t block, uint32_t* prevsz) {
  uint64_t       blk    = 0;
  uint32_t       sz     = 0;
  size_t         i      = 0;
  vhist_engine_t engine = ENGINE_UNKNOWN;

  for (i = 0; i < vh->diffs->b.len;) {
    bb_read_next(vh->diffs, &i, &blk);
    bb_read_next(vh->diffs, &i, &engine);
    bb_read_next(vh->diffs, &i, &sz);
    if (blk > block) {
      i -= ((*prevsz * 4) + 12 + sizeof(engine));
      break;
    }
    i += sz * 4;
    *prevsz = sz;
  }

  // If block exceeds last block at which there was a validator list change,
  // send latest validator list
  if (i >= vh->diffs->b.len) i -= (*prevsz) * 4;
  return i;
}

bytes_builder_t* vh_get_validators_for_block(vhist_t* vh, uint64_t block) {
  bytes_builder_t* bb  = bb_new();
  uint32_t         pos = 0, prevsz = 0;
  if (bb == NULL) return NULL;
  uint32_t i = vh_find_block(vh, block, &prevsz);
  for (size_t j = 0; j < prevsz; ++j) {
    bb_read_next(vh->diffs, &i, &pos);
    bb_write_raw_bytes(bb, vh->vldtrs->b.data + (pos * 20), 20);
  }
  return bb;
}

vhist_engine_t vh_get_engine_for_block(vhist_t* vh, uint64_t block) {
  vhist_engine_t engine = ENGINE_UNKNOWN;
  uint32_t       prevsz = 0;
  uint32_t       i      = vh_find_block(vh, block, &prevsz);
  i -= (4 + sizeof(vhist_engine_t));
  bb_read_next(vh->diffs, &i, &engine);
  return engine;
}

void vh_add_state(vhist_t* vh, d_token_t* state, bool is_spec) {
  bytes_t*       b;
  in3_ret_t      ret;
  uint64_t       blk    = 0;
  d_token_t*     vs     = NULL;
  vhist_engine_t engine = ENGINE_UNKNOWN;

  vs     = d_get(state, is_spec ? K_LIST : K_VALIDATORS);
  engine = stoengine(d_get_stringk(state, K_ENGINE));

  d_token_t* tmp;
  if ((tmp = d_get(state, K_BYPASS_FINALITY))) {
    blk = d_long(tmp);
  } else if ((tmp = d_get(d_get(state, K_PROOF), K_FINALITY_BLOCKS)) && d_len(tmp)) {
    b = d_get_bytes_at(tmp, d_len(tmp) - 1);
    rlp_decode_in_list(b, BLOCKHEADER_NUMBER, b);
    blk = bytes_to_long(b->data, b->len);
  } else
    blk = d_get_longk(state, K_BLOCK);

  bb_write_long(vh->diffs, blk);
  bb_write_raw_bytes(vh->diffs, &engine, sizeof(engine));
  bb_write_int(vh->diffs, d_len(vs));
  if (d_type(vs) == T_ARRAY) {
    for (d_iterator_t vitr = d_iter(vs); vitr.left; d_iter_next(&vitr)) {
      b   = (d_type(vitr.token) == T_STRING) ? hex_to_new_bytes(d_string(vitr.token), 40) : d_bytesl(vitr.token, 20);
      ret = bb_find(vh->vldtrs, b->data, 20);
      if (ret == IN3_EFIND) {
        bb_write_int(vh->diffs, vh->vldtrs->b.len / 20);
        bb_write_fixed_bytes(vh->vldtrs, b);
      } else {
        bb_write_int(vh->diffs, ret);
      }
      if (d_type(vitr.token) == T_STRING) b_free(b);
    }
  }
}

void vh_cache_save(vhist_t* vh, in3_t* c) {
  if (!c->cache) return;
  char             k[35];
  bytes_builder_t* cbb  = bb_new();
  uint8_t          vers = 1;
  bytes_t          b    = {.data = &vers, .len = sizeof(vers)};
  rlp_encode_item(cbb, &b); // Version flag
  rlp_encode_item(cbb, &vh->diffs->b);
  rlp_encode_item(cbb, &vh->vldtrs->b);
  b.data = (uint8_t*) &vh->last_change_block;
  b.len  = sizeof(vh->last_change_block);
  rlp_encode_item(cbb, &b);
  sprintf(k, VALIDATOR_LIST_KEY, c->chain_id);
  c->cache->set_item(c->cache->cptr, k, &cbb->b);
  bb_free(cbb);
}

vhist_t* vh_cache_retrieve(in3_t* c) {
  char     k[35];
  bytes_t *v_ = NULL, b_;
  vhist_t* vh = NULL;
  if (c->cache) {
    sprintf(k, VALIDATOR_LIST_KEY, c->chain_id);
    v_ = c->cache->get_item(c->cache->cptr, k);
    if (v_) {
      rlp_decode(v_, 0, &b_);
      uint8_t vers;
      b_read(&b_, 0, &vers);
      if (vers == 1) {
        vh = vh_new();
        if (vh == NULL) return NULL;
        rlp_decode(v_, 1, &b_);
        bb_write_raw_bytes(vh->diffs, b_.data, b_.len);
        vh->diffs->b.len = b_.len;
        rlp_decode(v_, 2, &b_);
        bb_write_raw_bytes(vh->vldtrs, b_.data, b_.len);
        vh->vldtrs->b.len = b_.len;
        rlp_decode(v_, 3, &b_);
        b_read(&b_, 0, &vh->last_change_block);
      }
      b_free(v_);
    }
  }
  return vh;
}
#endif