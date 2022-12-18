#include "tommath_private.h"
#ifdef BN_MP_SET_DOUBLE_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

#if defined(__STDC_IEC_559__) || defined(__GCC_IEC_559)
int mp_set_double(mp_int *a, double b)
{
   uint64_t frac;
   int exp, res;
   union {
      double   dbl;
      uint64_t bits;
   } cast;
   cast.dbl = b;

   exp = (int)((unsigned)(cast.bits >> 52) & 0x7FFU);
   frac = (cast.bits & ((1ULL << 52) - 1ULL)) | (1ULL << 52);

   if (exp == 0x7FF) { /* +-inf, NaN */
      return MP_VAL;
   }
   exp -= 1023 + 52;

   res = mp_set_long_long(a, frac);
   if (res != MP_OKAY) {
      return res;
   }

   res = (exp < 0) ? mp_div_2d(a, -exp, a, NULL) : mp_mul_2d(a, exp, a);
   if (res != MP_OKAY) {
      return res;
   }

   if (((cast.bits >> 63) != 0ULL) && !IS_ZERO(a)) {
      a->sign = MP_NEG;
   }

   return MP_OKAY;
}
#else
/* pragma message() not supported by several compilers (in mostly older but still used versions) */
#  ifdef _MSC_VER
#    pragma message("mp_set_double implementation is only available on platforms with IEEE754 floating point format")
#  else
#    warning "mp_set_double implementation is only available on platforms with IEEE754 floating point format"
#  endif
#endif
#endif
