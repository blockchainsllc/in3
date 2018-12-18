cmake_minimum_required(VERSION 3.8.2)
set(QEMU_EXTRA_FLAGS -s)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)


OPTION(DEBUG "Debug" OFF)
IF(DEBUG)
  ADD_DEFINITIONS(-DDEBUG)
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF(DEBUG)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__='\"$(subst
  ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

#add_definitions(-DZEPHYR_OS)
#add_definitions(-DZEPHYR)
#zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
#include_directories($ENV{ZEPHYR_BASE}/samples/in3-test-receiver/src/crypto)



target_sources(app PRIVATE
	src/main.c
	src/fsm.c
	src/rent.c
)
set_property(TARGET app PROPERTY C_STANDARD 99)


# include core in the headers
include_directories ("${PROJECT_SOURCE_DIR}/../core")
include_directories ("${PROJECT_SOURCE_DIR}/../eth_nano")
include(../core/include.cmake)
include(../eth_nano/include.cmake)

# add dependency
# add_subdirectory( ../core "${PROJECT_BINARY_DIR}/core")
#target_link_libraries(app core)

