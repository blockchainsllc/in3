

#ifndef TEST
#define TEST
#endif
#include <core/util/data.h>
#include <core/util/utils.h>
#include <eth_basic/trie.h>
#include <eth_full/evm.h>
#include <eth_nano/rlp.h>
#include <eth_nano/serialize.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "vm_runner.h"

void append_rlp(bytes_builder_t* bb, d_token_t* in) {
  int i;
  if (d_type(in) == T_ARRAY) {
    bytes_builder_t* tmp = bb_new();
    d_token_t*       t = NULL;
    for (i = 0, t = in + 1; i < d_len(in); i++, t = d_next(t))
      append_rlp(tmp, t);
    rlp_encode_list(bb, &tmp->b);
    bb_free(tmp);
    return;
  }
  bytes_t b = d_to_bytes(in);
  rlp_encode_item(bb, &b);
}

int test_rlp(d_token_t* test, uint32_t props, uint64_t* ms) {
  uint64_t   start = clock();
  int        res   = 0;
  d_token_t* in    = d_get(test, key("in"));
  bytes_t    out   = d_to_bytes(d_get(test, key("out")));
  if (d_type(in) == T_STRING && d_len(in) > 0 && *in->data == '#') return 0;
  bytes_builder_t* bb = bb_new();
  append_rlp(bb, in);
  if (!b_cmp(&bb->b, &out)) {
    print_error("Wrong result");
    printf("\nexpected:");
    ba_print(out.data, out.len);
    printf("\nis      :");
    ba_print(bb->b.data, bb->b.len);
    res = -1;
  }
  bb_free(bb);

  *ms = (clock() - start) / 1000;
  return res;
}