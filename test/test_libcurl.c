#ifndef TEST
#define TEST
#endif

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <transport_curl/in3_curl.h>

#include "test_utils.h"

static struct timeval begin, end;

static const char* test_urls[] = {
    "https://reqres.in",
    "https://www.microsoft.com",
    "https://opensource.org",
    "https://www.google.com",
    "https://www.yahoo.com",
    "https://www.ibm.com",
    "https://www.mysql.com",
    "https://www.oracle.com",
    "https://www.ripe.net",
    "https://www.iana.org",
    "https://www.amazon.com",
    "https://www.netcraft.com",
    "https://www.heise.de",
    "https://www.chip.de",
    "https://www.ca.com",
    "https://www.cnet.com",
    "https://www.mozilla.org",
    "https://www.cnn.com",
    "https://www.wikipedia.org",
    "https://www.dell.com",
    "https://www.hp.com",
    "https://www.cert.org",
    "https://www.mit.edu",
    "https://www.nist.gov",
    "https://www.ebay.com",
    "https://www.playstation.com",
    "https://www.uefa.com",
    "https://www.ieee.org",
    "https://www.apple.com",
    "https://www.symantec.com",
    "https://www.zdnet.com",
    "https://www.fujitsu.com/global/",
    "https://www.supermicro.com",
    "https://www.hotmail.com",
    "https://www.ietf.org",
    "https://www.bbc.co.uk",
    "https://news.google.com",
    "https://www.foxnews.com",
    "https://www.msn.com",
    "https://www.wired.com",
    "https://www.sky.com",
    "https://www.usatoday.com",
    "https://www.cbs.com",
    "https://www.nbc.com/",
    "https://slashdot.org",
    "https://www.informationweek.com",
    "https://apache.org",
    "https://www.un.org",
};

#define NUM_URLS (sizeof(test_urls) / sizeof(char*))

/*
 * Function declarations
 */
extern int  send_curl_blocking(const char** urls, int urls_len, char* payload, in3_response_t* result);
extern int  send_curl_nonblocking(const char** urls, int urls_len, char* payload, in3_response_t* result);
static void test_send_curl_blocking();
static void test_send_curl_nonblocking();
static void test_send_curl_match_responses();
static void test_send_curl_timing();

/*
 * Main
 */
int main() {
  TESTS_BEGIN();
  RUN_TIMED_TEST(test_send_curl_blocking);
  RUN_TIMED_TEST(test_send_curl_nonblocking);
  RUN_TEST(test_send_curl_match_responses);
  RUN_TEST(test_send_curl_timing);
  return TESTS_END();
}

/*
 * Tests
 */
void test_send_curl_nonblocking() {
  in3_response_t* response = _malloc(sizeof(*response) * NUM_URLS);
  for (int n = 0; n < NUM_URLS; n++) {
    sb_init(&response[n].error);
    sb_init(&response[n].result);
  }

  TEST_ASSERT_EQUAL(0, send_curl_nonblocking(test_urls, NUM_URLS, "[]", response));

  for (int n = 0; n < NUM_URLS; n++) {
    // printf("[%s] > %lu\n", test_urls[n], response[n].result.len);
    _free(response[n].error.data);
    _free(response[n].result.data);
  }
  _free(response);
}

void test_send_curl_blocking() {
  in3_response_t* response = _malloc(sizeof(*response) * NUM_URLS);
  for (int n = 0; n < NUM_URLS; n++) {
    sb_init(&response[n].error);
    sb_init(&response[n].result);
  }

  TEST_ASSERT_EQUAL(0, send_curl_blocking(test_urls, NUM_URLS, "[]", response));

  for (int n = 0; n < NUM_URLS; n++) {
    // printf("[%s] > %lu\n", test_urls[n], response[n].result.len);
    _free(response[n].error.data);
    _free(response[n].result.data);
  }
  _free(response);
}

void test_send_curl_match_responses() {
  in3_response_t* response1 = _malloc(sizeof(in3_response_t));
  sb_init(&response1[0].error);
  sb_init(&response1[0].result);
  in3_response_t* response2 = _malloc(sizeof(in3_response_t));
  sb_init(&response2[0].error);
  sb_init(&response2[0].result);

  TEST_ASSERT_EQUAL(0, send_curl_blocking(test_urls, 1, "{ name: \"in3\", tests: [\"libcurl\"] }", response1));
  TEST_ASSERT_EQUAL(0, send_curl_nonblocking(test_urls, 1, "{ name: \"in3\", tests: [\"libcurl\"] }", response2));
  TEST_ASSERT_EQUAL(response1->result.len, response2->result.len);
  TEST_ASSERT_EQUAL(response1->error.len, response2->error.len);
  TEST_ASSERT_EQUAL_MEMORY(response1->result.data, response2->result.data, response1->result.len);

  _free(response1[0].error.data);
  _free(response1[0].result.data);
  _free(response1);
  _free(response2[0].error.data);
  _free(response2[0].result.data);
  _free(response2);
}

void test_send_curl_timing() {
  const int    count     = 100;
  const char*  localhost = "http://127.0.0.1";
  const size_t sz        = strlen(localhost) + 1;
  char**       ips       = _malloc(sizeof(*ips) * count);
  for (int i = 0; i < count; ++i) {
    ips[i] = _malloc(sz);
    strcpy(ips[i], localhost);
  }
  in3_response_t* response1 = _malloc(sizeof(*response1) * count);
  for (int n = 0; n < count; n++) {
    sb_init(&response1[n].error);
    sb_init(&response1[n].result);
  }
  in3_response_t* response2 = _malloc(sizeof(*response2) * count);
  for (int n = 0; n < count; n++) {
    sb_init(&response2[n].error);
    sb_init(&response2[n].result);
  }

  TIMING_START();
  TEST_ASSERT_EQUAL(0, send_curl_blocking((const char**) ips, count, "{ name: \"in3\", tests: [\"libcurl\"] }", response1));
  TIMING_END();
  double t1 = TIMING_GET();

  TIMING_START();
  TEST_ASSERT_EQUAL(0, send_curl_nonblocking((const char**) ips, count, "{ name: \"in3\", tests: [\"libcurl\"] }", response2));
  TIMING_END();
  double t2 = TIMING_GET();

  TEST_ASSERT_MESSAGE(t1 > t2, "Blocking version is faster! Compiling with -DCURL_BLOCKING is recommended.");

  for (int n = 0; n < count; n++) {
    _free(response1[n].error.data);
    _free(response1[n].result.data);
  }
  _free(response1);
  for (int n = 0; n < count; n++) {
    _free(response2[n].error.data);
    _free(response2[n].result.data);
  }
  _free(response2);
  _free(ips);
}
