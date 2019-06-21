#ifndef IN3_VHIST_H
#define IN3_VHIST_H

#include "../core/util/bytes.h"
#include "../core/util/data.h"
#include <stdint.h>

typedef struct {
  bytes_builder_t* diffs;
  bytes_builder_t* vldtrs;
} vh_t;

vh_t*            vh_init(json_ctx_t* nodelist);
void             vh_free(vh_t* vh);
bytes_builder_t* vh_get_for_block(vh_t* vh, uint64_t block);

#endif //IN3_VHIST_H
