#ifndef IN3_TEST_UTILS_H
#define IN3_TEST_UTILS_H

#include "unity/unity.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TESTS_BEGIN() UNITY_BEGIN()
#define TESTS_END()   UNITY_END()

// Timing
#define TIMING_START() gettimeofday(&begin, NULL)
#define TIMING_END() gettimeofday(&end, NULL)
#define TIMING_GET() ((double) (end.tv_usec - begin.tv_usec) / 1000000 + (double) (end.tv_sec - begin.tv_sec))
#define TIMING_PRINT() fprintf(stdout, "[Completed in %fs]\n", TIMING_GET())

#define RUN_TIMED_TEST(t) \
  TIMING_START();         \
  RUN_TEST(t);            \
  TIMING_END();           \
  TIMING_PRINT();

#ifdef __cplusplus
}
#endif

#endif //IN3_TEST_UTILS_H
