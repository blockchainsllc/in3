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

vhist_t* vh_init(d_token_t* nodelist) {
  if (nodelist == NULL) return NULL;
  bytes_t   b   = {.data = NULL, .len = 0};
  in3_ret_t ret = IN3_OK;
  uint64_t  blk = 0;

  d_token_t *ss = d_get(nodelist, K_STATES), *vs = NULL;
  if (ss == NULL) return NULL;

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

  for (d_iterator_t sitr = d_iter(ss); sitr.left; d_iter_next(&sitr)) {
    vs  = d_get(sitr.token, K_VALIDATORS);
    blk = d_get_longk(sitr.token, K_BLOCK);
    bb_write_long(vh->diffs, blk);
    bb_write_int(vh->diffs, d_len(vs));
    if (d_type(vs) == T_ARRAY) {
      for (d_iterator_t vitr = d_iter(vs); vitr.left; d_iter_next(&vitr)) {
        b   = (d_type(vitr.token) == T_STRING) ? b_from_hexstr(d_string(vitr.token)) : *d_bytesl(vitr.token, 20);
        ret = bb_find(vh->vldtrs, b.data, 20);
        if (ret == IN3_EFIND) {
          bb_write_int(vh->diffs, vh->vldtrs->b.len / 20);
          bb_write_fixed_bytes(vh->vldtrs, &b);
        } else {
          bb_write_int(vh->diffs, ret);
        }
        if (d_type(vitr.token) == T_STRING) _free(b.data);
      }
    }
  }
  vh->last_change_block = blk;
  return vh;
}

void vh_free(vhist_t* vh) {
  bb_free(vh->diffs);
  bb_free(vh->vldtrs);
  _free(vh);
}

bytes_builder_t* vh_get_for_block(vhist_t* vh, uint64_t block) {
  bytes_builder_t* bb  = bb_new();
  uint64_t         blk = 0;
  uint32_t         sz = 0, pos = 0, prevsz = 0;
  size_t           i = 0;
  if (bb == NULL) return NULL;

  for (i = 0; i < vh->diffs->b.len;) {
    bb_read_next(vh->diffs, &i, &blk);
    bb_read_next(vh->diffs, &i, &sz);
    if (blk > block) {
      i -= ((prevsz * 4) + 12);
      break;
    }
    i += sz * 4;
    prevsz = sz;
  }

  // If block exceeds last block at which there was a validator list change,
  // send latest validator list
  if (i >= vh->diffs->b.len) i -= prevsz * 4;

  for (size_t j = 0; j < prevsz; ++j) {
    bb_read_next(vh->diffs, &i, &pos);
    bb_write_raw_bytes(bb, vh->vldtrs->b.data + (pos * 20), 20);
  }
  return bb;
}
