#include "tommath_private.h"
#ifdef BN_MP_MOD_2D_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* calc a value mod 2**b */
int mp_mod_2d(const mp_int *a, int b, mp_int *c)
{
   int     x, res;

   /* if b is <= 0 then zero the int */
   if (b <= 0) {
      mp_zero(c);
      return MP_OKAY;
   }

   /* if the modulus is larger than the value than return */
   if (b >= (a->used * DIGIT_BIT)) {
      res = mp_copy(a, c);
      return res;
   }

   /* copy */
   if ((res = mp_copy(a, c)) != MP_OKAY) {
      return res;
   }

   /* zero digits above the last digit of the modulus */
   for (x = (b / DIGIT_BIT) + (((b % DIGIT_BIT) == 0) ? 0 : 1); x < c->used; x++) {
      c->dp[x] = 0;
   }
   /* clear the digit that is not completely outside/inside the modulus */
   c->dp[b / DIGIT_BIT] &=
      ((mp_digit)1 << (mp_digit)(b % DIGIT_BIT)) - (mp_digit)1;
   mp_clamp(c);
   return MP_OKAY;
}
#endif
