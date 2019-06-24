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

static void bb_print(bytes_builder_t* bb) {
  for (int i = 0; i < bb->b.len; i += 20) {
    ba_print(&bb->b.data[i], 20);
    printf("\n");
  }
}

static bool vh_diff_matches(uint64_t block) {
  char*       nodeliststr = filetostr("/Users/sufi-al-hussaini/in3-core/test/testdata/tobalaba_nodelist.json");
  json_ctx_t* jnl         = parse_json(nodeliststr);
  if (jnl == NULL) return false;
  vhist_t*         vh = vh_init(jnl);
  bytes_builder_t* bb = vh_get_for_block(vh, block);
  //  bb_print(bb);

  uint64_t         blk = 0;
  bytes_t*         b   = NULL;
  bytes_builder_t* bb_ = bb_new();
  d_token_t *      ss = d_get(jnl->result, K_STATES), *vs = NULL;
  for (d_iterator_t sitr = d_iter(ss); sitr.left; d_iter_next(&sitr)) {
    vs  = d_get(sitr.token, K_VALIDATORS);
    blk = d_get_longk(sitr.token, K_BLOCK);
    if (blk > block) break;
    bb_clear(bb_);
    if (d_type(vs) == T_ARRAY) {
      for (d_iterator_t vitr = d_iter(vs); vitr.left; d_iter_next(&vitr)) {
        bb_write_fixed_bytes(bb_, d_bytesl(vitr.token, 20));
      }
    }
  }
  //  printf("-------\n");
  //  bb_print(bb_);

  bool ret = bytes_cmp(bb->b, bb_->b) != 0;
  bb_free(bb_);
  bb_free(bb);
  vh_free(vh);
  free_json(jnl);
  _free(nodeliststr);
  return ret;
}

static void test_vh_diff() {
  TEST_ASSERT_TRUE(vh_diff_matches(11540950));
  TEST_ASSERT_TRUE(vh_diff_matches(11540919));
  TEST_ASSERT_TRUE(vh_diff_matches(11540920));
  TEST_ASSERT_TRUE(vh_diff_matches(11540918));
  TEST_ASSERT_TRUE(vh_diff_matches(12851669));
  TEST_ASSERT_TRUE(vh_diff_matches(10120180));
  TEST_ASSERT_TRUE(vh_diff_matches(9814665));
  TEST_ASSERT_TRUE(vh_diff_matches(4723940));
  TEST_ASSERT_TRUE(vh_diff_matches(1946064));
  TEST_ASSERT_TRUE(vh_diff_matches(582));
  TEST_ASSERT_TRUE(vh_diff_matches(0));
}

/*
 * Main
 */
int main() {
  TESTS_BEGIN();
  RUN_TIMED_TEST(test_vh_diff);
  return TESTS_END();
}
