#include "vhist.h"
#include "../core/client/keys.h"
#include "../core/util/error.h"
#include "../core/util/mem.h"
#include <stdbool.h>
#include <string.h>

static in3_ret_t bb_find(bytes_builder_t* bb, uint8_t* v, size_t l) {
  if (v) {
    for (size_t i = 0; i < bb->b.len; i += l)
      if (!memcmp(bb->b.data + i, v, l))
        return (i / l);
  }
  return IN3_EFIND;
}

vh_t* vh_init(json_ctx_t* nodelist) {
  if (nodelist == NULL) return NULL;
  bytes_t*  b   = NULL;
  in3_ret_t ret = IN3_OK;
  uint64_t  blk = 0;

  d_token_t *ss = d_get(nodelist->result, K_STATES), *vs = NULL;
  if (ss == NULL) return NULL;

  vh_t* vh = _malloc(sizeof(*vh));
  if (vh == NULL) return NULL;
  vh->vldtrs = bb_new();
  vh->diffs  = bb_new();
  if (!vh->vldtrs || !vh->diffs) {
    _free(vh);
    _free(vh->vldtrs);
    _free(vh->diffs);
    return NULL;
  }

  for (d_iterator_t sitr = d_iter(ss); sitr.left; d_iter_next(&sitr)) {
    vs  = d_get(sitr.token, K_VALIDATORS);
    blk = d_get_longk(sitr.token, K_BLOCK);
    bb_write_long(vh->diffs, blk);
    bb_write_int(vh->diffs, d_len(vs));
    if (d_type(vs) == T_ARRAY) {
      for (d_iterator_t vitr = d_iter(vs); vitr.left; d_iter_next(&vitr)) {
        b   = d_bytesl(vitr.token, 20);
        ret = bb_find(vh->vldtrs, b->data, 20);
        if (ret == IN3_EFIND) {
          bb_write_int(vh->diffs, vh->vldtrs->b.len / 20);
          bb_write_fixed_bytes(vh->vldtrs, b);
        } else {
          bb_write_int(vh->diffs, ret);
        }
      }
    }
  }
  return vh;
}

void vh_free(vh_t* vh) {
  bb_free(vh->diffs);
  bb_free(vh->vldtrs);
  _free(vh);
}

bytes_builder_t* vh_get_for_block(vh_t* vh, uint64_t block) {
  bytes_builder_t* bb  = bb_new();
  uint64_t         blk = 0;
  uint32_t         sz = 0, pos = 0;
  if (bb == NULL) return NULL;

  for (size_t i = 0; i < vh->diffs->b.len;) {
    blk = bb_read_long(vh->diffs, &i);
    sz  = bb_read_int(vh->diffs, &i);
    bb_clear(bb);
    for (size_t j = 0; j < sz; ++j) {
      pos = bb_read_int(vh->diffs, &i);
      bb_write_raw_bytes(bb, vh->vldtrs->b.data + (pos * 20), 20);
    }
    if (blk >= block) break;
  }
  return bb;
}
