#include "tommath_private.h"
#ifdef BN_MP_DR_IS_MODULUS_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* determines if a number is a valid DR modulus */
int mp_dr_is_modulus(const mp_int *a)
{
   int ix;

   /* must be at least two digits */
   if (a->used < 2) {
      return 0;
   }

   /* must be of the form b**k - a [a <= b] so all
    * but the first digit must be equal to -1 (mod b).
    */
   for (ix = 1; ix < a->used; ix++) {
      if (a->dp[ix] != MP_MASK) {
         return 0;
      }
   }
   return 1;
}

#endif
