cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
include(../../../cmake/macro.cmake)

project(z_in3)

target_sources(app PRIVATE src/main.c)

ADD_DEFINITIONS(-DTEST)
ADD_DEFINITIONS(-DDEBUG)
ADD_DEFINITIONS(-DCRYPTO_TREZOR)
ADD_DEFINITIONS(-DNODESELECT_DEF)
ADD_DEFINITIONS(-DNO_BOOT_NODES_MAINNET)
ADD_DEFINITIONS(-DNO_BOOT_NODES_IPFS)
ADD_DEFINITIONS(-DNO_BOOT_NODES_BTC)
ADD_DEFINITIONS(-DNO_BOOT_NODES_EWC)
ADD_DEFINITIONS(-DUSE_SCRYPT=false)
ADD_DEFINITIONS(-DIN3_EXPORT_TEST=static)
ADD_DEFINITIONS(-DIN3_IMPORT_TEST=)
SET(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_VERBOSE_MAKEFILE ON)
SET(CRYPTO_TREZOR true)
ADD_DEFINITIONS(-DZEPHYR_OS)
ADD_DEFINITIONS(-DZEPHYR)
ADD_DEFINITIONS(-DIN3_MATH_LITE)

set(IN3_SRC ../../..)
set(IN3_DEPS
  ${IN3_SRC}/src/core
  ${IN3_SRC}/src/verifier/eth1/basic
  ${IN3_SRC}/src/verifier/eth1/nano
  ${IN3_SRC}/src/signer/pk-signer
  ${IN3_SRC}/src/api/eth1
  ${IN3_SRC}/src/api/utils
  ${IN3_SRC}/src/third-party/crypto
  ${IN3_SRC}/src/third-party/tommath
  ${IN3_SRC}/src/nodeselect/full
)

foreach(dep ${IN3_DEPS})
  file(GLOB files "${dep}/**/*.c")
  file(GLOB files2 "${dep}/*.c")

  # temp fix for building with a GLOB and not include all 3rd party libs
  list(FILTER files EXCLUDE REGEX ".*aestst.c$")
  list(FILTER files EXCLUDE REGEX ".*crypto_none.c$")
  list(FILTER files EXCLUDE REGEX ".*crypto_openssl.c$")
  target_sources(app PRIVATE "${files}" "${files2}")
  include_directories("${dep}")
endforeach()

target_include_directories(app PRIVATE "../include")
target_include_directories(app PRIVATE "$ENV{ZEPHYR_BASE}/include")
