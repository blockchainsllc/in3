#include <util/bytes.h>
#include <client/context.h>
#include <string.h>
#include <client/verifier.h>
#include "serialize.h"
#include "rlp.h"
#include "util/utils.h"

#define rlp_add(rlp, ml)                                                              \
  {                                                                                   \
    bb_clear(bb);                                                                     \
    if (t)                                                                            \
      bb_write_from_str(bb, vc->ctx->response_data + t->start, t->end - t->start, ml); \
    rlp_encode_item(rlp, &(bb->b));                                                   \
  }
#define find(root, name) t = res_get(vc, root, name)




bytes_t *serialize_tx(in3_vctx_t *vc, jsmntok_t *tx) {
  bytes_builder_t *bb = bb_new();
  bytes_builder_t *rlp = bb_new();
  jsmntok_t *t;
  find(tx, "nonce");
  rlp_add(rlp, 0);
  if (!((find(tx, "gasPrice")) || (find(tx, "gasLimit"))))
    t = NULL;
  rlp_add(rlp, 0);
  find(tx, "gasPrice");
  rlp_add(rlp, 0);
  if ((find(tx, "to")) && t->size > 2)
    rlp_add(rlp, 20)
  else
    rlp_add(rlp, 0)
  find(tx, "value");
  rlp_add(rlp, 0);

  if (!((find(tx, "input")) || (find(tx, "data"))))
    t = NULL;
  rlp_add(rlp, -1);

  find(tx, "v");
  rlp_add(rlp, 0);

  find(tx, "r");
  rlp_add(rlp, 0);

  find(tx, "s");
  rlp_add(rlp, 0);
  bb_clear(bb);
  rlp_encode_list(bb,&rlp->b);

  bb_free(rlp);
  return bb_move_to_bytes(bb);
}

bytes_t *serialize_tx_receipt(in3_vctx_t *vc, jsmntok_t *receipt) {
  bytes_builder_t *bb = bb_new();
  bytes_builder_t *rlp = bb_new();
  bytes_builder_t *rlp_log = bb_new();
  bytes_builder_t *rlp_topics = bb_new();
  bytes_builder_t *rlp_loglist = bb_new();
  jsmntok_t *t, *logs, *l, *topics;
  int i, j;

  if ((find(receipt, "status")) || (find(receipt, "root")))
    rlp_add(rlp, 0);
  find(receipt, "cumulativeGasUsed");
  rlp_add(rlp, 0);
  find(receipt, "logsBloom");
  rlp_add(rlp, 256);
  logs = res_get(vc, receipt, "logs");
  if (logs){
    for (i = 0; i < logs->size; i++) {
      l = ctx_get_array_token(logs, i);
      bb_clear(rlp_log);
      find(l, "address");
      rlp_add(rlp_log, 20);

      // encode topic
      topics = res_get(vc, l, "topics");
      bb_clear(rlp_topics);
      for (j = 0; j < topics->size; j++) {
        t = ctx_get_array_token(topics, j);
        rlp_add(rlp_topics, 32);
      }
      rlp_encode_list(rlp_log, &rlp_topics->b);
      find(l, "data");
      rlp_add(rlp_log, 0);
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