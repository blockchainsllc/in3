#include "tommath_private.h"
#ifdef BN_MP_XOR_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* XOR two ints together */
int mp_xor(const mp_int *a, const mp_int *b, mp_int *c)
{
   int     res, ix, px;
   mp_int  t;
   const mp_int *x;

   if (a->used > b->used) {
      if ((res = mp_init_copy(&t, a)) != MP_OKAY) {
         return res;
      }
      px = b->used;
      x = b;
   } else {
      if ((res = mp_init_copy(&t, b)) != MP_OKAY) {
         return res;
      }
      px = a->used;
      x = a;
   }

   for (ix = 0; ix < px; ix++) {
      t.dp[ix] ^= x->dp[ix];
   }
   mp_clamp(&t);
   mp_exch(c, &t);
   mp_clear(&t);
   return MP_OKAY;
}
#endif
