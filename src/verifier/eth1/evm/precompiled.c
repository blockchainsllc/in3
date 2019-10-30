/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2019 slock.it GmbH, Blockchains LLC
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
#include "../../../third-party/crypto/bignum.h"
#include "../../../third-party/crypto/ecdsa.h"
#include "../../../third-party/crypto/ripemd160.h"
#include "../../../third-party/crypto/secp256k1.h"
#include "../../../third-party/crypto/sha2.h"
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
  mp_int z;
} ecc_point;

static ecc_point* ecc_new_point(void) {
  ecc_point* p;
  p = _calloc(1, sizeof(*p));
  if (p == NULL) {
    return NULL;
  }
  if (mp_init_multi(&p->x, &p->y, &p->z, NULL) != MP_OKAY) {
    _free(p);
    return NULL;
  }
  return p;
}

static void ecc_del_point(ecc_point* p) {
  if (p != NULL) {
    mp_clear_multi(&p->x, &p->y, &p->z, NULL);
    _free(p);
  }
}

static void ecc_set_point_xyz(mp_digit x, mp_digit y, mp_digit z, ecc_point* p) {
  mp_set(&p->x, x);
  mp_set(&p->y, y);
  mp_set(&p->z, z);
}

static int ecc_copy_point(const ecc_point* src, ecc_point* dst) {
  int err;
  if ((err = mp_copy(&src->x, &dst->x)) != MP_OKAY) return err;
  if ((err = mp_copy(&src->y, &dst->y)) != MP_OKAY) return err;
  if ((err = mp_copy(&src->z, &dst->z)) != MP_OKAY) return err;
  return MP_OKAY;
}

static int ecc_is_point_at_infinity(const ecc_point* P, void* modulus, int* retval) {
  int    err;
  mp_int x3, y2;

  /* trivial case */
  if (!mp_iszero(&P->z)) {
    *retval = 0;
    return MP_OKAY;
  }

  /* point (0,0,0) is not at infinity */
  if (mp_iszero(&P->x) && mp_iszero(&P->y)) {
    *retval = 0;
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
    /* if P is point at infinity >> Result = point at infinity */
    ecc_set_point_xyz(1, 1, 0, R);
    err = MP_OKAY;
    goto done;
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
  // MP_PRINT(R->x);

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
  // MP_PRINT(R->y);

  err = MP_OKAY;
  done:
  mp_clear_multi(&t4, &t3, &t2, &t1, NULL);
  return err;
}

uint8_t evm_is_precompiled(evm_t* evm, address_t address) {
  UNUSED_VAR(evm);
  int l = 20;
  optimize_len(address, l);
  return (l == 1 && *address && *address < 9);
}

int pre_ecrecover(evm_t* evm) {
  subgas(G_PRE_EC_RECOVER);
  if (evm->call_data.len < 128) return 0;

  uint8_t pubkey[65], *vdata = evm->call_data.data + 32, vl = 32;
  optimize_len(vdata, vl);
  if (vl > 1) return 0;

  // verify signature
  if (ecdsa_recover_pub_from_sig(&secp256k1, pubkey, evm->call_data.data + 64, evm->call_data.data, *vdata >= 27 ? *vdata - 27 : *vdata) == 0) {
    evm->return_data.data = _malloc(20);
    evm->return_data.len  = 20;

    uint8_t hash[32];

    // hash it and return the last 20 bytes as address
    bytes_t public_key = {.data = pubkey + 1, .len = 64};
    if (sha3_to(&public_key, hash) == 0)
      memcpy(evm->return_data.data, hash + 12, 20);
  }
  return 0;
}

int pre_sha256(evm_t* evm) {
  subgas(G_PRE_SHA256 + (evm->call_data.len + 31) / 32 * G_PRE_SHA256_WORD);
  evm->return_data.data = _malloc(32);
  evm->return_data.len  = 32;
  SHA256_CTX ctx;
  sha256_Init(&ctx);
  sha256_Update(&ctx, evm->call_data.data, evm->call_data.len);
  sha256_Final(&ctx, evm->return_data.data);
  return 0;
}
int pre_ripemd160(evm_t* evm) {
  subgas(G_PRE_RIPEMD160 + (evm->call_data.len + 31) / 32 * G_PRE_RIPEMD160_WORD);
  evm->return_data.data = _malloc(20);
  evm->return_data.len  = 20;
  ripemd160(evm->call_data.data, evm->call_data.len, evm->return_data.data);
  return 0;
}
int pre_identity(evm_t* evm) {
  subgas(G_PRE_IDENTITY + (evm->call_data.len + 31) / 32 * G_PRE_IDENTITY_WORD);
  evm->return_data.data = _malloc(evm->call_data.len);
  evm->return_data.len  = evm->call_data.len;
  memcpy(evm->return_data.data, evm->call_data.data, evm->call_data.len);
  return 0;
}

int pre_modexp(evm_t* evm) {
  if (evm->call_data.len < 96) return -1;
  uint8_t      res[64];
  uint_fast8_t hp     = 0;
  uint32_t     l_base = bytes_to_int(evm->call_data.data + 28, 4);
  uint32_t     l_exp  = bytes_to_int(evm->call_data.data + 28 + 32, 4);
  uint32_t     l_mod  = bytes_to_int(evm->call_data.data + 28 + 64, 4);
  if (evm->call_data.len < 96 + l_base + l_exp + l_mod) return -1;
  bytes_t b_base = bytes(evm->call_data.data + 96, l_base);
  bytes_t b_exp  = bytes(evm->call_data.data + 96 + l_base, l_exp);
  bytes_t b_mod  = bytes(evm->call_data.data + 96 + l_base + l_exp, l_mod);

#ifdef EVM_GAS

  for (uint32_t i = 0; i < MIN(l_exp, 32); i++) {
    if (b_exp.data[i]) {
      for (int n = 7; n >= 0; n--) {
        if (b_exp.data[i] >> n) {
          hp = ((l_exp - i - 1) << 3) + n;
          break;
        }
      }
      break;
    }
  }
  uint64_t ael;
  if (l_exp <= 32 && hp == 0)
    ael = 0;
  else if (l_exp <= 32)
    ael = hp;
  else
    ael = 8 * (l_exp - 32) + hp;

  // calc gas
  //  floor(mult_complexity(max(length_of_MODULUS, length_of_BASE)) * max(ADJUSTED_EXPONENT_LENGTH, 1) / GQUADDIVISOR)
  uint64_t lm = MAX(l_mod, l_base);
  if (lm <= 64)
    lm *= lm;
  else if (lm <= 1024)
    lm = lm * lm / 4 + 96 * lm - 3072;
  else
    lm = lm * lm / 16 + 480 * lm - 199680;

  subgas(lm * MAX(1, ael) / G_PRE_MODEXP_GQUAD_DIVISOR);

#endif
  // we use gmp for now
  mp_int m_base, m_exp, m_mod, m_res;
  mp_init(&m_base);
  mp_init(&m_exp);
  mp_init(&m_mod);
  mp_init(&m_res);

  mp_import(&m_base, b_base.len, 1, sizeof(uint8_t), 1, 0, b_base.data);
  mp_import(&m_exp, b_exp.len, 1, sizeof(uint8_t), 1, 0, b_exp.data);
  mp_import(&m_mod, b_mod.len, 1, sizeof(uint8_t), 1, 0, b_mod.data);

  m_base.sign = m_exp.sign = m_mod.sign = 0;
  mp_exptmod(&m_base, &m_exp, &m_mod, &m_res);
  size_t ml;
  mp_export(res, &ml, 1, sizeof(uint8_t), 1, 0, &m_res);

  mp_clear(&m_base);
  mp_clear(&m_exp);
  mp_clear(&m_mod);
  mp_clear(&m_res);

  evm->return_data.data = _malloc(ml);
  evm->return_data.len  = ml;
  memcpy(evm->return_data.data, res, ml);
  return 0;
}

int pre_ec_add(evm_t* evm) {
  subgas(500);
  uint8_t cdata[128];
  memset(cdata, 0, 128);
  memcpy(cdata, evm->call_data.data, MIN(128, evm->call_data.len));

  int        err = 0;
  ecc_point *p1, *p2, *p3;
  mp_int     modulus;
  p1 = ecc_new_point();
  p2 = ecc_new_point();
  p3 = ecc_new_point();
  if ((err = mp_read_unsigned_bin(&p1->x, cdata, 32)) != MP_OKAY) { return EVM_ERROR_INVALID_ENV; }
  if ((err = mp_read_unsigned_bin(&p1->y, cdata + 32, 32)) != MP_OKAY) { return EVM_ERROR_INVALID_ENV; }
  if ((err = mp_read_unsigned_bin(&p2->x, cdata + 64, 32)) != MP_OKAY) { return EVM_ERROR_INVALID_ENV; }
  if ((err = mp_read_unsigned_bin(&p2->y, cdata + 96, 32)) != MP_OKAY) { return EVM_ERROR_INVALID_ENV; }

  mp_init(&modulus);
  if ((err = mp_read_unsigned_bin(&modulus, modulus_bin, 32)) != MP_OKAY) { return EVM_ERROR_INVALID_ENV; }

  ecc_point_double(p1, p3, &modulus);

  evm->return_data = bytes(_malloc(64), 64);
  mp_to_unsigned_bin(&p3->x, evm->return_data.data);
  mp_to_unsigned_bin(&p3->y, evm->return_data.data + 32);
  ecc_del_point(p1);
  ecc_del_point(p2);
  ecc_del_point(p3);

  in3_log_set_level(LOG_TRACE);
  b_print(&evm->return_data);
  in3_log_set_level(LOG_ERROR);

  mp_clear_multi(&modulus, NULL);
  return 0;
}

int pre_ec_mul(evm_t* evm) {
  subgas(40000);
  curve_point a, b;
  bignum256   s;
  uint8_t     cdata[96];
  memset(cdata, 0, 96);
  memcpy(cdata, evm->call_data.data, MIN(96, evm->call_data.len));
  
  evm->return_data = bytes(_malloc(64), 64);
  bn_write_be(&b.x, evm->return_data.data);
  bn_write_be(&b.y, evm->return_data.data + 32);
  return 0;
}

int evm_run_precompiled(evm_t* evm, address_t address) {
  switch (address[19]) {
    case 1:
      return pre_ecrecover(evm);
    case 2:
      return pre_sha256(evm);
    case 3:
      return pre_ripemd160(evm);
    case 4:
      return pre_identity(evm);
    case 5:
      return pre_modexp(evm);
    case 6:
      return pre_ec_add(evm);
    case 7:
      return pre_ec_mul(evm);
    default:
      return -1;
  }
}
