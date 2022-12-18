#include "tommath_private.h"
#ifdef BN_FAST_MP_INVMOD_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* computes the modular inverse via binary extended euclidean algorithm,
 * that is c = 1/a mod b
 *
 * Based on slow invmod except this is optimized for the case where b is
 * odd as per HAC Note 14.64 on pp. 610
 */
int fast_mp_invmod(const mp_int *a, const mp_int *b, mp_int *c)
{
   mp_int  x, y, u, v, B, D;
   int     res, neg;

   /* 2. [modified] b must be odd   */
   if (IS_EVEN(b)) {
      return MP_VAL;
   }

   /* init all our temps */
   if ((res = mp_init_multi(&x, &y, &u, &v, &B, &D, NULL)) != MP_OKAY) {
      return res;
   }

   /* x == modulus, y == value to invert */
   if ((res = mp_copy(b, &x)) != MP_OKAY) {
      goto LBL_ERR;
   }

   /* we need y = |a| */
   if ((res = mp_mod(a, b, &y)) != MP_OKAY) {
      goto LBL_ERR;
   }

   /* if one of x,y is zero return an error! */
   if (IS_ZERO(&x) || IS_ZERO(&y)) {
      res = MP_VAL;
      goto LBL_ERR;
   }

   /* 3. u=x, v=y, A=1, B=0, C=0,D=1 */
   if ((res = mp_copy(&x, &u)) != MP_OKAY) {
      goto LBL_ERR;
   }
   if ((res = mp_copy(&y, &v)) != MP_OKAY) {
      goto LBL_ERR;
   }
   mp_set(&D, 1uL);

top:
   /* 4.  while u is even do */
   while (IS_EVEN(&u)) {
      /* 4.1 u = u/2 */
      if ((res = mp_div_2(&u, &u)) != MP_OKAY) {
         goto LBL_ERR;
      }
      /* 4.2 if B is odd then */
      if (IS_ODD(&B)) {
         if ((res = mp_sub(&B, &x, &B)) != MP_OKAY) {
            goto LBL_ERR;
         }
      }
      /* B = B/2 */
      if ((res = mp_div_2(&B, &B)) != MP_OKAY) {
         goto LBL_ERR;
      }
   }

   /* 5.  while v is even do */
   while (IS_EVEN(&v)) {
      /* 5.1 v = v/2 */
      if ((res = mp_div_2(&v, &v)) != MP_OKAY) {
         goto LBL_ERR;
      }
      /* 5.2 if D is odd then */
      if (IS_ODD(&D)) {
         /* D = (D-x)/2 */
         if ((res = mp_sub(&D, &x, &D)) != MP_OKAY) {
            goto LBL_ERR;
         }
      }
      /* D = D/2 */
      if ((res = mp_div_2(&D, &D)) != MP_OKAY) {
         goto LBL_ERR;
      }
   }

   /* 6.  if u >= v then */
   if (mp_cmp(&u, &v) != MP_LT) {
      /* u = u - v, B = B - D */
      if ((res = mp_sub(&u, &v, &u)) != MP_OKAY) {
         goto LBL_ERR;
      }

      if ((res = mp_sub(&B, &D, &B)) != MP_OKAY) {
         goto LBL_ERR;
      }
   } else {
      /* v - v - u, D = D - B */
      if ((res = mp_sub(&v, &u, &v)) != MP_OKAY) {
         goto LBL_ERR;
      }

      if ((res = mp_sub(&D, &B, &D)) != MP_OKAY) {
         goto LBL_ERR;
      }
   }

   /* if not zero goto step 4 */
   if (!IS_ZERO(&u)) {
      goto top;
   }

   /* now a = C, b = D, gcd == g*v */

   /* if v != 1 then there is no inverse */
   if (mp_cmp_d(&v, 1uL) != MP_EQ) {
      res = MP_VAL;
      goto LBL_ERR;
   }

   /* b is now the inverse */
   neg = a->sign;
   while (D.sign == MP_NEG) {
      if ((res = mp_add(&D, b, &D)) != MP_OKAY) {
         goto LBL_ERR;
      }
   }

   /* too big */
   while (mp_cmp_mag(&D, b) != MP_LT) {
      if ((res = mp_sub(&D, b, &D)) != MP_OKAY) {
         goto LBL_ERR;
      }
   }

   mp_exch(&D, c);
   c->sign = neg;
   res = MP_OKAY;

LBL_ERR:
   mp_clear_multi(&x, &y, &u, &v, &B, &D, NULL);
   return res;
}
#endif
