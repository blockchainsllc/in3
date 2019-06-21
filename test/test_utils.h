#ifndef IN3_TEST_UTILS_H
#define IN3_TEST_UTILS_H

#include "unity/unity.h"
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TESTS_BEGIN() UNITY_BEGIN()
#define TESTS_END() UNITY_END()
#define TEST_LOG(fmt_, ...) printf("%s:%d:%s:LOG:" fmt_, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define TEST_LOG_INTERNAL(f_, fmt_, ...) printf("%s:%d:%s:LOG:" fmt_, __FILE__, __LINE__, f_, __VA_ARGS__)

// Timing
#define TIMING_START() gettimeofday(&begin, NULL)
#define TIMING_END() gettimeofday(&end, NULL)
#define TIMING_GET() ((double) (end.tv_usec - begin.tv_usec) / 1000000 + (double) (end.tv_sec - begin.tv_sec))

#define RUN_TIMED_TEST(t)                                      \
  do {                                                         \
    TIMING_START();                                            \
    RUN_TEST(t);                                               \
    TIMING_END();                                              \
    TEST_LOG_INTERNAL(#t, "Completed in %fs\n", TIMING_GET()); \
  } while (0)

#ifdef __cplusplus
}
#endif

#endif //IN3_TEST_UTILS_H
