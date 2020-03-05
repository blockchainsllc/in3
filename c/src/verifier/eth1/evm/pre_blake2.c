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

#include "../../../core/util/utils.h"
#include "evm.h"
#include "gas.h"
#include "precompiled.h"
#include <stdint.h>
#include <string.h>

static const uint64_t IV[8] = {
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
    0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL};

static const uint8_t SIGMA[12][16] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
    {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
    {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
    {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
    {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
    {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
    {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
    {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
    {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3}};

static uint64_t le_to_long(uint8_t* data) {
  return data[0] | (((uint64_t) data[1]) << 8) | (((uint64_t) data[2]) << 16) | (((uint64_t) data[3]) << 24) | (((uint64_t) data[4]) << 32) | (((uint64_t) data[5]) << 40) | (((uint64_t) data[6]) << 48) | (((uint64_t) data[7]) << 56);
}

static void le_from_long(uint64_t val, uint8_t* data) {
  data[0] = val & 0xFF;
  data[1] = val >> 8 & 0xFF;
  data[2] = val >> 16 & 0xFF;
  data[3] = val >> 24 & 0xFF;
  data[4] = val >> 32 & 0xFF;
  data[5] = val >> 40 & 0xFF;
  data[6] = val >> 48 & 0xFF;
  data[7] = val >> 56 & 0xFF;
}

static inline uint64_t rotr64(const uint64_t w, const unsigned c) {
  return (w >> c) | (w << (64 - c));
}

#define G(r, i, a, b, c, d)             \
  do {                                  \
    a = a + b + m[SIGMA[r][2 * i + 0]]; \
    d = rotr64(d ^ a, 32);              \
    c = c + d;                          \
    b = rotr64(b ^ c, 24);              \
    a = a + b + m[SIGMA[r][2 * i + 1]]; \
    d = rotr64(d ^ a, 16);              \
    c = c + d;                          \
    b = rotr64(b ^ c, 63);              \
  } while (0)

void precompiled_blake2(uint8_t* in, uint8_t* out) {

  uint64_t v[16];
  uint64_t h[8];
  uint64_t m[16];
  uint64_t t_0 = le_to_long(in + 196), t_1 = le_to_long(in + 204);
  uint8_t  f = in[212];
  for (int i = 0; i < 8; i++) h[i] = le_to_long(in + 4 + 8 * i);
  for (int i = 0; i < 16; i++) m[i] = le_to_long(in + 68 + 8 * i);
  int rounds = (int) bytes_to_int(in, 4);

  memcpy(v, h, sizeof(h));
  v[8]  = IV[0];
  v[9]  = IV[1];
  v[10] = IV[2];
  v[11] = IV[3];
  v[12] = IV[4] ^ t_0;
  v[13] = IV[5] ^ t_1;
  v[14] = IV[6] ^ (f ? 0xFFFFFFFFFFFFFFFF : 0);
  v[15] = IV[7];

  for (int r = 0; r < rounds; r++) {
    G(r, 0, v[0], v[4], v[8], v[12]);
    G(r, 1, v[1], v[5], v[9], v[13]);
    G(r, 2, v[2], v[6], v[10], v[14]);
    G(r, 3, v[3], v[7], v[11], v[15]);
    G(r, 4, v[0], v[5], v[10], v[15]);
    G(r, 5, v[1], v[6], v[11], v[12]);
    G(r, 6, v[2], v[7], v[8], v[13]);
    G(r, 7, v[3], v[4], v[9], v[14]);
  }

  for (int i = 0; i < 8; i++)
    le_from_long(h[i] ^ v[i] ^ v[i + 8], out + 8 * i);
}

int pre_blake2(evm_t* evm) {
  if (evm->call_data.len != 213) return -1;
  subgas(bytes_to_int(evm->call_data.data, 4));
  evm->return_data.data = _malloc(128);
  evm->return_data.len  = 128;
  precompiled_blake2(evm->call_data.data, evm->return_data.data);
  return 0;
}
