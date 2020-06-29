###############################################################################
# This file is part of the Incubed project.
# Sources: https://github.com/slockit/in3-c
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

cmake_minimum_required(VERSION 3.5.1)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/scripts/cmake_modules/")

# project name
project(in3)

# options
option(BUILD_DOC "generates the documenation with doxygen." OFF)
option(TAG_VERSION "the tagged version, which should be used" 2.0.0)
option(ETH_NANO "build minimal eth verification.(eth_getTransactionReceipt)" ON)
option(ETH_BASIC "build basic eth verification.(all rpc-calls except eth_call)" ON)
option(ETH_FULL "build full eth verification.(including eth_call)" ON)
option(IPFS "build IPFS verification" ON)
option(COLOR "Enable color codes for debug" ON)
option(BTC "if true, the bitcoin verifiers will be build" ON)
option(IN3API "build the USN-API which offer better interfaces and additional functions on top of the pure verification" ON)
option(USE_PRECOMPUTED_EC "if true the secp256k1 curve uses precompiled tables to boost performance. turning this off makes ecrecover slower, but saves about 37kb." ON)
option(ERR_MSG "if set human readable error messages will be inculded in th executable, otherwise only the error code is used. (saves about 19kB)" ON)
option(EVM_GAS "if true the gas costs are verified when validating a eth_call. This is a optimization since most calls are only interessted in the result. EVM_GAS would be required if the contract uses gas-dependend op-codes." true)
option(IN3_LIB "if true a shared anmd static library with all in3-modules will be build." ON)
option(TEST "builds the tests and also adds special memory-management, which detects memory leaks, but will cause slower performance" OFF)
option(FAST_MATH "Math optimizations used in the EVM. This will also increase the filesize." OFF)
option(SEGGER_RTT "Use the segger real time transfer terminal as the logging mechanism" OFF)
option(CURL_BLOCKING "if true the curl-request will block until the response is received" OFF)
option(JAVA "build the java-binding (shared-lib and jar-file)" OFF)
option(JAVA_MULTI_LIBS "embedds multiple shared libs in the jar" OFF)
option(WASM "Includes the WASM-Build. In order to build it you need emscripten as toolchain. Usually you also want to turn off other builds in this case." OFF)
option(ASMJS "compiles the code as asm.js." OFF)
option(WASM_EMBED "embedds the wasm as base64-encoded into the js-file" ON)
option(WASM_EMMALLOC "use ther smaller EMSCRIPTEN Malloc, which reduces the size about 10k, but may be a bit slower" ON)
option(WASM_SYNC "intiaializes the WASM synchronisly, which allows to require and use it the same function, but this will not be supported by chrome (4k limit)" OFF)
option(CODE_COVERAGE  "Builds targets with code coverage instrumentation. (Requires GCC or Clang)"  OFF)
option(GCC_ANALYZER "GCC10 static code analyses" OFF)
option(PAY_ETH  "support for direct Eth-Payment"  OFF)
option(USE_SCRYPT "integrate scrypt into the build in order to allow decrypt_key for scrypt encoded keys." ON)
option(USE_CURL "if true the curl transport will be built (with a dependency to libcurl)" ON)
option(DEV_NO_INTRN_PTR "(*dev option*) if true the client will NOT include a void pointer (named internal) for use by devs)" ON)
option(LEDGER_NANO "include support for nano ledger" ON)
option(ESP_IDF "include support for ESP-IDF microcontroller framework" OFF)
option(ASSERTIONS "includes assertions into the code, which help track errors but may cost time during runtime" OFF)
OPTION(TRANSPORTS "builds transports, which may require extra libraries." ON)
OPTION(IN3_SERVER "support for proxy server as part of the cmd-tool, which allows to start the cmd-tool with the -p option and listens to the given port for rpc-requests" OFF)
OPTION(CMD "build the comandline utils" ON)
OPTION(POA "support POA verification including validatorlist updates" OFF)

IF (POA)
  ADD_DEFINITIONS(-DPOA)
ENDIF (POA)

if (USE_PRECOMPUTED_EC)
  ADD_DEFINITIONS(-DUSE_PRECOMPUTED_CP=1)
else()
  ADD_DEFINITIONS(-DUSE_PRECOMPUTED_CP=0)
endif()

if (ERR_MSG)
  ADD_DEFINITIONS(-DERR_MSG)
endif()

if(ETH_FULL)
    ADD_DEFINITIONS(-DETH_FULL)
    set(IN3_VERIFIER eth_full)
    set(ETH_BASIC true)
    set(ETH_NANO true)
elseif(ETH_BASIC)
    ADD_DEFINITIONS(-DETH_BASIC)
    set(IN3_VERIFIER eth_basic)
    set(ETH_NANO true)
elseif(ETH_NANO)
    ADD_DEFINITIONS(-DETH_NANO)
    set(IN3_VERIFIER eth_nano)
endif()

if(IN3API)
    ADD_DEFINITIONS(-DETH_API)
    set(IN3_API eth_api)
endif()

if (ESP_IDF)
    ADD_DEFINITIONS(-DESP_IDF)
endif()

if(PAY_ETH)
  ADD_DEFINITIONS(-DPAY_ETH -DPAY)
  set(IN3_API ${IN3_API} pay_eth)
endif()

if(IPFS)
    ADD_DEFINITIONS(-DIPFS)
    set(IN3_VERIFIER ${IN3_VERIFIER} ipfs)
    if(IN3API)
       set(IN3_API ${IN3_API} ipfs_api)
    endif()
endif()

if(BTC)
    ADD_DEFINITIONS(-DBTC)
    set(IN3_VERIFIER ${IN3_VERIFIER} btc)
    if(IN3API)
       set(IN3_API ${IN3_API} btc_api)
    endif()
endif()

if(LEDGER_NANO)
    add_definitions(-DLEDGER_NANO)
endif()

if(COLOR AND NOT (MSVC OR MSYS OR MINGW))
   ADD_DEFINITIONS(-DLOG_USE_COLOR)
endif()


if(CMAKE_BUILD_TYPE MATCHES Debug)
    ADD_DEFINITIONS(-DDEBUG)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

if(EVM_GAS)
    ADD_DEFINITIONS(-DEVM_GAS)
endif(EVM_GAS)

if(FAST_MATH)
    ADD_DEFINITIONS(-DIN3_MATH_FAST)
else()
    ADD_DEFINITIONS(-DIN3_MATH_LITE)
endif(FAST_MATH)

if(SEGGER_RTT)
    ADD_DEFINITIONS(-DSEGGER_RTT)
endif(SEGGER_RTT)

if (DEV_NO_INTRN_PTR)
    ADD_DEFINITIONS(-DDEV_NO_INTRN_PTR)
endif()

# handle version
if (TAG_VERSION)
   set(PROJECT_VERSION "${TAG_VERSION}")
else(TAG_VERSION)
   set(PROJECT_VERSION "2.2.2-local")
endif(TAG_VERSION)

MESSAGE(STATUS "Building version ${PROJECT_VERSION}")

string(REPLACE "." ";" VERSION_LIST ${PROJECT_VERSION})
list(GET VERSION_LIST 0 PROJECT_VERSION_MAJOR)
list(GET VERSION_LIST 1 PROJECT_VERSION_MINOR)
list(GET VERSION_LIST 2 PROJECT_VERSION_PATCH)

ADD_DEFINITIONS("-DIN3_VERSION=\"${PROJECT_VERSION}\"")
ADD_DEFINITIONS(-DIN3_VERSION_MAJOR=${PROJECT_VERSION_MINOR})
ADD_DEFINITIONS(-DIN3_VERSION_MINOR=${PROJECT_VERSION_MINOR})
ADD_DEFINITIONS(-DIN3_VERSION_PATCH=${PROJECT_VERSION_PATCH})


# define output dir structure
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


IF (WASM)
  set(TEST false)
  set(TRANSPORTS false)
  set(BUILD_DOC false)
  set(IN3_LIB false)
  set(CMD false)
  set(USE_CURL false)
  add_subdirectory(wasm/src)
ENDIF (WASM)


# build tests
if(TEST)
    ADD_DEFINITIONS(-DTEST)
    ADD_DEFINITIONS(-DIN3_DONT_HASH_KEYS)
    ADD_DEFINITIONS(-DIN3_EXPORT_TEST=)
    ADD_DEFINITIONS(-DIN3_IMPORT_TEST=extern)
    ADD_DEFINITIONS(-DDEBUG)
    SET(CMAKE_BUILD_TYPE Debug)
    enable_testing()
    add_subdirectory(c/test)
    add_custom_target(ptest COMMAND ${CMAKE_CTEST_COMMAND} -j 8)
    add_custom_target(rtest COMMAND ${CMAKE_CTEST_COMMAND} -V )
endif(TEST)


add_subdirectory(c)

IF (JAVA)
   add_subdirectory(java)
ENDIF (JAVA)
