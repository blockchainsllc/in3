cmake_minimum_required(VERSION 3.8.2)
set(QEMU_EXTRA_FLAGS -s)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)


OPTION(DEBUG "Debug" OFF)
IF(DEBUG)
  ADD_DEFINITIONS(-DDEBUG)
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF(DEBUG)


target_sources(app PRIVATE
	src/main.c
	src/fsm.c
	src/rent.c
)

# include core in the headers
include_directories ("${PROJECT_SOURCE_DIR}/../core")
include_directories ("${PROJECT_SOURCE_DIR}/../eth_full")
include_directories ("${PROJECT_SOURCE_DIR}/../eth_nano")
include_directories ("${PROJECT_SOURCE_DIR}/../eth_basic")
include(../core/include.cmake)

# choose which mlodule to include
#include(../eth_basic/include.cmake)

include(../eth_full/include.cmake)
include(../core/compiler.cmake)
set_property(GLOBAL PROPERTY C_STANDARD 99)

# add dependency
# add_subdirectory( ../core "${PROJECT_BINARY_DIR}/core")
#target_link_libraries(app core)

