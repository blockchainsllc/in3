/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 * 
 * 
 * COMMERCIAL LICENSE USAGE
 * 
 * Licensees holding a valid commercial license may use this file in accordance 
 * with the commercial license agreement provided with the Software or, alternatively, 
 * in accordance with the terms contained in a written agreement between you and 
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
 * information please contact slock.it at in3@slock.it.
 * 	
 * Alternatively, this file may be used under the AGPL license as follows:
 *    
 * AGPL LICENSE USAGE
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

#include "../../../core/util/mem.h"
#include "../../../core/util/utils.h"
#include "../../../third-party/crypto/ecdsa.h"
#include "../../../third-party/crypto/secp256k1.h"
#include "../../../third-party/tommath/tommath.h"
#include "evm.h"
#include "gas.h"
#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

static const uint8_t modulus_bin[] = {0x30, 0x64, 0x4e, 0x72, 0xe1, 0x31, 0xa0, 0x29, 0xb8, 0x50, 0x45, 0xb6, 0x81, 0x81, 0x58, 0x5d, 0x97, 0x81, 0x6a, 0x91, 0x68, 0x71, 0xca, 0x8d, 0x3c, 0x20, 0x8c, 0x16, 0xd8, 0x7c, 0xfd, 0x47};

typedef struct {
  mp_int x;
  mp_int y;
} ecc_point;

static ecc_point* ecc_new_point(void) {
  ecc_point* p;
  p = _calloc(1, sizeof(*p));
  if (p == NULL) {
    return NULL;
  }
  if (mp_init_multi(&p->x, &p->y, NULL) != MP_OKAY) {
    _free(p);
    return NULL;
  }
  return p;
}

static void ecc_del_point(ecc_point* p) {
  if (p != NULL) {
    mp_clear_multi(&p->x, &p->y, NULL);
    _free(p);
  }
}

static int ecc_set_point_xyz(mp_digit x, mp_digit y, ecc_point* p) {
  mp_set(&p->x, x);
  mp_set(&p->y, y);
  return MP_OKAY;
}

static int ecc_copy_point(const ecc_point* src, ecc_point* dst) {
  int err;
  if ((err = mp_copy(&src->x, &dst->x)) != MP_OKAY) return err;
  if ((err = mp_copy(&src->y, &dst->y)) != MP_OKAY) return err;
  return MP_OKAY;
}

static int ecc_is_point_at_infinity(const ecc_point* P, void* modulus, int* retval) {
  int    err = MP_OKAY;
  mp_int x3, y2;

  /* point (0,0,0) is point at infinity */
  if (mp_iszero(&P->x) && mp_iszero(&P->y)) {
    *retval = 1;
    return MP_OKAY;
  }

  /* initialize */
  if ((err = mp_init_multi(&x3, &y2, NULL)) != MP_OKAY) goto done;

  /* compute y^2 */
  if ((err = mp_mulmod(&P->y, &P->y, modulus, &y2)) != MP_OKAY) goto cleanup;

  /* compute x^3 */
  if ((err = mp_mulmod(&P->x, &P->x, modulus, &x3)) != MP_OKAY) goto cleanup;
  if ((err = mp_mulmod(&P->x, &x3, modulus, &x3)) != MP_OKAY) goto cleanup;

  /* test y^2 == x^3 */
  err = MP_OKAY;
  if ((mp_cmp(&x3, &y2) == MP_EQ) && !mp_iszero(&y2)) {
    *retval = 1;
  } else {
    *retval = 0;
  }

cleanup:
  mp_clear_multi(&x3, &y2, NULL);
done:
  return err;
}

// Check that a point is on the curve defined by y^2 == x^3 + b
static int ecc_is_point_on_curve(const ecc_point* P, mp_int* modulus, mp_int* b, int* oncurve) {
  int    err;
  mp_int t1, t2, t3;

  if ((err = mp_init_multi(&t1, &t2, &t3, NULL)) != MP_OKAY) { return err; }
  if ((err = ecc_is_point_at_infinity(P, modulus, oncurve)) != MP_OKAY) { goto done; }

  // t1 = x^3
  if ((err = mp_sqrmod(&P->x, modulus, &t1)) != MP_OKAY) { goto done; }
  if ((err = mp_mulmod(&t1, &P->x, modulus, &t1)) != MP_OKAY) { goto done; }

  // t2 = y^2
  if ((err = mp_sqrmod(&P->y, modulus, &t2)) != MP_OKAY) { goto done; }

  // t2 = y^2 - x^3
  if ((err = mp_submod(&t2, &t1, modulus, &t2)) != MP_OKAY) { goto done; }

  // y^2 - x^3 = b ?
  *oncurve = (mp_cmp(&t2, b) == MP_EQ);

done:
  mp_clear_multi(&t3, &t2, &t1, NULL);
  return err;
}

static bool ecc_point_validate(ecc_point* P, mp_int* modulus, mp_int* b) {
  if (mp_cmp(&P->x, modulus) != MP_LT) return false;
  if (mp_cmp(&P->y, modulus) != MP_LT) return false;
  if (!mp_iszero(&P->x) && !mp_iszero(&P->y)) {
    int oncurve = 0;
    ecc_is_point_on_curve(P, modulus, b, &oncurve);
    return oncurve != 0;
  } else if (mp_iszero(&P->x) && mp_iszero(&P->y)) {
    return true;
  }
  return false;
}

static int ecc_point_double(const ecc_point* P, ecc_point* R, mp_int* modulus) {
  mp_int t1, t2, t3, t4;
  int    err, inf;

  if ((err = mp_init_multi(&t1, &t2, &t3, &t4, NULL)) != MP_OKAY) {
    return err;
  }

  if (P != R) {
    if ((err = ecc_copy_point(P, R)) != MP_OKAY) { goto done; }
  }

  if ((err = ecc_is_point_at_infinity(P, modulus, &inf)) != MP_OKAY) return err;
  if (inf) {
    if ((err = ecc_copy_point(P, R)) != MP_OKAY) { goto done; }
  }

  // t1 = x^2
  if ((err = mp_sqrmod(&R->x, modulus, &t1)) != MP_OKAY) { goto done; }

  // t1 = 3*x^2
  mp_set(&t2, 3);
  if ((err = mp_mulmod(&t1, &t2, modulus, &t1)) != MP_OKAY) { goto done; }

  // t3 = 2*y
  mp_set(&t2, 2);
  if ((err = mp_mulmod(&R->y, &t2, modulus, &t3)) != MP_OKAY) { goto done; }

  // t3 = 1/(2*y)
  if ((err = mp_invmod(&t3, modulus, &t3)) != MP_OKAY) { goto done; }

  // t3 = 1/(2*y) * 3*x^2
  if ((err = mp_mulmod(&t3, &t1, modulus, &t3)) != MP_OKAY) { goto done; }

  // t4 = t3^2
  if ((err = mp_sqrmod(&t3, modulus, &t4)) != MP_OKAY) { goto done; }

  // t2 = 2*x
  if ((err = mp_mulmod(&R->x, &t2, modulus, &t2)) != MP_OKAY) { goto done; }

  // t1 = t3 * x
  if ((err = mp_mulmod(&t3, &R->x, modulus, &t1)) != MP_OKAY) { goto done; }

  // x = m^2 - 2*x
  if ((err = mp_submod(&t4, &t2, modulus, &R->x)) != MP_OKAY) { goto done; }

  // t4 = x
  if ((err = mp_copy(&R->x, &t4)) != MP_OKAY) { goto done; }

  // t2 = -t3
  if ((err = mp_neg(&t3, &t2)) != MP_OKAY) { goto done; }

  // t2 = -t3 * t4
  if ((err = mp_mulmod(&t2, &t4, modulus, &t2)) != MP_OKAY) { goto done; }

  // t2 = t1 + t2
  if ((err = mp_addmod(&t1, &t2, modulus, &t2)) != MP_OKAY) { goto done; }

  // y = t2 - y
  if ((err = mp_submod(&t2, &R->y, modulus, &R->y)) != MP_OKAY) { goto done; }

  err = MP_OKAY;
done:
  mp_clear_multi(&t4, &t3, &t2, &t1, NULL);
  return err;
}

static int ecc_point_add(const ecc_point* P, ecc_point* Q, ecc_point* R, mp_int* modulus) {
  int    inf = 0, err = MP_OKAY;
  mp_int t1, m;

  mp_init_multi(&t1, &m, NULL);

  // P is point-at-infinity, so result is Q
  if ((err = ecc_is_point_at_infinity(P, modulus, &inf)) != MP_OKAY) return err;
  if (inf) {
    err = ecc_copy_point(Q, R);
    goto done;
  }

  // Q is point-at-infinity, so result is P
  if ((err = ecc_is_point_at_infinity(Q, modulus, &inf)) != MP_OKAY) return err;
  if (inf) {
    err = ecc_copy_point(P, R);
    goto done;
  }

  if ((mp_cmp(&P->x, &Q->x) == MP_EQ)) {
    // P = Q, so result is P doubled
    if (mp_cmp(&P->y, &Q->y) == MP_EQ) {
      mp_clear_multi(&t1, m, NULL);
      return ecc_point_double(P, R, modulus);
    }

    // Q = -P, so result is point-at-infinity
    if ((err = mp_sub(modulus, &Q->y, &t1)) != MP_OKAY) { goto done; }
    if (mp_cmp(&P->y, &t1) == MP_EQ) {
      err = ecc_set_point_xyz(0, 0, R);
      goto done;
    }
  }

  // t1 = y2 - y1
  if ((err = mp_submod(&Q->y, &P->y, modulus, &t1)) != MP_OKAY) { goto done; }
  // m = x2 - x1
  if ((err = mp_submod(&Q->x, &P->x, modulus, &m)) != MP_OKAY) { goto done; }
  // m = 1 / (x2 - x1)
  if ((err = mp_invmod(&m, modulus, &m)) != MP_OKAY) { goto done; }
  // m = (y2 - y1) / (x2 - x1)
  if ((err = mp_mulmod(&m, &t1, modulus, &m)) != MP_OKAY) { goto done; }

  // t1 = m^2
  if ((err = mp_sqrmod(&m, modulus, &t1)) != MP_OKAY) { goto done; }
  // t1 = m^2 - x1
  if ((err = mp_submod(&t1, &P->x, modulus, &t1)) != MP_OKAY) { goto done; }
  // x = m^2 - x1 - x2
  if ((err = mp_submod(&t1, &Q->x, modulus, &R->x)) != MP_OKAY) { goto done; }

  // t1 = m * x1
  if ((err = mp_mulmod(&m, &P->x, modulus, &t1)) != MP_OKAY) { goto done; }
  // m = -m
  if ((err = mp_neg(&m, &m)) != MP_OKAY) { goto done; }
  // m = -m * x
  if ((err = mp_mulmod(&m, &R->x, modulus, &m)) != MP_OKAY) { goto done; }
  // m = -m * x + m * x1
  if ((err = mp_addmod(&t1, &m, modulus, &m)) != MP_OKAY) { goto done; }
  // y = -m * x + m * x1 - y1
  if ((err = mp_submod(&m, &P->y, modulus, &R->y)) != MP_OKAY) { goto done; }

  // TODO: assert newy == (-m * newx + m * x2 - y2)

done:
  mp_clear_multi(&t1, &m, NULL);
  return err;
}

static int ecc_point_mul(const mp_int* k, const ecc_point* P, ecc_point* Q, mp_int* modulus) {
  int    err = MP_OKAY;
  mp_int t1;

  if (mp_iszero(k)) {
    return ecc_set_point_xyz(0, 0, Q);
  }

  mp_init(&t1);
  mp_set(&t1, 1);
  if (mp_cmp(k, &t1) == MP_EQ) {
    err = ecc_copy_point(P, Q);
    goto done;
  } else if (mp_iseven(k)) {
    ecc_point* R = ecc_new_point();
    err          = ecc_point_double(P, R, modulus);
    if (err == MP_OKAY) {
      err = mp_div_2(k, &t1);
      if (err == MP_OKAY)
        err = ecc_point_mul(&t1, R, Q, modulus);
    }
    ecc_del_point(R);
  } else {
    ecc_point* R = ecc_new_point();
    err          = ecc_point_double(P, R, modulus);
    if (err == MP_OKAY) {
      err = mp_div_2(k, &t1);
      if (err == MP_OKAY) {
        err = ecc_point_mul(&t1, R, Q, modulus);
        if (err == MP_OKAY)
          err = ecc_point_add(P, Q, Q, modulus);
      }
    }
    ecc_del_point(R);
  }

done:
  mp_clear(&t1);
  return err;
}

int pre_ec_add(evm_t* evm) {
  subgas(500);
  uint8_t cdata[128];
  memset(cdata, 0, 128);
  memcpy(cdata, evm->call_data.data, MIN(128, evm->call_data.len));

  int        err = 0;
  ecc_point *p1, *p2, *p3;
  mp_int     modulus, b;
  p1 = ecc_new_point();
  p2 = ecc_new_point();
  p3 = ecc_new_point();
  if ((err = mp_read_unsigned_bin(&p1->x, cdata, 32)) != MP_OKAY) { goto done; }
  if ((err = mp_read_unsigned_bin(&p1->y, cdata + 32, 32)) != MP_OKAY) { goto done; }
  if ((err = mp_read_unsigned_bin(&p2->x, cdata + 64, 32)) != MP_OKAY) { goto done; }
  if ((err = mp_read_unsigned_bin(&p2->y, cdata + 96, 32)) != MP_OKAY) { goto done; }

  mp_init_multi(&modulus, &b, NULL);
  if ((err = mp_read_unsigned_bin(&modulus, modulus_bin, 32)) != MP_OKAY) { goto done; }
  mp_set(&b, 3);

  evm->return_data = bytes(_calloc(1, 64), 64);

  if (mp_iszero(&p1->x) && mp_iszero(&p1->y) && mp_iszero(&p2->x) && mp_iszero(&p2->y)) {
    err = EVM_ERROR_SUCCESS_CONSUME_GAS;
    goto done;
  } else if (!ecc_point_validate(p1, &modulus, &b) || !ecc_point_validate(p2, &modulus, &b)) {
    err = EVM_ERROR_INVALID_ENV;
    goto done;
  }

  if ((err = ecc_point_add(p1, p2, p3, &modulus)) != MP_OKAY) { goto done; }

  size_t ml = mp_unsigned_bin_size(&p3->x);
  mp_to_unsigned_bin(&p3->x, evm->return_data.data + 32 - ml);
  ml = mp_unsigned_bin_size(&p3->y);
  mp_to_unsigned_bin(&p3->y, evm->return_data.data + 64 - ml);

done:
  ecc_del_point(p1);
  ecc_del_point(p2);
  ecc_del_point(p3);

  mp_clear_multi(&modulus, &b, NULL);
  return err;
}

int pre_ec_mul(evm_t* evm) {
  subgas(40000);
  uint8_t cdata[96];
  memset(cdata, 0, 96);
  memcpy(cdata, evm->call_data.data, MIN(96, evm->call_data.len));

  int        err = 0;
  ecc_point *p1, *p2;
  mp_int     modulus, b, k;
  p1 = ecc_new_point();
  p2 = ecc_new_point();
  if ((err = mp_read_unsigned_bin(&p1->x, cdata, 32)) != MP_OKAY) { goto done; }
  if ((err = mp_read_unsigned_bin(&p1->y, cdata + 32, 32)) != MP_OKAY) { goto done; }

  mp_init_multi(&modulus, &b, &k, NULL);
  if ((err = mp_read_unsigned_bin(&k, cdata + 64, 32)) != MP_OKAY) { goto done; }
  if ((err = mp_read_unsigned_bin(&modulus, modulus_bin, 32)) != MP_OKAY) { goto done; }
  mp_set(&b, 3);

  evm->return_data = bytes(_calloc(1, 64), 64);

  if (mp_iszero(&p1->x) && mp_iszero(&p1->y)) {
    err = EVM_ERROR_SUCCESS_CONSUME_GAS;
    goto done;
  } else if (!ecc_point_validate(p1, &modulus, &b)) {
    err = EVM_ERROR_INVALID_ENV;
    goto done;
  }

  if ((err = ecc_point_mul(&k, p1, p2, &modulus)) != MP_OKAY) { goto done; }

  size_t ml = mp_unsigned_bin_size(&p2->x);
  mp_to_unsigned_bin(&p2->x, evm->return_data.data + 32 - ml);
  ml = mp_unsigned_bin_size(&p2->y);
  mp_to_unsigned_bin(&p2->y, evm->return_data.data + 64 - ml);

done:
  ecc_del_point(p1);
  ecc_del_point(p2);

  mp_clear_multi(&modulus, &b, &k, NULL);
  return err;
}
