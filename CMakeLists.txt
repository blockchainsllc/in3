cmake_minimum_required(VERSION 3.7.2)

# project name
project(in3-core)

# configure a header file to pass some of the CMake settings
# to the source code
# The version number.
set (IN3_VERSION_MAJOR 1)
set (IN3_VERSION_MINOR 0)
configure_file (
  "${PROJECT_SOURCE_DIR}/src/core/util/config.h.in"
  "${PROJECT_BINARY_DIR}/in3Config.h"
  )
 
# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("${PROJECT_BINARY_DIR}")


# options
OPTION(DEBUG
  "Debug"
  OFF)
IF(DEBUG)
  ADD_DEFINITIONS(-DDEBUG)
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF(DEBUG)

# subst
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__='\"$(subst
  ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")


# add the executable
add_executable(in3-core src/main.c)

target_sources(in3-core PRIVATE
	src/core/client/context.c
	src/core/client/client.c
	src/core/client/client_init.c
	src/core/util/debug.c
	src/core/util/bytes.c
	src/core/util/utils.c
	src/core/jsmn/jsmn.c
	src/core/jsmn/jsmnutil.c
	src/core/crypto/ecdsa.c
	src/core/crypto/address.c
	src/core/crypto/bignum.c
	src/core/crypto/rand.c
	src/core/crypto/hmac.c
	src/core/crypto/secp256k1.c
	src/core/crypto/memzero.c
	src/core/crypto/sha3.c
	src/core/crypto/ripemd160.c
	src/core/crypto/base58.c
	src/core/crypto/hasher.c
	src/core/crypto/sha2.c
	src/core/crypto/blake256.c
	src/core/crypto/blake2b.c
	src/core/crypto/groestl.c
)

set_property(TARGET in3-core PROPERTY C_STANDARD 99)
