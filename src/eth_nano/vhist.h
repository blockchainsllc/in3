#ifndef IN3_VHIST_H
#define IN3_VHIST_H

#include "../core/util/bytes.h"
#include "../core/util/data.h"
#include <stdint.h>

typedef struct {
  bytes_builder_t* diffs;
  bytes_builder_t* vldtrs;
  uint64_t         last_change_block;
} vhist_t;

vhist_t*         vh_init(d_token_t* nodelist);
void             vh_free(vhist_t* vh);
bytes_builder_t* vh_get_for_block(vhist_t* vh, uint64_t block);

#endif //IN3_VHIST_H
