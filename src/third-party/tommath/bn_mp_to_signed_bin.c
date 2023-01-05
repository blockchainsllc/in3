#include "tommath_private.h"
#ifdef BN_MP_TO_SIGNED_BIN_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* store in signed [big endian] format */
int mp_to_signed_bin(const mp_int *a, unsigned char *b)
{
   int     res;

   if ((res = mp_to_unsigned_bin(a, b + 1)) != MP_OKAY) {
      return res;
   }
   b[0] = (a->sign == MP_ZPOS) ? (unsigned char)0 : (unsigned char)1;
   return MP_OKAY;
}
#endif
