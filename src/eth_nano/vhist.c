#include "vhist.h"
#include "../core/util/mem.h"
#include <stdbool.h>
#include <string.h>

#define FIRST_CHANGE(a, alen, b, blen) change_idx(a, alen, b, blen, true)
#define LAST_CHANGE(a, alen, b, blen) change_idx(a, alen, b, blen, false)

static size_t change_idx(address_t* a, size_t alen, address_t* b, size_t blen, bool forward) {
  size_t l = min(alen, blen);
  for (size_t i = 0; i < l; i++) {
    if ((forward && memcmp(a[i], b[i], 20)) ||
        (!forward && memcmp(a[alen - 1 - i], b[blen - 1 - i], 20)))
      return i;
  }
  return l;
}

static vdiff_t* diff(address_t* a, size_t alen, address_t* b, size_t blen, uint64_t block) {
  const size_t first = FIRST_CHANGE(a, alen, b, blen);
  const size_t last  = LAST_CHANGE(a, alen, b, blen);
  if (alen == blen && first == alen) return NULL;

  vdiff_t* vd = _malloc(sizeof(*vd) + (blen - last) * 20);
  if (vd) {
    vd->block = block;
    vd->start = first;
    vd->len   = alen - last - first;
    memcpy(vd->validators, b, (blen - last) * 20);
  }
  return vd;
}

vhist_t* vhist_init(address_t* validators) {
  vhist_t* vh = _calloc(1, sizeof(*vh));
  if (vh != NULL) {
    if (validators != NULL && vhist_update(vh, validators) != IN3_OK) {
      _free(vh);
      return NULL;
    }
  }
  return vh;
}

in3_ret_t vhist_push_diff(vhist_t* history, vdiff_t *diff) {
  vdiff_t** vd_ = _realloc(history->diffs, sizeof(*vd_)*(history->len+1), sizeof(*vd_)*history->len);
  if (vd_ ==NULL) return IN3_ENOMEM;
  history->diffs = vd_;
  history->diffs[history->len] = diff;
  return IN3_OK;
}

static in3_ret_t array_splice(address_t **v, size_t vlen, uint64_t start, size_t ldel, address_t *add, size_t ladd) {
  *v += start;
  int i = 0;
  for (i = 0; ldel--; ++i)
    memmove(*v[i], *v[i+vlen], 20);
  *v += i;
  for (i = 0; ladd--; ++i)
    memmove(*v[i], add[i], 20);
  return vlen -ldel + ladd;
}

//in3_ret_t vhist_update(vhist_t* history, address_t* validators, size_t len, uint64_t start) {
//  address_t *vprev = NULL;
//  const in3_ret_t vprevlen = vhist_get_for_block(history, start-1, &vprev);
//  vdiff_t* delta = diff(vprev, vprevlen,validators, len, start);
//  if (!delta) return IN3_OK;
//  if (!history->len || history->diffs[history->len-1]->block<start)
//    return vhist_push_diff(history, delta);
//  else {
//    for (size_t i=history->len-1;i>=0;i--) {
//      vdiff_t* d = history->diffs[i];
//      if (d->block== start) {
//        address_t *vnext = vprev;
//        in3_ret_t vnextlen = array_splice(&vnext, vprevlen, d->start, d->len, d->validators, d->len);  // old state
//        if (i+1<history->len)
//          vnextlen= array_splice(&vnext, vnextlen, history->diffs[i+1]->start, history->diffs[i+1]->len, history->diffs[i+1]->validators, history->diffs[i+1]->len) ; // next state
//        history->diffs[i]=delta;
//        if (i+1==history->len) return IN3_OK;
//        vdiff_t* nd = diff(validators, len,vnext, vnextlen,history->diffs[i+1]->block);
//        if (!nd)
//          this.data.splice(i+1,1);
//        else
//          this.data[i+1]=nextDelta;
//        return IN3_OK;
//      }
//      else if (d.block<start) {
//        const n = this.data[i+1];
//        const next = [...prev];
//        next.splice(n.start,n.len, ...n.data);  // next state
//        const nextDelta = createDelta(data,next,n.block);
//        if (!nextDelta)
//          n.block = start;
//        else
//          this.data.splice(i+1,1,delta,nextDelta);
//        return IN3_OK;
//      }
//    }
//    // we need to insert into first pos
//    const dn = createDelta(data,this.data[0].data,this.data[0].block)
//    if (!dn)
//      this.data[0].block=start
//    else
//      this.data.splice(0,1,delta,dn)
//  }
//
//  return IN3_OK;
//}

in3_ret_t vhist_get_for_block(vhist_t* history, uint64_t block, address_t** validators) {
  vdiff_t** vds   = history->diffs;
  size_t   oldsz = 0, l = 0;
  address_t *v_ = NULL;
  for (int i = 0; i < history->len && vds[i]->block > block; ++i) {
    v_ = _realloc(*validators, oldsz + (sizeof(**validators) * vds[i]->len), oldsz);
    if (!v_) {
      _free(*validators);
      return IN3_ENOMEM;
    }
    *validators = v_;
    oldsz += (sizeof(**validators) * vds[i]->len);
    for (int j = 0; j < vds[i]->len; ++j)
      memcpy(*validators[l++], vds[i]->validators[j], 20);
  }
  return l;
}
