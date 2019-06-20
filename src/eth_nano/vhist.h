#ifndef IN3_VHIST_H
#define IN3_VHIST_H

#include "../core/util/bytes.h"
#include "../core/util/error.h"
#include "../core/util/utils.h"
#include <stdint.h>

typedef struct {
  uint64_t   block;
  size_t     start;
  size_t     len;
  address_t* validators;
} vdiff_t;

typedef struct {
  vdiff_t** diffs;
  size_t   len;
} vhist_t;

vhist_t*  vhist_init(address_t* validators, size_t* len);
in3_ret_t vhist_update(vhist_t* history, address_t* validators, size_t len, uint64_t start);
in3_ret_t vhist_get_for_block(vhist_t* history, uint64_t block, address_t** validators);

#endif //IN3_VHIST_H
