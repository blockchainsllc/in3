#ifndef IN3_VHIST_H
#define IN3_VHIST_H

#include "../core/util/bytes.h"
#include "../core/util/data.h"
#include "../core/util/error.h"
#include "../core/util/utils.h"
#include <stdint.h>

typedef struct {
  uint16_t pos;
  uint8_t* v;
} vdiff_t;

typedef struct {
  vdiff_t* df;
  uint64_t blk;
  size_t   len;
  size_t   vlen;
} vblkdiff_t;

typedef struct {
  vblkdiff_t*      bdf;
  bytes_builder_t* vect;
  size_t           len;
} vhist_t;

vhist_t*  vhist_init(json_ctx_t* nodelist);
void      vhist_free(vhist_t* vh);
in3_ret_t vhist_get_for_block(vhist_t* vh, uint64_t block);

#endif //IN3_VHIST_H
