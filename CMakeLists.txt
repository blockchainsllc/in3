###############################################################################
# This file is part of the Incubed project.
# Sources: https://github.com/slockit/in3-c
# 
# Copyright (C) 2018-2019 slock.it GmbH, Blockchains LLC
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

cmake_minimum_required(VERSION 3.6.1)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/scripts/cmake_modules/")
# project name
project(in3)
OPTION(TAG_VERSION "the tagged version, which should be used" 2.0.0)
OPTION(ETH_NANO "build minimal eth verification.(eth_getTransactionReceipt)" ON)
OPTION(ETH_BASIC "build basic eth verification.(all rpc-calls except eth_call)" ON)
OPTION(ETH_FULL "build full eth verification.(including eth_call)" ON)
OPTION(IN3API "build the USN-API which offer better interfaces and additional functions on top of the pure verification" ON)
OPTION(USE_PRECOMPUTED_EC "if true the secp256k1 curve uses precompiled tables to boost performance. turning this off makes ecrecover slower, but saves about 37kb." ON)
OPTION(ERR_MSG "if set human readable error messages will be inculded in th executable, otherwise only the error code is used. (saves about 19kB)" ON)
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
elseif(ETH_BASIC) 
  ADD_DEFINITIONS(-DETH_BASIC)
  set(IN3_VERIFIER eth_basic)
elseif(ETH_NANO) 
  ADD_DEFINITIONS(-DETH_NANO)
  set(IN3_VERIFIER eth_nano)
endif()
if(IN3API) 
  ADD_DEFINITIONS(-DETH_API)
  set(IN3_API eth_api)
endif()

set(PROJECT_VERSION "0.1.0-alpha.0")
if (TAG_VERSION)
   set(PROJECT_VERSION "${TAG_VERSION}")
ENDIF(TAG_VERSION)
MESSAGE(STATUS "Building version ${PROJECT_VERSION}")

string(REPLACE "." ";" VERSION_LIST ${PROJECT_VERSION})
list(GET VERSION_LIST 0 PROJECT_VERSION_MAJOR)
list(GET VERSION_LIST 1 PROJECT_VERSION_MINOR)
list(GET VERSION_LIST 2 PROJECT_VERSION_PATCH)

ADD_DEFINITIONS("-DIN3_VERSION=\"${PROJECT_VERSION}\"")
ADD_DEFINITIONS(-DIN3_VERSION_MAJOR=${PROJECT_VERSION_MINOR})
ADD_DEFINITIONS(-DIN3_VERSION_MINOR=${PROJECT_VERSION_MINOR})
ADD_DEFINITIONS(-DIN3_VERSION_PATCH=${PROJECT_VERSION_PATCH})


IF (CMAKE_BUILD_TYPE MATCHES Debug)
    ADD_DEFINITIONS(-DDEBUG)
ENDIF (CMAKE_BUILD_TYPE MATCHES Debug)

OPTION(EVM_GAS "if true the gas costs are verified when validating a eth_call. This is a optimization since most calls are only interessted in the result. EVM_GAS would be required if the contract uses gas-dependend op-codes." true)
IF (EVM_GAS)
    MESSAGE(STATUS "Enable GAS in EVM")
    ADD_DEFINITIONS(-DEVM_GAS)
ENDIF (EVM_GAS)

OPTION(IN3_LIB "if true a shared anmd static library with all in3-modules will be build." ON)

OPTION(TEST "builds the tests and also adds special memory-management, which detects memory leaks, but will cause slower performance" OFF)
IF (TEST)
    MESSAGE(STATUS "Build Tests and add debug infos")
    ADD_DEFINITIONS(-DTEST)
    ADD_DEFINITIONS(-DLOG_USE_COLOR)
    ADD_DEFINITIONS(-DIN3_DONT_HASH_KEYS)
    ADD_DEFINITIONS(-DIN3_EXPORT_TEST=)
    ADD_DEFINITIONS(-DIN3_IMPORT_TEST=extern)
    SET(CMAKE_BUILD_TYPE Debug)
    enable_testing()
    add_subdirectory(test)
    add_custom_target(ptest COMMAND ${CMAKE_CTEST_COMMAND} -j 16)
ELSE ()
    ADD_DEFINITIONS(-DIN3_EXPORT_TEST=static)
ENDIF (TEST)

OPTION(FAST_MATH "Math optimizations used in the EVM. This will also increase the filesize." OFF)
IF (FAST_MATH)
    MESSAGE(STATUS "Enable math optimizations (excutable size may increase)")
    ADD_DEFINITIONS(-DIN3_MATH_FAST)
ELSE ()
    MESSAGE(STATUS "Disable math optimizations (optimised for executable size)")
    ADD_DEFINITIONS(-DIN3_MATH_LITE)
ENDIF (FAST_MATH)

OPTION(SEGGER_RTT "Use the segger real time transfer terminal as the logging mechanism" OFF)
IF (SEGGER_RTT)
    MESSAGE(STATUS "Enable segger RTT for logging")
    ADD_DEFINITIONS(-DSEGGER_RTT)
ENDIF (SEGGER_RTT)

OPTION(FILTER_NODES "if true the nodelist is filtered against config node properties" false)
IF (FILTER_NODES)
    MESSAGE(STATUS "Enable nodelist filter based on properties")
    ADD_DEFINITIONS(-DFILTER_NODES)
ENDIF (FILTER_NODES)

# substiture __FILENAME__ as it is used in DEBUG
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include("${PROJECT_SOURCE_DIR}/compiler.cmake")

#  build modules
add_subdirectory(src/third-party)
add_subdirectory(src/core)
add_subdirectory(src/transport)
add_subdirectory(src/verifier)
add_subdirectory(src/bindings)
add_subdirectory(src/api)
add_subdirectory(src/cmd)
add_subdirectory(docs)


# create the library
if (IN3_LIB)
    set(IN3_LIBS
        $<TARGET_OBJECTS:core_o>
        $<TARGET_OBJECTS:crypto_o>
        $<TARGET_OBJECTS:tommath_o>
        $<TARGET_OBJECTS:evm_o>
        $<TARGET_OBJECTS:eth_full_o>
        $<TARGET_OBJECTS:eth_api_o>
        $<TARGET_OBJECTS:eth_basic_o>
        $<TARGET_OBJECTS:eth_nano_o>
        $<TARGET_OBJECTS:usn_api_o>
    )
    # if we use curl, we include curl transport as well
    if (USE_CURL)
        set(IN3_LIBS ${IN3_LIBS} $<TARGET_OBJECTS:transport_curl_o>)
    endif()

    # create the libraries
    add_library(in3_bundle STATIC ${IN3_LIBS} )
    add_library(in3_lib    SHARED ${IN3_LIBS} )
    set_target_properties(in3_bundle PROPERTIES OUTPUT_NAME "in3")
    set_target_properties(in3_lib PROPERTIES OUTPUT_NAME "in3")

    # for curl we need to add the dependency
    if (USE_CURL)
       target_link_libraries(in3_lib transport_curl)
    endif()

    # install
    INSTALL(TARGETS in3_bundle
      DESTINATION "lib"
    )
    INSTALL(TARGETS in3_lib
        DESTINATION lib
        PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE)
    INSTALL (
        DIRECTORY ${CMAKE_SOURCE_DIR}/include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h*")

endif()
