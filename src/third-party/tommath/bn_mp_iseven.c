#include "tommath_private.h"
#ifdef BN_MP_ISEVEN_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

int mp_iseven(const mp_int *a)
{
   return IS_EVEN(a) ? MP_YES : MP_NO;
}
#endif
