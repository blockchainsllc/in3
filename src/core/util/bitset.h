#ifndef IN3_BITSET_H
#define IN3_BITSET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "error.h"

#ifndef BS_MAX
#define BS_MAX 64
#endif

#if BS_MAX == 64
typedef uint64_t uintbs_t;
#elif BS_MAX == 32
typedef uint32_t uintbs_t;
#else
#error "Unsupported value for BS_MAX"
#endif

// Helper macros
#define BIT_SET(_a_, _b_) ((_a_) |= (1ULL << (_b_)))
#define BIT_CLEAR(_a_, _b_) ((_a_) &= ~(1ULL << (_b_)))
#define BIT_FLIP(_a_, _b_) ((_a_) ^= (1ULL << (_b_)))
#define BIT_CHECK(_a_, _b_) (!!((_a_) & (1ULL << (_b_))))
#define BITMASK_SET(_x_, _y_) ((_x_) |= (_y_))
#define BITMASK_CLEAR(_x_, _y_) ((_x_) &= (~(_y_)))
#define BITMASK_FLIP(_x_, _y_) ((_x_) ^= (_y_))
#define BITMASK_CHECK_ALL(_x_, _y_) (((_x_) & (_y_)) == (_y_))
#define BITMASK_CHECK_ANY(_x_, _y_) ((_x_) & (_y_))

#define bs_set(_bs_, _pos_) bs_modify(_bs_, _pos_, BS_SET)
#define bs_clear(_bs_, _pos_) bs_modify(_bs_, _pos_, BS_CLEAR)
#define bs_toggle(_bs_, _pos_) bs_modify(_bs_, _pos_, BS_TOGGLE)

typedef struct {
  union {
    uintbs_t b;
    uint8_t* p;
  } bits;
  size_t len; // length in bits, guaranteed to be multiple of 8
} bitset_t;

typedef enum {
  BS_SET = 0,
  BS_CLEAR,
  BS_TOGGLE
} bs_op_t;

bitset_t* bs_new(size_t len);
void      bs_free(bitset_t* bs);
bool      bs_isset(bitset_t* bs, size_t pos);
in3_ret_t bs_modify(bitset_t* bs, size_t pos, bs_op_t op); // will reallocate if pos is greater than BS_MAX and initial length
bool      bs_isempty(bitset_t* bs);
bitset_t* bs_clone(bitset_t* bs);

#endif //IN3_BITSET_H
