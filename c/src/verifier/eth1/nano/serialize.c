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

#include "serialize.h"
#include "../../../core/client/keys.h"
#include "../../../core/client/plugin.h"
#include "../../../core/client/request.h"
#include "../../../core/util/bytes.h"
#include "../../../core/util/data.h"
#include "../../../core/util/mem.h"
#include "../../../verifier/eth1/nano/rlp.h"
#include <string.h>

static int rlp_add_bytes(bytes_builder_t* rlp, bytes_t b, int ml) {
  // if this is a unit we need to make sure we remove the leading zeros.
  if (!b.data) {
    b.len  = 0;
    b.data = (uint8_t*) &b;
  }
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
  }
  else
    rlp_encode_item(rlp, &b);

  return 0;
}

int rlp_add(bytes_builder_t* rlp, d_token_t* t, int ml) {
  uint8_t tmp[4] = {0};
  bytes_t b      = d_bytes(t);
  if (d_type(t) == T_NULL) b = bytes(tmp, 0);
  return b.data || d_type(t) == T_BYTES ? rlp_add_bytes(rlp, b, ml) : -1;
}

static bytes_t* convert_to_typed_list(bytes_builder_t* rlp, int32_t type) {
  rlp_encode_to_list(rlp);
  if (type) {
    bb_check_size(rlp, 1);
    memmove(rlp->b.data + 1, rlp->b.data, rlp->b.len);
    rlp->b.len++;
    rlp->b.data[0] = (uint8_t) type;
  }
  return bb_move_to_bytes(rlp);
}

#define UINT    0
#define BYTES   -1
#define ADDRESS -20
#define HASH    32
#define BLOOM   256

static void rlp_add_list(bytes_builder_t* rlp, d_token_t* t) {

  bytes_builder_t bb1 = {0}, bb2 = {0}, bb3 = {0};
  for (d_iterator_t adr = d_iter(t); adr.left && d_len(adr.token) == 2 && d_type(adr.token) == T_OBJECT; d_iter_next(&adr)) {
    bb_clear(&bb2);
    bb_clear(&bb3);
    rlp_add(&bb2, d_get(adr.token, K_ADDRESS), ADDRESS);
    d_token_t* keys = d_get(adr.token, K_STORAGE_KEYS);
    if (!keys) keys = d_get(adr.token, K_STORAGE_PROOF);
    if (keys) {
      for (d_iterator_t st = d_iter(keys); st.left && d_is_bytes(st.token); d_iter_next(&st)) {
        d_token_t* h = st.token;
        if (d_type(h) == T_OBJECT) h = d_get(h, K_KEY);
        rlp_add(&bb3, h, HASH);
      }
    }
    rlp_encode_list(&bb2, &bb3.b);
    rlp_encode_list(&bb1, &bb2.b);
  }
  rlp_encode_list(rlp, &bb1.b);
  _free(bb1.b.data);
  _free(bb2.b.data);
  _free(bb3.b.data);
}

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
  bytes_builder_t* rlp  = bb_new();
  int32_t          type = d_get_int(tx, K_TYPE);
  switch (type) {
    case 0: // legacy tx
      rlp_add(rlp, d_get(tx, K_NONCE), UINT);
      rlp_add(rlp, d_get(tx, K_GAS_PRICE), UINT);
      rlp_add(rlp, d_get_or(tx, K_GAS, K_GAS_LIMIT), UINT);
      rlp_add(rlp, d_getl(tx, K_TO, 20), ADDRESS);
      rlp_add(rlp, d_get(tx, K_VALUE), UINT);
      rlp_add(rlp, d_get_or(tx, K_INPUT, K_DATA), BYTES);
      break;

    case 1: // EIP 2930
      rlp_add(rlp, d_get(tx, K_CHAIN_ID), UINT);
      rlp_add(rlp, d_get(tx, K_NONCE), UINT);
      rlp_add(rlp, d_get(tx, K_GAS_PRICE), UINT);
      rlp_add(rlp, d_get_or(tx, K_GAS, K_GAS_LIMIT), UINT);
      rlp_add(rlp, d_getl(tx, K_TO, 20), ADDRESS);
      rlp_add(rlp, d_get(tx, K_VALUE), UINT);
      rlp_add(rlp, d_get_or(tx, K_INPUT, K_DATA), BYTES);
      rlp_add_list(rlp, d_get(tx, K_ACCESS_LIST));
      break;

    case 2: // EIP 1559
      rlp_add(rlp, d_get(tx, K_CHAIN_ID), UINT);
      rlp_add(rlp, d_get(tx, K_NONCE), UINT);
      rlp_add(rlp, d_get(tx, K_MAX_PRIORITY_FEE_PER_GAS), UINT);
      rlp_add(rlp, d_get(tx, K_MAX_FEE_PER_GAS), UINT);
      rlp_add(rlp, d_get_or(tx, K_GAS, K_GAS_LIMIT), UINT);
      rlp_add(rlp, d_getl(tx, K_TO, 20), ADDRESS);
      rlp_add(rlp, d_get(tx, K_VALUE), UINT);
      rlp_add(rlp, d_get_or(tx, K_INPUT, K_DATA), BYTES);
      rlp_add_list(rlp, d_get(tx, K_ACCESS_LIST));
      break;
  }

  rlp_add(rlp, d_get(tx, K_V), UINT);
  rlp_add(rlp, d_getl(tx, K_R, 32), UINT);
  rlp_add(rlp, d_getl(tx, K_S, 32), UINT);

  return convert_to_typed_list(rlp, type);
}

bytes_t serialize_tx_raw(eth_tx_data_t* tx, uint64_t v, bytes_t r, bytes_t s) {
  bytes_builder_t* rlp = bb_new();
  uint8_t          chain_tmp[8];
  long_to_bytes(tx->chain_id, chain_tmp);

  // clang-format off
  switch (tx->type) {
    case 0: // legacy tx
      rlp_add_bytes(rlp, tx->nonce             , UINT);
      rlp_add_bytes(rlp, tx->gas_price         , UINT);
      rlp_add_bytes(rlp, tx->gas_limit         , UINT);
      rlp_add_bytes(rlp, tx->to                , ADDRESS);
      rlp_add_bytes(rlp, tx->value             , UINT);
      rlp_add_bytes(rlp, tx->data              , BYTES);
      break;

    case 1: // EIP 2930
      rlp_add_bytes(rlp, bytes(chain_tmp,8), UINT);
      rlp_add_bytes(rlp, tx->nonce             , UINT);
      rlp_add_bytes(rlp, tx->gas_price         , UINT);
      rlp_add_bytes(rlp, tx->gas_limit         , UINT);
      rlp_add_bytes(rlp, tx->to                , ADDRESS);
      rlp_add_bytes(rlp, tx->value             , UINT);
      rlp_add_bytes(rlp, tx->data              , BYTES);
      rlp_add_list(rlp, tx->access_list);
      break;

    case 2: // EIP 1559
      rlp_add_bytes(rlp, bytes(chain_tmp,8)      , UINT);
      rlp_add_bytes(rlp, tx->nonce                   , UINT);
      rlp_add_bytes(rlp, tx->max_priority_fee_per_gas, UINT);
      rlp_add_bytes(rlp, tx->max_fee_per_gas         , UINT);
      rlp_add_bytes(rlp, tx->gas_limit               , UINT);
      rlp_add_bytes(rlp, tx->to                      , ADDRESS);
      rlp_add_bytes(rlp, tx->value                   , UINT);
      rlp_add_bytes(rlp, tx->data                    , BYTES);
      rlp_add_list(rlp, tx->access_list);
      break;
  }

  if (v) {
     uint8_t tmp[8],*p=tmp,l=8;
     long_to_bytes(v,tmp);
     optimize_len(p,l);
    rlp_add_bytes(rlp, bytes(p,l)        , UINT);
    rlp_add_bytes(rlp, r                 , UINT);
    rlp_add_bytes(rlp, s                 , UINT);
  }

  // clang-format on
  bytes_t* res    = convert_to_typed_list(rlp, tx->type);
  bytes_t  result = *res;
  _free(res);
  return result;
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
    for (i=0,t=d_get_at(sealed_fields,0);i<d_len(sealed_fields);i++,t=d_next(t)) {
      bytes_t b = d_bytes(t);
      bb_write_raw_bytes(rlp,b.data, b.len);   // we need to check if the nodes is within the bounds!
    }
  }
  else {
    // good old proof of work...
    rlp_add(rlp, d_getl(block,K_MIX_HASH, 32)       , HASH);
    rlp_add(rlp, d_get(block,K_NONCE)               , BYTES);
    // if we have a base gas fee we need to use EIP-1559
  }
  if (d_get_long(block,K_BASE_GAS_FEE)) 
      rlp_add(rlp, d_get(block,K_BASE_GAS_FEE)      , UINT);
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
    for (i = 0,l=d_get_at(logs,0); i < d_len(logs); i++, l=d_next(l)) {
      bb_clear(rlp_log);

      rlp_add(rlp_log, d_getl(l, K_ADDRESS, 20)    , ADDRESS);

      topics = d_get(l,K_TOPICS);
      bb_clear(rlp_topics);
      for (j = 0, t = d_get_at(topics,0); j < d_len(topics); j++, t=d_next(t))
         rlp_add( rlp_topics, t, HASH);

      rlp_encode_list(rlp_log, &rlp_topics->b);

      rlp_add(rlp_log, d_get(l,K_DATA)             ,BYTES);
      rlp_encode_list(rlp_loglist, &rlp_log->b);
    }

  }

  // clang-format on
  rlp_encode_list(rlp, &rlp_loglist->b);
  bb_free(bb);
  bb_free(rlp_log);
  bb_free(rlp_topics);
  bb_free(rlp_loglist);
  return convert_to_typed_list(rlp, d_get_int(receipt, K_TYPE));
}