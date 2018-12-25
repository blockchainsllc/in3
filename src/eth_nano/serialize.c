#include <util/bytes.h>
#include <client/context.h>
#include <string.h>
#include <client/verifier.h>
#include "serialize.h"
#include "rlp.h"
#include <util/utils.h>
#include <util/data.h>
#include <client/keys.h>

int rlp_add(bytes_builder_t *rlp, d_token_t* t, int ml) {
  uint8_t tmp[4];
  bytes_t b;

  switch (d_type(t)) {
    case T_NULL:
      b.data=tmp;
      b.len=0;
      break;
    case T_INTEGER:
      tmp[3] = t->len & 0xFF;
      tmp[2] = (t->len & 0xFF00)>>8;
      tmp[1] = (t->len & 0xFF0000)>>16;
      tmp[0] = (t->len & 0xF000000)>>24;
      b.data= (uint8_t*) &tmp;
      b.len = tmp[0] ? 4 : (tmp[1] ? 3 : (tmp[2] ? 2 : (tmp[3] ? 1:0)));
      break;
    case T_BYTES:
      b.data = t->data;
      b.len  = t->len;
      break;
    default:
      return -1;
  }

  if (ml==0 && b.len==1 && b.data[0]==0) b.len=0;
  if (ml<0) {
    if (b.len==0)
       ml=0;
    else
       ml=-ml;
  }

  if (ml>b.len) {
    // we need to fill left
    uint8_t* tmp = _calloc(ml,1);
    memcpy(tmp+ ml-b.len,b.data,b.len);
    b.data = tmp;
    b.len  = ml;
    rlp_encode_item(rlp,&b);
    _free(tmp);
  }
  else
    rlp_encode_item(rlp,&b);

  return 0;
}

#define UINT 0 
#define BYTES -1
#define ADDRESS -20
#define HASH 32
#define BLOOM 256



bytes_t *serialize_tx(in3_vctx_t *vc, d_token_t *tx) {
  bytes_builder_t *rlp = bb_new();
  bytes_builder_t *bb  = bb_new();

  rlp_add(rlp, d_get(tx,K_NONCE)             , UINT);
  rlp_add(rlp, d_get(tx,K_GAS_PRICE)         , UINT);
  rlp_add(rlp, d_get_or(tx,K_GAS,K_GAS_LIMIT), UINT);
  rlp_add(rlp, d_get(tx,K_TO)                , ADDRESS);
  rlp_add(rlp, d_get(tx,K_VALUE)             , UINT);
  rlp_add(rlp, d_get_or(tx,K_INPUT,K_DATA)   , BYTES);
  rlp_add(rlp, d_get(tx,K_V)                 , UINT);
  rlp_add(rlp, d_get(tx,K_R)                 , UINT);
  rlp_add(rlp, d_get(tx,K_S)                 , UINT);

  rlp_encode_list(bb,&rlp->b);

  bb_free(rlp);
  return bb_move_to_bytes(bb);
}

bytes_t *serialize_tx_receipt(in3_vctx_t *vc, d_token_t *receipt) {
  bytes_builder_t *bb = bb_new();
  bytes_builder_t *rlp = bb_new();
  bytes_builder_t *rlp_log = bb_new();
  bytes_builder_t *rlp_topics = bb_new();
  bytes_builder_t *rlp_loglist = bb_new();
  d_token_t *t, *logs, *l, *topics;
  int i, j;

  if ((t=d_get_or(receipt,K_STATUS,K_ROOT)))
    rlp_add(rlp, t, UINT);
  
  rlp_add(rlp, d_get(receipt,K_CUMULATIVE_GAS_USED) , UINT);
  rlp_add(rlp, d_get(receipt,K_LOGS_BLOOM         ) , BLOOM);

  logs =  d_get(receipt,K_LOGS);
  if (logs) {
    for (i = 0,l=logs+1; i < d_len(logs); i++, l=d_next(l)) {
      bb_clear(rlp_log);

      rlp_add(rlp_log, d_get(l,K_ADDRESS) , ADDRESS);

      topics = d_get(l,K_TOPICS);
      bb_clear(rlp_topics);
      for (j = 0, t = topics+1; j < d_len(topics); j++, t=d_next(t))
         rlp_add( rlp_topics, t, HASH);

      rlp_encode_list(rlp_log, &rlp_topics->b);

      rlp_add(rlp_log, d_get(l,K_DATA),BYTES);
      rlp_encode_list(rlp_loglist, &rlp_log->b);
    }

  }
  rlp_encode_list(rlp, &rlp_loglist->b);

  bb_clear(rlp_log);
  rlp_encode_list(rlp_log, &rlp->b);
  
  bb_free(bb);
  bb_free(rlp);
  bb_free(rlp_topics);
  bb_free(rlp_loglist);
  return bb_move_to_bytes(rlp_log);
}