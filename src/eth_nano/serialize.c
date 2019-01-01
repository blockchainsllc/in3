#include "serialize.h"
#include "rlp.h"
#include <client/context.h>
#include <client/keys.h>
#include <client/verifier.h>
#include <string.h>
#include <util/bytes.h>
#include <util/data.h>
#include <util/utils.h>

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
  rlp_add(rlp, d_get(a,K_STORAGE_HASH)       , HASH);
  rlp_add(rlp, d_get(a,K_CODE_HASH)          , HASH);
  // clang-format on
  return bb_move_to_bytes(rlp_encode_to_list(rlp));
}

bytes_t* serialize_tx(d_token_t* tx) {
  bytes_builder_t* rlp = bb_new();
  // clang-format off
  rlp_add(rlp, d_get(tx,K_NONCE)             , UINT);
  rlp_add(rlp, d_get(tx,K_GAS_PRICE)         , UINT);
  rlp_add(rlp, d_get_or(tx,K_GAS,K_GAS_LIMIT), UINT);
  rlp_add(rlp, d_get(tx,K_TO)                , ADDRESS);
  rlp_add(rlp, d_get(tx,K_VALUE)             , UINT);
  rlp_add(rlp, d_get_or(tx,K_INPUT,K_DATA)   , BYTES);
  rlp_add(rlp, d_get(tx,K_V)                 , UINT);
  rlp_add(rlp, d_get(tx,K_R)                 , UINT);
  rlp_add(rlp, d_get(tx,K_S)                 , UINT);
  // clang-format on
  return bb_move_to_bytes(rlp_encode_to_list(rlp));
}

bytes_t* serialize_block_header(d_token_t* block) {
  bytes_builder_t* rlp = bb_new();
  d_token_t *      sealed_fields, *t;
  int              i;
  // clang-format off
  rlp_add(rlp, d_get(block,K_PARENT_HASH)                    , HASH);
  rlp_add(rlp, d_get(block,K_SHA3_UNCLES)                    , HASH);
  rlp_add(rlp, d_get_or(block,K_MINER,K_COINBASE)            , ADDRESS);
  rlp_add(rlp, d_get(block,K_STATE_ROOT)                     , HASH);
  rlp_add(rlp, d_get(block,K_TRANSACTIONS_ROOT)              , HASH);
  rlp_add(rlp, d_get_or(block,K_RECEIPT_ROOT,K_RECEIPTS_ROOT), HASH);
  rlp_add(rlp, d_get(block,K_LOGS_BLOOM)                     , BLOOM);
  rlp_add(rlp, d_get(block,K_DIFFICULTY)                     , UINT);
  rlp_add(rlp, d_get(block,K_NUMBER)                         , UINT);
  rlp_add(rlp, d_get(block,K_GAS_LIMIT)                      , UINT);
  rlp_add(rlp, d_get(block,K_GAS_USED)                       , UINT);
  rlp_add(rlp, d_get(block,K_TIMESTAMP)                      , UINT);
  rlp_add(rlp, d_get(block,K_EXTRA_DATA)                     , BYTES);

  // if there are sealed field we take them as raw already rlp-encoded data and add them.
  if ((sealed_fields=d_get(block,K_SEAL_FIELDS))) {
    for (i=0,t=sealed_fields+1;i<d_len(sealed_fields);i++,t=d_next(t))
       bb_write_raw_bytes(rlp,t->data, t->len);   // we need to check if the nodes is within the bounds!
  }
  else {
    // good old proof of work...
    rlp_add(rlp, d_get(block,K_MIX_HASH)                     , HASH);
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
  if ((t = d_get_or(receipt, K_STATUS, K_ROOT)))
    rlp_add(rlp, t                                  , UINT);

  rlp_add(rlp, d_get(receipt,K_CUMULATIVE_GAS_USED) , UINT);
  rlp_add(rlp, d_get(receipt,K_LOGS_BLOOM         ) , BLOOM);

  if ((logs =  d_get(receipt,K_LOGS))) {
    // iterate over log-entries
    for (i = 0,l=logs+1; i < d_len(logs); i++, l=d_next(l)) {
      bb_clear(rlp_log);

      rlp_add(rlp_log, d_get(l,K_ADDRESS)          , ADDRESS);

      topics = d_get(l,K_TOPICS);
      bb_clear(rlp_topics);
      for (j = 0, t = topics+1; j < d_len(topics); j++, t=d_next(t))
         rlp_add( rlp_topics, t, HASH);

      rlp_encode_list(rlp_log, &rlp_topics->b);

      rlp_add(rlp_log, d_get(l,K_DATA)              ,BYTES);
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