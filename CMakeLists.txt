cmake_minimum_required(VERSION 3.7.2)

# project name
project(in3)

# options
OPTION(DEBUG "Debug" OFF)
IF(DEBUG)
  ADD_DEFINITIONS(-DDEBUG)
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF(DEBUG)

OPTION(TEST "Test" OFF)
IF(TEST)

  MESSAGE ( STATUS "Build Tests" )
  ADD_DEFINITIONS(-DTEST)
  SET(CMAKE_BUILD_TYPE Debug)
  enable_testing()
  add_subdirectory( test )

ENDIF(TEST)

# substiture __FILENAME__ as it is used in DEBUG
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__='\"$(subst
  ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

#  build modules
add_subdirectory( src/core )
add_subdirectory( src/libin3 )
add_subdirectory( src/transport_curl )
add_subdirectory( src/eth_nano )
add_subdirectory( src/eth_basic )
add_subdirectory( src/eth_full )
add_subdirectory( src/cmd )

add_subdirectory( docs )
#add_subdirectory( src/wasm )


