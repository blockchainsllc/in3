cmake_minimum_required(VERSION 3.7.2)

# project name
project(in3)

# options
OPTION(WASM "Wasm" OFF)
OPTION(DEBUG "Debug" OFF)
IF(DEBUG)
  ADD_DEFINITIONS(-DDEBUG)
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF(DEBUG)

OPTION(EVM_GAS "Evm Gas" ON)
IF(EVM_GAS)
  MESSAGE ( STATUS "Enable GAS in EVM" )
  ADD_DEFINITIONS(-DEVM_GAS)
ENDIF(EVM_GAS)

OPTION(TEST "Test" OFF)
IF(TEST)
  MESSAGE ( STATUS "Build Tests and add debug infos" )
  ADD_DEFINITIONS(-DTEST)
  ADD_DEFINITIONS(-DLOG_USE_COLOR)
  SET(CMAKE_BUILD_TYPE Debug)
  enable_testing()
  add_subdirectory( test )
  add_custom_target(ptest COMMAND ${CMAKE_CTEST_COMMAND} -j 16)
ENDIF(TEST)

OPTION(FAST_MATH "Math optimizations" OFF)
IF(FAST_MATH)
  MESSAGE ( STATUS "Enable math optimizations (excutable size may increase)" )
  ADD_DEFINITIONS(-DIN3_MATH_FAST)
ELSE()
  MESSAGE ( STATUS "Disable math optimizations (optimised for executable size)" )
  ADD_DEFINITIONS(-DIN3_MATH_LITE)
ENDIF(FAST_MATH)

# substiture __FILENAME__ as it is used in DEBUG
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

#  build modules
add_subdirectory( src/core )
add_subdirectory( src/third-party )
add_subdirectory( src/transport )
#add_subdirectory( src/verifier )
#add_subdirectory( src/bindings )
#IF(WASM)
#  add_subdirectory( src/bindings/wasm )
#ELSE(WASM)
#  add_subdirectory( src/api/eth1 )
#  add_subdirectory( src/api/usn )
#  add_subdirectory( src/transport )
#  add_subdirectory( src/cmd )
#  add_subdirectory( docs )
#ENDIF()



