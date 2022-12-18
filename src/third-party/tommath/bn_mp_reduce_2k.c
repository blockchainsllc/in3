#include "tommath_private.h"
#ifdef BN_MP_REDUCE_2K_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* reduces a modulo n where n is of the form 2**p - d */
int mp_reduce_2k(mp_int *a, const mp_int *n, mp_digit d)
{
   mp_int q;
   int    p, res;

   if ((res = mp_init(&q)) != MP_OKAY) {
      return res;
   }

   p = mp_count_bits(n);
top:
   /* q = a/2**p, a = a mod 2**p */
   if ((res = mp_div_2d(a, p, &q, a)) != MP_OKAY) {
      goto LBL_ERR;
   }

   if (d != 1u) {
      /* q = q * d */
      if ((res = mp_mul_d(&q, d, &q)) != MP_OKAY) {
         goto LBL_ERR;
      }
   }

   /* a = a + q */
   if ((res = s_mp_add(a, &q, a)) != MP_OKAY) {
      goto LBL_ERR;
   }

   if (mp_cmp_mag(a, n) != MP_LT) {
      if ((res = s_mp_sub(a, n, a)) != MP_OKAY) {
         goto LBL_ERR;
      }
      goto top;
   }

LBL_ERR:
   mp_clear(&q);
   return res;
}

#endif
