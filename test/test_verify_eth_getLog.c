#ifndef TEST
#define TEST
#endif

#include <core/util/bytes.h>
#include <core/util/data.h>
#include <core/util/log.h>
#include <stdio.h>
#include <string.h>

#include "test_utils.h"

extern bool matches_filter(d_token_t* req, bytes_t addrs, uint64_t blockno, bytes_t blockhash, d_token_t* topics);

static bytes_t hexstrtobin(const char* hexstr) {
  bytes_t     b   = {.data = NULL, .len = 0};
  size_t      sz  = strlen(hexstr);
  const char* pos = hexstr;
  if (pos[0] == '0' && pos[1] == 'x') {
    pos += 2;
    sz -= 2;
  }
  const uint8_t odd   = ((sz % 2) == 1);
  const uint8_t bytes = (sz / 2) + odd;
  uint8_t*      val   = malloc(bytes);
  if (val != NULL) {
    for (size_t i = 0; i < bytes; i++) {
      if (sscanf(pos, (!i && odd) ? "%1hhx" : "%2hhx", (unsigned char*) &val[i]) != 1) {
        free(val);
        return b;
      }
      pos += (!i && odd) ? 1 : 2;
    }
  }
  b.data = val;
  b.len  = bytes;
  return b;
}

static void test_verify_eth_getLog_filter_default() {
  json_ctx_t* jreq    = parse_json("{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_getLogs\",\"params\":[{}],\"in3\":{\"chainId\":\"0x1\",\"verification\":\"proof\"}}");
  json_ctx_t* jtopics = parse_json("[\"0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef\",\"0x0000000000000000000000004e339b727b62103a4429b56bffd5609e92574f2d\",\"0x00000000000000000000000099fe5d6383289cdd56e54fc0baf7f67c957a8888\"]");
  bytes_t     addrs   = hexstrtobin("0x8912358d977e123b51ecad1ffa0cc4a7e32ff774");
  bytes_t     blkhash = hexstrtobin("0x6936d98945a4dec4c4c98052f429435f7f8ae45ea02f9503a67ae296014bef27");
  TEST_ASSERT_TRUE(matches_filter(&jreq->result[0], addrs, 0x793426, blkhash, &jtopics->result[0]));
  free(blkhash.data);
  free(addrs.data);
  free_json(jtopics);
  free_json(jreq);
}

static void test_verify_eth_getLog_filter_address() {
  json_ctx_t* jreq    = parse_json("{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_getLogs\",\"params\":[{\"address\": \"0x44e4963f9012e7a7aeee05b7f2caae3419557aea\"}],\"in3\":{\"chainId\":\"0x1\",\"verification\":\"proof\"}}");
  json_ctx_t* jtopics = parse_json("[\"0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef\",\"0x0000000000000000000000004e339b727b62103a4429b56bffd5609e92574f2d\",\"0x00000000000000000000000099fe5d6383289cdd56e54fc0baf7f67c957a8888\"]");
  bytes_t     addrs   = hexstrtobin("0x8912358d977e123b51ecad1ffa0cc4a7e32ff774");
  bytes_t     blkhash = hexstrtobin("0x6936d98945a4dec4c4c98052f429435f7f8ae45ea02f9503a67ae296014bef27");
  TEST_ASSERT_FALSE(matches_filter(&jreq->result[0], addrs, 0x793426, blkhash, &jtopics->result[0]));
  free_json(jreq);
  jreq = parse_json("{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_getLogs\",\"params\":[{\"address\": \"0x8912358d977e123b51ecad1ffa0cc4a7e32ff774\"}],\"in3\":{\"chainId\":\"0x1\",\"verification\":\"proof\"}}");
  TEST_ASSERT_TRUE(matches_filter(&jreq->result[0], addrs, 0x793426, blkhash, &jtopics->result[0]));
  free(blkhash.data);
  free(addrs.data);
  free_json(jtopics);
  free_json(jreq);
}

static void test_verify_eth_getLog_filter_range() {
  json_ctx_t* jreq    = parse_json("{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_getLogs\",\"params\":[{\"fromBlock\": \"0x793422\", \"toBlock\": \"0x793490\"}],\"in3\":{\"chainId\":\"0x1\",\"verification\":\"proof\"}}");
  json_ctx_t* jtopics = parse_json("[\"0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef\",\"0x0000000000000000000000004e339b727b62103a4429b56bffd5609e92574f2d\",\"0x00000000000000000000000099fe5d6383289cdd56e54fc0baf7f67c957a8888\"]");
  bytes_t     addrs   = hexstrtobin("0x8912358d977e123b51ecad1ffa0cc4a7e32ff774");
  bytes_t     blkhash = hexstrtobin("0x6936d98945a4dec4c4c98052f429435f7f8ae45ea02f9503a67ae296014bef27");
  TEST_ASSERT_TRUE(matches_filter(&jreq->result[0], addrs, 0x793426, blkhash, &jtopics->result[0]));
  TEST_ASSERT_FALSE(matches_filter(&jreq->result[0], addrs, 0x793421, blkhash, &jtopics->result[0]));
  TEST_ASSERT_TRUE(matches_filter(&jreq->result[0], addrs, 0x793422, blkhash, &jtopics->result[0]));
  TEST_ASSERT_TRUE(matches_filter(&jreq->result[0], addrs, 0x793489, blkhash, &jtopics->result[0]));
  TEST_ASSERT_FALSE(matches_filter(&jreq->result[0], addrs, 0x793491, blkhash, &jtopics->result[0]));
  TEST_ASSERT_TRUE(matches_filter(&jreq->result[0], addrs, 0x793490, blkhash, &jtopics->result[0]));
  free(blkhash.data);
  free(addrs.data);
  free_json(jtopics);
  free_json(jreq);
}

static void test_verify_eth_getLog_filter_blockhash() {
  json_ctx_t* jreq    = parse_json("{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_getLogs\",\"params\":[{\"blockHash\": \"0x990bf8675bd3d808834f627ace83ede18c708768d5693b95915a85bbe9aa7aaf\"}],\"in3\":{\"chainId\":\"0x1\",\"verification\":\"proof\"}}");
  json_ctx_t* jtopics = parse_json("[\"0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef\",\"0x0000000000000000000000004e339b727b62103a4429b56bffd5609e92574f2d\",\"0x00000000000000000000000099fe5d6383289cdd56e54fc0baf7f67c957a8888\"]");
  bytes_t     addrs   = hexstrtobin("0x8912358d977e123b51ecad1ffa0cc4a7e32ff774");
  bytes_t     blkhash = hexstrtobin("0x6936d98945a4dec4c4c98052f429435f7f8ae45ea02f9503a67ae296014bef27");
  TEST_ASSERT_FALSE(matches_filter(&jreq->result[0], addrs, 0x793426, blkhash, &jtopics->result[0]));
  free_json(jreq);
  jreq = parse_json("{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_getLogs\",\"params\":[{\"blockHash\": \"0x6936d98945a4dec4c4c98052f429435f7f8ae45ea02f9503a67ae296014bef27\"}],\"in3\":{\"chainId\":\"0x1\",\"verification\":\"proof\"}}");
  TEST_ASSERT_TRUE(matches_filter(&jreq->result[0], addrs, 0x793426, blkhash, &jtopics->result[0]));
  free(blkhash.data);
  free(addrs.data);
  free_json(jtopics);
  free_json(jreq);
}

static void test_verify_eth_getLog_filter_topics() {
  json_ctx_t* jreq    = parse_json("{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_getLogs\",\"params\":[{\"topics\": [\"0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef\"]}],\"in3\":{\"chainId\":\"0x1\",\"verification\":\"proof\"}}");
  json_ctx_t* jtopics = parse_json("[\"0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef\",\"0x0000000000000000000000004e339b727b62103a4429b56bffd5609e92574f2d\",\"0x00000000000000000000000099fe5d6383289cdd56e54fc0baf7f67c957a8888\"]");
  bytes_t     addrs   = hexstrtobin("0x8912358d977e123b51ecad1ffa0cc4a7e32ff774");
  bytes_t     blkhash = hexstrtobin("0x6936d98945a4dec4c4c98052f429435f7f8ae45ea02f9503a67ae296014bef27");
  TEST_ASSERT_TRUE(matches_filter(&jreq->result[0], addrs, 0x793426, blkhash, &jtopics->result[0]));
  free_json(jreq);
  jreq = parse_json("{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_getLogs\",\"params\":[{\"topics\": [null, \"0x0000000000000000000000004e339b727b62103a4429b56bffd5609e92574f2d\"]}],\"in3\":{\"chainId\":\"0x1\",\"verification\":\"proof\"}}");
  TEST_ASSERT_TRUE(matches_filter(&jreq->result[0], addrs, 0x793426, blkhash, &jtopics->result[0]));
  free_json(jreq);
  jreq = parse_json("{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_getLogs\",\"params\":[{\"topics\": [null, [\"0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef\", \"0x00000000000000000000000099fe5d6383289cdd56e54fc0baf7f67c957a8888\"]]}],\"in3\":{\"chainId\":\"0x1\",\"verification\":\"proof\"}}");
  TEST_ASSERT_FALSE(matches_filter(&jreq->result[0], addrs, 0x793426, blkhash, &jtopics->result[0]));
  free_json(jreq);
  jreq = parse_json("{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_getLogs\",\"params\":[{\"topics\": [[\"0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef\", \"0x00000000000000000000000099fe5d6383289cdd56e54fc0baf7f67c957a8888\"], null, \"0x00000000000000000000000099fe5d6383289cdd56e54fc0baf7f67c957a8888\"]}],\"in3\":{\"chainId\":\"0x1\",\"verification\":\"proof\"}}");
  TEST_ASSERT_TRUE(matches_filter(&jreq->result[0], addrs, 0x793426, blkhash, &jtopics->result[0]));
  free(blkhash.data);
  free(addrs.data);
  free_json(jtopics);
  free_json(jreq);
}

int main() {
  in3_log_set_quiet(1);
  TESTS_BEGIN();
  RUN_TEST(test_verify_eth_getLog_filter_default);
  RUN_TEST(test_verify_eth_getLog_filter_address);
  RUN_TEST(test_verify_eth_getLog_filter_range);
  RUN_TEST(test_verify_eth_getLog_filter_blockhash);
  RUN_TEST(test_verify_eth_getLog_filter_topics);
  return TESTS_END();
}
