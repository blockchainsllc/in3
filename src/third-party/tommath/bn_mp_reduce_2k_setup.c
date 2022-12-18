#include "tommath_private.h"
#ifdef BN_MP_REDUCE_2K_SETUP_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* determines the setup value */
int mp_reduce_2k_setup(const mp_int *a, mp_digit *d)
{
   int res, p;
   mp_int tmp;

   if ((res = mp_init(&tmp)) != MP_OKAY) {
      return res;
   }

   p = mp_count_bits(a);
   if ((res = mp_2expt(&tmp, p)) != MP_OKAY) {
      mp_clear(&tmp);
      return res;
   }

   if ((res = s_mp_sub(&tmp, a, &tmp)) != MP_OKAY) {
      mp_clear(&tmp);
      return res;
   }

   *d = tmp.dp[0];
   mp_clear(&tmp);
   return MP_OKAY;
}
#endif
