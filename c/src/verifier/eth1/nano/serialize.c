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

#include "serialize.h"
#include "../../../core/client/context.h"
#include "../../../core/client/keys.h"
#include "../../../core/client/verifier.h"
#include "../../../core/util/bytes.h"
#include "../../../core/util/data.h"
#include "../../../core/util/mem.h"
#include "../../../verifier/eth1/nano/rlp.h"
#include <string.h>

static int rlp_add_bytes(bytes_builder_t* rlp, bytes_t b, int ml) {
  // if this is a unit we need to make sure we remove the leading zeros.
  while (ml == 0 && b.len > 1 && *b.data == 0) {
    b.len--;
    b.data++;
  }

  if (ml == 0 && b.len == 1 && b.data[0] == 0) b.len = 0;
  if (ml < 0) {
    if (b.len == 0)
      ml = 0;
    else
      ml = -ml;
  }

  if ((size_t) ml > b.len) {
    // we need to fill left
    uint8_t* tmp = _calloc(ml, 1);
    memcpy(tmp + ml - b.len, b.data, b.len);
    b.data = tmp;
    b.len  = ml;
    rlp_encode_item(rlp, &b);
    _free(tmp);
  } else
    rlp_encode_item(rlp, &b);

  return 0;
}

int rlp_add(bytes_builder_t* rlp, d_token_t* t, int ml) {
  uint8_t tmp[4];
  bytes_t b;

  switch (d_type(t)) {
    case T_NULL:
      b.data = tmp;
      b.len  = 0;
      break;
    case T_INTEGER:
      tmp[3] = t->len & 0xFF;
      tmp[2] = (t->len & 0xFF00) >> 8;
      tmp[1] = (t->len & 0xFF0000) >> 16;
      tmp[0] = (t->len & 0xF000000) >> 24;
      b.len  = tmp[0] ? 4 : (tmp[1] ? 3 : (tmp[2] ? 2 : (tmp[3] ? 1 : 0)));
      b.data = (uint8_t*) &tmp + 4 - b.len;
      break;
    case T_BYTES:
      b.data = t->data;
      b.len  = t->len;
      break;
    default:
      return -1;
  }

  return rlp_add_bytes(rlp, b, ml);
}

#define UINT 0
#define BYTES -1
#define ADDRESS -20
#define HASH 32
#define BLOOM 256

bytes_t* serialize_account(d_token_t* a) {
  bytes_builder_t* rlp = bb_new();
  // clang-format off
  rlp_add(rlp, d_get(a,K_NONCE)              , UINT);
  rlp_add(rlp, d_get(a,K_BALANCE)            , UINT);
  rlp_add(rlp, d_getl(a,K_STORAGE_HASH, 32)  , HASH);
  rlp_add(rlp, d_getl(a,K_CODE_HASH, 32)     , HASH);
  // clang-format on
  return bb_move_to_bytes(rlp_encode_to_list(rlp));
}

bytes_t* serialize_tx(d_token_t* tx) {
  bytes_builder_t* rlp = bb_new();
  // clang-format off
  rlp_add(rlp, d_get(tx,K_NONCE)             , UINT);
  rlp_add(rlp, d_get(tx,K_GAS_PRICE)         , UINT);
  rlp_add(rlp, d_get_or(tx,K_GAS,K_GAS_LIMIT), UINT);
  rlp_add(rlp, d_getl(tx,K_TO, 20)           , ADDRESS);
  rlp_add(rlp, d_get(tx,K_VALUE)             , UINT);
  rlp_add(rlp, d_get_or(tx,K_INPUT,K_DATA)   , BYTES);
  rlp_add(rlp, d_get(tx,K_V)                 , UINT);
  rlp_add(rlp, d_getl(tx,K_R, 32)            , UINT);
  rlp_add(rlp, d_getl(tx,K_S, 32)            , UINT);
  // clang-format on
  return bb_move_to_bytes(rlp_encode_to_list(rlp));
}

bytes_t* serialize_tx_raw(bytes_t nonce, bytes_t gas_price, bytes_t gas_limit, bytes_t to, bytes_t value, bytes_t data, uint64_t v, bytes_t r, bytes_t s) {
  bytes_builder_t* rlp = bb_new();
  // clang-format off
  rlp_add_bytes(rlp, nonce             , UINT);
  rlp_add_bytes(rlp, gas_price         , UINT);
  rlp_add_bytes(rlp, gas_limit         , UINT);
  rlp_add_bytes(rlp, to                , ADDRESS);
  rlp_add_bytes(rlp, value             , UINT);
  rlp_add_bytes(rlp, data              , BYTES);
  if (v) {
     uint8_t tmp[8],*p=tmp,l=8;
     long_to_bytes(v,tmp);
     optimize_len(p,l);
    rlp_add_bytes(rlp, bytes(p,l)        , UINT);
    rlp_add_bytes(rlp, r                 , UINT);
    rlp_add_bytes(rlp, s                 , UINT);
  }

  // clang-format on
  return bb_move_to_bytes(rlp_encode_to_list(rlp));
}

bytes_t* serialize_block_header(d_token_t* block) {
  bytes_builder_t* rlp = bb_new();
  d_token_t *      sealed_fields, *t;
  int              i;
  // clang-format off
  rlp_add(rlp, d_getl(block,K_PARENT_HASH, 32)      , HASH);
  rlp_add(rlp, d_get(block,K_SHA3_UNCLES)           , HASH);
  
  if ((t = d_getl(block, K_MINER, 20)) || (t = d_getl(block, K_COINBASE, 20)))
    rlp_add(rlp, t                                  , ADDRESS);  

  rlp_add(rlp, d_getl(block,K_STATE_ROOT, 32)       , HASH);
  rlp_add(rlp, d_getl(block,K_TRANSACTIONS_ROOT, 32), HASH);

  if ((t = d_getl(block, K_RECEIPT_ROOT, 32)) || (t = d_getl(block, K_RECEIPTS_ROOT, 32)))
    rlp_add(rlp, t                                  , HASH);

  rlp_add(rlp, d_getl(block,K_LOGS_BLOOM, 256)      , BLOOM);
  rlp_add(rlp, d_get(block,K_DIFFICULTY)            , UINT);
  rlp_add(rlp, d_get(block,K_NUMBER)                , UINT);
  rlp_add(rlp, d_get(block,K_GAS_LIMIT)             , UINT);
  rlp_add(rlp, d_get(block,K_GAS_USED)              , UINT);
  rlp_add(rlp, d_get(block,K_TIMESTAMP)             , UINT);
  rlp_add(rlp, d_get(block,K_EXTRA_DATA)            , BYTES);

  // if there are sealed field we take them as raw already rlp-encoded data and add them.
  if ((sealed_fields=d_get(block,K_SEAL_FIELDS))) {
    for (i=0,t=sealed_fields+1;i<d_len(sealed_fields);i++,t=d_next(t))
       bb_write_raw_bytes(rlp,t->data, t->len);   // we need to check if the nodes is within the bounds!
  }
  else {
    // good old proof of work...
    rlp_add(rlp, d_getl(block,K_MIX_HASH, 32)                     , HASH);
    rlp_add(rlp, d_get(block,K_NONCE)                        , BYTES);
  }
  // clang-format on
  return bb_move_to_bytes(rlp_encode_to_list(rlp));
}

bytes_t* serialize_tx_receipt(d_token_t* receipt) {
  bytes_builder_t* bb          = bb_new();
  bytes_builder_t* rlp         = bb_new();
  bytes_builder_t* rlp_log     = bb_new();
  bytes_builder_t* rlp_topics  = bb_new();
  bytes_builder_t* rlp_loglist = bb_new();
  d_token_t *      t, *logs, *l, *topics;
  int              i, j;

  // clang-format off
  // we only add it if it exists since this EIP came later.
  if ((t = d_get(receipt, K_STATUS)) || (t = d_getl(receipt, K_ROOT, 32)))
    rlp_add(rlp, t                                 , UINT);

  rlp_add(rlp, d_get(receipt,K_CUMULATIVE_GAS_USED), UINT);
  rlp_add(rlp, d_getl(receipt,K_LOGS_BLOOM, 256)   , BLOOM);

  if ((logs =  d_get(receipt,K_LOGS))) {
    // iterate over log-entries
    for (i = 0,l=logs+1; i < d_len(logs); i++, l=d_next(l)) {
      bb_clear(rlp_log);

      rlp_add(rlp_log, d_getl(l, K_ADDRESS, 20)    , ADDRESS);

      topics = d_get(l,K_TOPICS);
      bb_clear(rlp_topics);
      for (j = 0, t = topics+1; j < d_len(topics); j++, t=d_next(t))
         rlp_add( rlp_topics, t, HASH);

      rlp_encode_list(rlp_log, &rlp_topics->b);

      rlp_add(rlp_log, d_get(l,K_DATA)             ,BYTES);
      rlp_encode_list(rlp_loglist, &rlp_log->b);
    }

  }

  // clang-format on
  rlp_encode_list(rlp, &rlp_loglist->b);
  rlp_encode_to_list(rlp);

  bb_free(bb);
  bb_free(rlp_log);
  bb_free(rlp_topics);
  bb_free(rlp_loglist);
  return bb_move_to_bytes(rlp);
}