#ifndef IN3_STACK_DIAG_H
#define IN3_STACK_DIAG_H

#include "platform.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern size_t stack_top;
extern size_t stack_size;
extern size_t stack_max_used;

#if PLATFORM_IS_POSIX(PLATFORM)
#include <sys/resource.h>
#define STACK_TOP() STACK_TOP_POSIX()
#elif
#define STACK_TOP()
#endif

#define STACK_TOP_POSIX()                                \
  do {                                                   \
    int x_    = 0;                                       \
    stack_top = (size_t) &x_;                            \
    struct rlimit rl;                                    \
    int           result = getrlimit(RLIMIT_STACK, &rl); \
    if (result == 0) {                                   \
      stack_size = rl.rlim_cur;                          \
    }                                                    \
  } while (0)

#define STACK_CURR()                                     \
  do {                                                   \
    int    x_         = 0;                               \
    size_t stack_used = stack_top - (size_t) &x_;        \
    stack_max_used    = max(stack_max_used, stack_used); \
  } while (0)

#define STACK_USED() stack_max_used
#define STACK_SZ() stack_size

#ifdef __cplusplus
}
#endif

#endif //IN3_STACK_DIAG_H
