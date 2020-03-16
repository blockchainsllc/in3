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

#include "precompiled.h"
#include "../../../core/util/mem.h"
#include "../../../core/util/utils.h"
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

uint8_t evm_is_precompiled(evm_t* evm, address_t address) {
  UNUSED_VAR(evm);
  int l = 20;
  optimize_len(address, l);
  return (l == 1 && *address && *address < 10);
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

#else
  UNUSED_VAR(hp);

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

int evm_run_precompiled(evm_t* evm, const address_t address) {
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
    case 9:
      return pre_blake2(evm);
    default:
      return -1;
  }
}
