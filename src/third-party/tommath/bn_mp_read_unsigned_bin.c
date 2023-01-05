#include "tommath_private.h"
#ifdef BN_MP_READ_UNSIGNED_BIN_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* reads a unsigned char array, assumes the msb is stored first [big endian] */
int mp_read_unsigned_bin(mp_int *a, const unsigned char *b, int c)
{
   int     res;

   /* make sure there are at least two digits */
   if (a->alloc < 2) {
      if ((res = mp_grow(a, 2)) != MP_OKAY) {
         return res;
      }
   }

   /* zero the int */
   mp_zero(a);

   /* read the bytes in */
   while (c-- > 0) {
      if ((res = mp_mul_2d(a, 8, a)) != MP_OKAY) {
         return res;
      }

#ifndef MP_8BIT
      a->dp[0] |= *b++;
      a->used += 1;
#else
      a->dp[0] = (*b & MP_MASK);
      a->dp[1] |= ((*b++ >> 7) & 1u);
      a->used += 2;
#endif
   }
   mp_clamp(a);
   return MP_OKAY;
}
#endif
