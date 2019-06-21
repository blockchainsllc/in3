#include "vhist.h"
#include "../core/client/keys.h"
#include "../core/util/mem.h"
#include <stdbool.h>
#include <string.h>

static in3_ret_t vec_find(bytes_builder_t* bb, uint8_t* v) {
  if (v) {
    for (size_t i = 0; i < bb->b.len; i += 20)
      if (!memcmp(bb->b.data + i, v, 20))
        return (i / 20);
  }
  return IN3_EFIND;
}

static in3_ret_t blkdiff_emplace(vhist_t* h, uint64_t blk, size_t vlen) {
  vblkdiff_t* bd = NULL;
  for (size_t i = 0; i < h->len; ++i)
    if (h->bdf[i].blk == blk)
      bd = &h->bdf[i];

  if (bd == NULL) {
    vblkdiff_t* bd_ = _realloc(h->bdf, sizeof(*bd_) * (h->len + 1), sizeof(*bd_) * h->len);
    if (bd_ == NULL) return IN3_ENOMEM;
    bd_[h->len].blk  = blk;
    bd_[h->len].vlen = vlen;
    bd_[h->len].len  = 0;
    bd_[h->len].df   = NULL;
    h->bdf           = bd_;
    bd               = &h->bdf[h->len];
    h->len++;
  }
  return h->len - 1;
}

in3_ret_t diff_emplace(vblkdiff_t* bd, uint16_t pos, bytes_t* b) {
  vdiff_t* d_ = _realloc(bd->df, sizeof(*d_) * (bd->len + 1), sizeof(*d_) * bd->len);
  if (d_ == NULL) return IN3_ENOMEM;
  d_[bd->len].pos = pos;
  if (b) {
    d_[bd->len].v = _malloc(20);
    if (d_[bd->len].v == NULL) {
      _free(d_);
      return IN3_ENOMEM;
    }
    memcpy(d_[bd->len].v, b->data, b->len);
  } else {
    d_[bd->len].v = NULL;
  }
  bd->df = d_;
  bd->len++;
  return bd->len - 1;
}

vhist_t* vhist_init(json_ctx_t* nodelist) {
  if (nodelist == NULL) return NULL;
  bytes_t*  b   = NULL;
  in3_ret_t ret = IN3_OK, ib = IN3_OK;
  int       i   = 0;
  uint64_t  blk = 0;

  d_token_t *ss = d_get(nodelist->result, K_STATES), *vs = NULL;
  if (ss == NULL) return NULL;

  vhist_t* vh = _malloc(sizeof(*vh));
  if (vh == NULL) return NULL;
  vh->bdf  = NULL;
  vh->len  = 0;
  vh->vect = bb_new();
  if (vh->vect == NULL) {
    _free(vh);
    return NULL;
  }

  // create diff
  for (d_iterator_t sitr = d_iter(ss); sitr.left; d_iter_next(&sitr)) {
    vs  = d_get(sitr.token, K_VALIDATORS);
    blk = d_get_longk(sitr.token, K_BLOCK);
    ib  = blkdiff_emplace(vh, blk, d_len(vs) * 20);
    if (d_type(vs) == T_ARRAY) {
      i = 0;
      for (d_iterator_t vitr = d_iter(vs); vitr.left; d_iter_next(&vitr)) {
        b   = d_bytesl(vitr.token, 20);
        ret = vec_find(vh->vect, b->data);
        if (ret == IN3_EFIND) {
          // if not found in vect, push back
          diff_emplace(&vh->bdf[ib], i, b);
          bb_write_fixed_bytes(vh->vect, b);
        } else if (ret != i) {
          // if found replace
          diff_emplace(&vh->bdf[ib], i, b);
          bb_replace(vh->vect, i * 20, 20, vh->vect->b.data + (ret * 20), 20);
          for (size_t k = (ret * 20); k + 20 < vh->vect->b.len / 20; k += 20) {
            memmove(&vh->vect->b.data + k, &vh->vect->b.data + k + 20, 20);
          }
        }
        i++;
      }
      vh->vect->b.len = d_len(vs) * 20;
    }
  }
  return vh;
}

void vhist_free(vhist_t* vh) {
  for (size_t i = 0; i < vh->len; ++i) {
    for (size_t j = 0; j < vh->bdf[i].len; ++j)
      _free(vh->bdf[i].df[j].v);
    _free(vh->bdf[i].df);
  }
  _free(vh->bdf);
  bb_free(vh->vect);
  _free(vh);
}

in3_ret_t vhist_get_for_block(vhist_t* vh, uint64_t block) {
  in3_ret_t   ret = IN3_OK;
  vblkdiff_t* bd  = NULL;
  vdiff_t*    d   = NULL;

  bb_clear(vh->vect);
  for (size_t j = 0; j < vh->len; ++j) {
    bd = &vh->bdf[j];
    if (bd->blk > block) {
      break;
    } else if (!bd->len) {
      vh->vect->b.len = bd->vlen;
      continue;
    }
    for (size_t i = 0; i < bd->len; ++i) {
      d   = &bd->df[i];
      ret = vec_find(vh->vect, d->v);
      if (ret == IN3_EFIND) {
        printf("+ %d [%" PRIu64 "]\n", d->pos, bd->blk);
        bytes_t b_ = {.data = d->v, .len = 20};
        bb_write_fixed_bytes(vh->vect, &b_);
        if (vh->vect->b.len / 20 == (d->pos + 2)) {
          bb_replace(vh->vect, d->pos * 20, 20, vh->vect->b.data + vh->vect->b.len - 20, 20);
          vh->vect->b.len -= 20;
        }
      } else if (ret != d->pos) {
        printf("-/+ %d, %d [%" PRIu64 "]\n", ret, d->pos, bd->blk);
        bb_replace(vh->vect, d->pos * 20, 20, vh->vect->b.data + (ret * 20), 20);
        vh->vect->b.len = bd->vlen;
      }
    }
  }
  return ret;
}
