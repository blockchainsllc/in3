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
# main incubed module defining the interfaces for transport, verifier and storage. 
#
# This module does not have any dependencies and cannot be used without additional modules providing verification and transport.
OPTION(IN3_STAGING "if true, the client will use the staging-network instead of the live ones" OFF)
IF (IN3_STAGING)
    ADD_DEFINITIONS(-DIN3_STAGING)
ENDIF (IN3_STAGING)


if(CRYPTO_TREZOR)
  set(CRYPTO_SRC util/crypto_trezor.c)
  set(DEPS crypto)
elseif(CRYPTO_OPENSSL)
  set(CRYPTO_SRC util/crypto_openssl.c)
  set(DEPS crypto_ssl)
  include(ExternalProject)
  ExternalProject_Add(openssl
    GIT_REPOSITORY    https://github.com/openssl/openssl.git
    GIT_TAG           master
    GIT_PROGRESS      true
    BUILD_IN_SOURCE   true
    USES_TERMINAL_CONFIGURE true
    USES_TERMINAL_BUILD true
    SOURCE_DIR        "${CMAKE_BINARY_DIR}/openssl"
    CONFIGURE_COMMAND eval "test -f Makefile || ./Configure --debug --prefix=${CMAKE_INSTALL_PREFIX} no-uplink no-ui-console no-ts no-threads no-tests no-ssl-trace  no-sse2 no-srtp no-srp no-sock no-rdrand no-psk no-padlockeng no-ocsp  no-nextprotoneg no-multiblock no-makedepend no-legacy no-gost no-dso no-dgram no-deprecated no-ct no-comp no-cms no-cmp no-capieng no-buildtest-c++  no-autoalginit no-async no-asm no-afalgeng no-ktls no-asan "
    BUILD_COMMAND     make
    INSTALL_COMMAND   ""
  )

  include_directories( ${CMAKE_BINARY_DIR}/openssl/include )
  add_library(crypto_ssl STATIC IMPORTED GLOBAL)
  set_target_properties(crypto_ssl PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/openssl/libcrypto.a)
  add_dependencies(crypto_ssl openssl)
else()
  set(CRYPTO_SRC util/crypto_none.c)
endif()

if (BASE64)
  set(DEPS ${DEPS} b64)
endif()

if(ETH_FULL AND IN3API)
  set(DEPS ${DEPS} tommath)
endif()

if (ZEPHYR_BASE)
  add_library(core STATIC client/request.c client/client.c client/execute.c client/client_init.c util/debug.c util/bytes.c util/utils.c util/scache.c util/data.c util/log.c util/mem.c util/stringbuilder.c util/bitset.c util/crypto_encode.c ${CRYPTO_SRC})
  add_dependencies(core zephyr_interface ${DEPS})
else()
  add_static_library(
    NAME     core 
    DESCR    "The core of SDK and Incubed, which defines the plugin-concept and handles the request execution life cycle."
  
    SOURCES 
      client/request.c
      client/client.c
      client/execute.c
      client/client_init.c
      util/debug.c
      util/bytes.c
      util/utils.c
      util/scache.c
      util/data.c
      util/log.c
      util/mem.c
      util/stringbuilder.c
      util/bitset.c
      util/crypto_encode.c
      ${CRYPTO_SRC}
    DEPENDS
      ${DEPS}    
  )
endif()