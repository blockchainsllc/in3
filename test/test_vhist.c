#ifndef TEST
#define TEST
#endif
#define DEBUG

#include <core/client/keys.h>
#include <core/util/data.h>
#include <core/util/mem.h>
#include <eth_nano/vhist.h>
#include <inttypes.h>

#include "test_utils.h"

static struct timeval begin, end;

static char* filetostr(const char* filename) {
  char* buffer = NULL;
  long  length;
  FILE* f = fopen(filename, "r");
  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = _malloc(length);
    if (buffer) {
      fread(buffer, 1, length, f);
    }
    fclose(f);
  }
  return buffer;
}

static in3_ret_t incache(uint8_t* varr, size_t l, uint8_t* v) {
  if (v) {
    for (int i = 0; i < l; i += 20)
      if (!memcmp(varr + i, v, 20))
        return (i / 20);
  }
  return IN3_EFIND;
}

static void printcache(uint8_t* varr, size_t l) {
  printf("--- CACHE ---\n");
  for (int i = 0; i < l; i += 20) {
    ba_print(&varr[i], 20);
    printf("\n");
  }
}

static void test_bitset_ordering() {
  char*       nodeliststr = filetostr("/Users/sufi-al-hussaini/in3-core/test/testdata/tobalaba_nodelist.json");
  json_ctx_t* jnl         = parse_json(nodeliststr);
  if (jnl == NULL) return;

  d_token_t *      ss = d_get(jnl->result, K_STATES), *vs = NULL;
  bytes_builder_t* bb  = bb_new();
  bytes_t*         b   = NULL;
  in3_ret_t        ret = IN3_OK;
  int              i   = 0;

  for (d_iterator_t sitr = d_iter(ss); sitr.left; d_iter_next(&sitr)) {
    vs = d_get(sitr.token, K_VALIDATORS);
    printf("--- %" PRIu64 "\n", d_get_longk(sitr.token, K_BLOCK));
    if (d_type(vs) == T_ARRAY) {
      i = 0;
      for (d_iterator_t vitr = d_iter(vs); vitr.left; d_iter_next(&vitr)) {
        b   = d_bytesl(vitr.token, 20);
        ret = incache(bb->b.data, bb->b.len, b->data);
        if (ret == IN3_EFIND)
          bb_write_fixed_bytes(bb, b);
        else if (ret != i) {
          printf(": order changed -> %d to %d!\n", ret, i);
        } else if (ret == i) {
          printf(": order unchanged\n");
        }
        i++;
      }
    }
  }
  //  printcache(bb->b.data, bb->b.len);
  _free(nodeliststr);
}

typedef struct {
  uint16_t pos;
  uint8_t* v;
  uint64_t blk;
  size_t   len;
} vdiff2_t;

typedef struct {
  vdiff2_t* diffs;
  size_t    len;
} vhist2_t;

in3_ret_t diff_emplace2(vhist2_t* h, uint16_t pos, bytes_t* b, uint64_t blk, size_t len) {
  vdiff2_t* d_ = _realloc(h->diffs, sizeof(*d_) * (h->len + 1), sizeof(*d_) * h->len);
  if (d_ == NULL) return IN3_ENOMEM;
  d_[h->len].pos = pos;
  if (b) {
    d_[h->len].v = _malloc(20);
    if (d_[h->len].v == NULL) {
      _free(d_);
      return IN3_ENOMEM;
    }
    memcpy(d_[h->len].v, b->data, b->len);
  } else {
    d_[h->len].v = NULL;
  }
  d_[h->len].blk = blk;
  d_[h->len].len = len;
  h->diffs       = d_;
  h->len++;
  return h->len;
}

static void test_vhist_diff2() {
  char*       nodeliststr = filetostr("/Users/sufi-al-hussaini/in3-core/test/testdata/tobalaba_nodelist.json");
  json_ctx_t* jnl         = parse_json(nodeliststr);
  if (jnl == NULL) return;

  d_token_t *      ss = d_get(jnl->result, K_STATES), *vs = NULL;
  bytes_builder_t* bb  = bb_new();
  bytes_t*         b   = NULL;
  in3_ret_t        ret = IN3_OK;
  vhist2_t         h   = {.diffs = NULL, .len = 0};
  int              i   = 0;
  uint64_t         blk = 0;

  for (d_iterator_t sitr = d_iter(ss); sitr.left; d_iter_next(&sitr)) {
    vs  = d_get(sitr.token, K_VALIDATORS);
    blk = d_get_longk(sitr.token, K_BLOCK);
    if (d_type(vs) == T_ARRAY) {
      i = 0;
      for (d_iterator_t vitr = d_iter(vs); vitr.left; d_iter_next(&vitr)) {
        b   = d_bytesl(vitr.token, 20);
        ret = incache(bb->b.data, bb->b.len, b->data);
        if (ret == IN3_EFIND) {
          //          printf("+ %d [%" PRIu64 "]\n", i, blk);
          diff_emplace2(&h, i, b, blk, d_len(vs) * 20);
          bb_write_fixed_bytes(bb, b);
        } else if (ret != i) {
          //          printf("-/+ %d, %d [%" PRIu64 "]\n", ret, i, blk);
          diff_emplace2(&h, i, b, blk, d_len(vs) * 20);
          bb_replace(bb, i * 20, 20, bb->b.data + (ret * 20), 20);
          for (int k = (ret * 20); k + 20 < bb->b.len / 20; k += 20) {
            memmove(&bb->b.data + k, &bb->b.data + k + 20, 20);
          }
        }
        i++;
      }
      bb->b.len = d_len(vs) * 20;
      //      printcache(bb->b.data, bb->b.len);
    }
  }

  bb_free(bb);
  bb          = bb_new();
  vdiff2_t* d = NULL;
  for (int j = 0; j < h.len; ++j) {
    d   = &h.diffs[j];
    ret = incache(bb->b.data, bb->b.len, d->v);
    if (ret == IN3_EFIND) {
      printf("+ %d [%" PRIu64 "]\n", d->pos, d->blk);
      bytes_t b_ = {.data = d->v, .len = 20};
      bb_write_fixed_bytes(bb, &b_);
      if (bb->b.len / 20 == (d->pos + 2)) {
        bb_replace(bb, d->pos * 20, 20, bb->b.data + bb->b.len - 20, 20);
        bb->b.len -= 20;
      }
    } else if (ret != d->pos) {
      printf("-/+ %d, %d [%" PRIu64 "]\n", ret, d->pos, d->blk);
      bb_replace(bb, d->pos * 20, 20, bb->b.data + (ret * 20), 20);
      bb->b.len = d->len;
    }
    printcache(bb->b.data, bb->b.len);
  }

  _free(h.diffs);
  bb_free(bb);
  _free(nodeliststr);
}

static void test_vhist_diff() {
  char*       nodeliststr = filetostr("/Users/sufi-al-hussaini/in3-core/test/testdata/tobalaba_nodelist.json");
  json_ctx_t* jnl         = parse_json(nodeliststr);
  if (jnl == NULL) return;
  vhist_t* vh = vhist_init(jnl);
  vhist_get_for_block(vh, 7157871);
  printcache(vh->vect->b.data, vh->vect->b.len);
  vhist_free(vh);
  free_json(jnl);
  _free(nodeliststr);
}

static void test_vh_diff() {
  char*       nodeliststr = filetostr("/Users/sufi-al-hussaini/in3-core/test/testdata/tobalaba_nodelist.json");
  json_ctx_t* jnl         = parse_json(nodeliststr);
  if (jnl == NULL) return;
  vh_t*            vh = vh_init(jnl);
  bytes_builder_t* bb = vh_get_for_block(vh, 7157871);
  printcache(bb->b.data, bb->b.len);
  //  vh_free(vh);
  free_json(jnl);
  _free(nodeliststr);
}

/*
 * Main
 */
int main() {
  TESTS_BEGIN();
  RUN_TIMED_TEST(test_vhist_diff);
  RUN_TIMED_TEST(test_vh_diff);
  return TESTS_END();
}
