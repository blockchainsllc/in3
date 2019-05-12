cmake_minimum_required(VERSION 3.8.2)
set(QEMU_EXTRA_FLAGS -s)

#######################################################
### OPTIONS ###    REMEMBER TO RUN cmake.sh WHEN    ###
#                   YOU MAKE CHANGES IN OPTIONS!    ###
#######################################################

OPTION(USB_DONGLE "Set this ON for USB dongle version" ON)
OPTION(BT_MAC "Set this to force hardcoded BT MAC address" OFF)
OPTION(DEBUG "Set this ON for debug mode (on SDK shows dbg_log messages)" OFF)

IF(USB_DONGLE)
  SET(BOARD nrf52840_pca10059)
  ADD_DEFINITIONS(-DUSB_DONGLE)
ELSE()
  SET(BOARD nrf52840_pca10056)
ENDIF()

IF(BT_MAC)
  ADD_DEFINITIONS(-DBT_MAC)
ENDIF()

IF(DEBUG)
  ADD_DEFINITIONS(-DDEBUG)
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF()
#######################################################

# next line removes the >"unused parameter" warning when compiling
ADD_DEFINITIONS(-Wno-unused-parameter)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

set_property(GLOBAL PROPERTY C_STANDARD 99)

target_sources(app PRIVATE
	src/main.c
	src/fsm.c
	src/rent.c
  src/meterReadings.c
  src/uart_comm.c
)

# include core in the headers
include_directories ("${PROJECT_SOURCE_DIR}/../core")
include_directories ("${PROJECT_SOURCE_DIR}/../eth_full")
include_directories ("${PROJECT_SOURCE_DIR}/../eth_api")
include_directories ("${PROJECT_SOURCE_DIR}/../eth_nano")
include_directories ("${PROJECT_SOURCE_DIR}/../transport_curl")
include_directories ("${PROJECT_SOURCE_DIR}/../eth_basic")
include("${PROJECT_SOURCE_DIR}/include_eth_core.cmake")
include("${PROJECT_SOURCE_DIR}/include_eth_nano.cmake")

# choose which module to include
#include(../eth_basic/include.cmake)
#include(../eth_nano/include.cmake)
include(../core/compiler.cmake)

# add dependency
#add_subdirectory( ../core "${PROJECT_BINARY_DIR}/core")
#target_link_libraries(app core)

