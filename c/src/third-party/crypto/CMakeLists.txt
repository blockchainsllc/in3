###############################################################################
# This file is part of the Incubed project.
# Sources: https://github.com/blockchainsllc/in3
# 
# Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
# 
# 
# COMMERCIAL LICENSE USAGE
# 
# Licensees holding a valid commercial license may use this file in accordance 
# with the commercial license agreement provided with the Software or, alternatively, 
# in accordance with the terms contained in a written agreement between you and 
# slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
# information please contact slock.it at in3@slock.it.
# 	
# Alternatively, this file may be used under the AGPL license as follows:
#    
# AGPL LICENSE USAGE
# 
# This program is free software: you can redistribute it and/or modify it under the
# terms of the GNU Affero General Public License as published by the Free Software 
# Foundation, either version 3 of the License, or (at your option) any later version.
#  
# This program is distributed in the hope that it will be useful, but WITHOUT ANY 
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
# PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
# [Permissions of this strong copyleft license are conditioned on making available 
# complete source code of licensed works and modifications, which include larger 
# works using a licensed work, under the same license. Copyright and license notices 
# must be preserved. Contributors provide an express grant of patent rights.]
# You should have received a copy of the GNU Affero General Public License along 
# with this program. If not, see <https://www.gnu.org/licenses/>.
###############################################################################

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-null-pointer-subtraction")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-null-pointer-subtraction")

set(CRYPTO_SRC 
  ecdsa.c
  address.c
  bignum.c
  rand.c
  secp256k1.c
  memzero.c
  sha3.c
  hmac_drbg.c
  ripemd160.c
  base58.c
  hasher.c
  sha2.c
  blake256.c
  blake2b.c
  groestl.c
  pbkdf2.c
  rfc6979.c
  aes/aes_modes.c
  aes/aescrypt.c
  aes/aeskey.c
  aes/aestab.c

  bip39.c

  curves.c
  bip32.c
  nist256p1.c

)

if (ED25519) 
  set(CRYPTO_SRC ${CRYPTO_SRC} 
    ed25519-donna/curve25519-donna-32bit.c
    ed25519-donna/curve25519-donna-helpers.c
    ed25519-donna/curve25519-donna-scalarmult-base.c
    ed25519-donna/ed25519-donna-32bit-tables.c
    ed25519-donna/ed25519-donna-basepoint-table.c
    ed25519-donna/ed25519-donna-impl-base.c
    ed25519-donna/ed25519-keccak.c
    ed25519-donna/ed25519-sha3.c
    ed25519-donna/ed25519.c
    ed25519-donna/modm-donna-32bit.c
  )
 ADD_DEFINITIONS(-DUSE_BIP32_25519_CURVES=1)
else()
 ADD_DEFINITIONS(-DUSE_BIP32_25519_CURVES=0)
endif()


if (NOT ESP_IDF)
    set(CRYPTO_SRC ${CRYPTO_SRC} hmac.c)
endif()

add_static_library(
  NAME     crypto 
  
  SOURCES 
    ${CRYPTO_SRC}
)



