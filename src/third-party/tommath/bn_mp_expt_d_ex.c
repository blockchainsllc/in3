#include "tommath_private.h"
#ifdef BN_MP_EXPT_D_EX_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* calculate c = a**b  using a square-multiply algorithm */
int mp_expt_d_ex(const mp_int *a, mp_digit b, mp_int *c, int fast)
{
   int     res;
   unsigned int x;

   mp_int  g;

   if ((res = mp_init_copy(&g, a)) != MP_OKAY) {
      return res;
   }

   /* set initial result */
   mp_set(c, 1uL);

   if (fast != 0) {
      while (b > 0u) {
         /* if the bit is set multiply */
         if ((b & 1u) != 0u) {
            if ((res = mp_mul(c, &g, c)) != MP_OKAY) {
               mp_clear(&g);
               return res;
            }
         }

         /* square */
         if (b > 1u) {
            if ((res = mp_sqr(&g, &g)) != MP_OKAY) {
               mp_clear(&g);
               return res;
            }
         }

         /* shift to next bit */
         b >>= 1;
      }
   } else {
      for (x = 0; x < (unsigned)DIGIT_BIT; x++) {
         /* square */
         if ((res = mp_sqr(c, c)) != MP_OKAY) {
            mp_clear(&g);
            return res;
         }

         /* if the bit is set multiply */
         if ((b & ((mp_digit)1 << (DIGIT_BIT - 1))) != 0u) {
            if ((res = mp_mul(c, &g, c)) != MP_OKAY) {
               mp_clear(&g);
               return res;
            }
         }

         /* shift to next bit */
         b <<= 1;
      }
   } /* if ... else */

   mp_clear(&g);
   return MP_OKAY;
}
#endif
