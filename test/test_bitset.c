#ifndef TEST
#define TEST
#endif
#define DEBUG

#include <core/client/keys.h>
#include <core/util/bitset.h>
#include <core/util/data.h>
#include <core/util/log.h>
#include <core/util/mem.h>
#include <inttypes.h>

#include "test_utils.h"

static void bs_print(bitset_t* bs) {
  fprintf(stderr, "> [%s]: ", bs->len > BS_MAX ? "heap" : "stack");
  for (size_t i = bs->len; i != 0; --i)
    fprintf(stderr, "%x", bs_isset(bs, i - 1));
  fprintf(stderr, "\n");
}

static void test_bitset_op() {
  bitset_t* bs = bs_new(30);
  bs_set(bs, 16);
  bs_toggle(bs, 16);
  bs_set(bs, 14);
  bs_clear(bs, 14);
  bs_toggle(bs, 28);
  bs_clear(bs, 28);
  TEST_ASSERT_TRUE(bs_isempty(bs));
  bs_free(bs);
  bs = bs_new(130);
  bs_set(bs, 116);
  bs_toggle(bs, 116);
  bs_set(bs, 114);
  bs_clear(bs, 114);
  bs_toggle(bs, 128);
  bs_clear(bs, 128);
  TEST_ASSERT_TRUE(bs_isempty(bs));
  bs_free(bs);
}

static void test_bitset_transition() {
  bitset_t* bs = bs_new(20);
  bs_set(bs, 19);
  bs_set(bs, 11);
  bs_set(bs, 121);
  TEST_ASSERT_TRUE(bs_isset(bs, 19));
  TEST_ASSERT_TRUE(bs_isset(bs, 11));
  TEST_ASSERT_TRUE(bs_isset(bs, 121));
  bs_clear(bs, 19);
  bs_clear(bs, 11);
  bs_clear(bs, 121);
  TEST_ASSERT_TRUE(bs_isempty(bs));
  bs_free(bs);
}

static void test_bitset_realloc() {
  bitset_t* bs = bs_new(8);
  bs_set(bs, 16);
  bs_set(bs, 32);
  bs_set(bs, 64);
  bs_set(bs, 128);
  bs_set(bs, 256);
  TEST_ASSERT_TRUE(bs_isset(bs, 16));
  TEST_ASSERT_TRUE(bs_isset(bs, 32));
  TEST_ASSERT_TRUE(bs_isset(bs, 64));
  TEST_ASSERT_TRUE(bs_isset(bs, 128));
  TEST_ASSERT_TRUE(bs_isset(bs, 256));
  bs_free(bs);
}

static void test_bitset_edge_cases() {
  bitset_t* bs = bs_new(64);
  bs_set(bs, 63);
  bs_set(bs, 64);
  bs_set(bs, 65);
  TEST_ASSERT_TRUE(bs_isset(bs, 63));
  TEST_ASSERT_TRUE(bs_isset(bs, 64));
  TEST_ASSERT_TRUE(bs_isset(bs, 65));
  bs_clear(bs, 63);
  bs_clear(bs, 64);
  bs_clear(bs, 65);
  TEST_ASSERT_TRUE(bs_isempty(bs));
  bs_free(bs);
  bs = bs_new(128);
  bs_set(bs, 127);
  bs_set(bs, 126);
  bs_set(bs, 125);
  bs_set(bs, 128);
  bs_set(bs, 129);
  TEST_ASSERT_TRUE(bs_isset(bs, 125));
  TEST_ASSERT_TRUE(bs_isset(bs, 126));
  TEST_ASSERT_TRUE(bs_isset(bs, 127));
  TEST_ASSERT_TRUE(bs_isset(bs, 128));
  TEST_ASSERT_TRUE(bs_isset(bs, 129));
  bs_clear(bs, 127);
  bs_clear(bs, 126);
  bs_clear(bs, 125);
  bs_clear(bs, 128);
  bs_clear(bs, 129);
  TEST_ASSERT_TRUE(bs_isempty(bs));
  bs_free(bs);
}

static void test_bitset_clone() {
  bitset_t* bs1 = bs_new(11);
  bs_set(bs1, 0);
  bs_set(bs1, 2);
  bs_set(bs1, 4);
  bitset_t* bs2 = bs_clone(bs1);
  TEST_ASSERT_TRUE(bs_isset(bs2, 0));
  TEST_ASSERT_TRUE(bs_isset(bs2, 2));
  TEST_ASSERT_TRUE(bs_isset(bs2, 4));
  bs_clear(bs2, 0);
  bs_clear(bs2, 2);
  bs_clear(bs2, 4);
  TEST_ASSERT_TRUE(bs_isempty(bs2));
  bs_free(bs1);
  bs_free(bs2);
  bs1 = bs_new(130);
  bs_set(bs1, 120);
  bs_set(bs1, 122);
  bs_set(bs1, 124);
  bs2 = bs_clone(bs1);
  TEST_ASSERT_TRUE(bs_isset(bs2, 120));
  TEST_ASSERT_TRUE(bs_isset(bs2, 122));
  TEST_ASSERT_TRUE(bs_isset(bs2, 124));
  bs_clear(bs2, 120);
  bs_clear(bs2, 122);
  bs_clear(bs2, 124);
  TEST_ASSERT_TRUE(bs_isempty(bs2));
  bs_free(bs1);
  bs_free(bs2);
}

static void test_bitset_out_of_range() {
  bitset_t* bs = bs_new(30);
  TEST_ASSERT_FALSE(bs_isset(bs, 64));
  TEST_ASSERT_FALSE(bs_isset(bs, 128));
  bs_free(bs);
  bs = bs_new(120);
  TEST_ASSERT_FALSE(bs_isset(bs, 128));
  TEST_ASSERT_FALSE(bs_isset(bs, 256));
  bs_free(bs);
}

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
  char*       nodeliststr = filetostr("/Users/sufi-al-hussaini/in3-core/test/tobalaba_nodelist.json");
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
          //          b_print(b);
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
} vdiff_t;

typedef struct {
  vdiff_t* diffs;
  size_t   len;
} vhist_t;

in3_ret_t diff_emplace(vhist_t* h, uint16_t pos, bytes_t* b, uint64_t blk, size_t len) {
  vdiff_t* d_ = _realloc(h->diffs, sizeof(*d_) * (h->len + 1), sizeof(*d_) * h->len);
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

static void test_vlist_diff() {
  char*       nodeliststr = filetostr("/Users/sufi-al-hussaini/in3-core/test/tobalaba_nodelist.json");
  json_ctx_t* jnl         = parse_json(nodeliststr);
  if (jnl == NULL) return;

  d_token_t *      ss = d_get(jnl->result, K_STATES), *vs = NULL;
  bytes_builder_t* bb  = bb_new();
  bytes_t*         b   = NULL;
  in3_ret_t        ret = IN3_OK;
  vhist_t          h   = {.diffs = NULL, .len = 0};
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
          diff_emplace(&h, i, b, blk, d_len(vs) * 20);
          bb_write_fixed_bytes(bb, b);
        } else if (ret != i) {
          //          printf("-/+ %d, %d [%" PRIu64 "]\n", ret, i, blk);
          diff_emplace(&h, i, b, blk, d_len(vs) * 20);
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
  bb         = bb_new();
  vdiff_t* d = NULL;
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

/*
 * Main
 */
int main() {
  TESTS_BEGIN();
  //  RUN_TEST(test_bitset_op);
  //  RUN_TEST(test_bitset_transition);
  //  RUN_TEST(test_bitset_realloc);
  //  RUN_TEST(test_bitset_edge_cases);
  //  RUN_TEST(test_bitset_clone);
  //  RUN_TEST(test_bitset_out_of_range);
  RUN_TEST(test_vlist_diff);
  return TESTS_END();
}
