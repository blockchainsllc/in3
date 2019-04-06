cmake_minimum_required(VERSION 3.8.2)
set(QEMU_EXTRA_FLAGS -s)

#######################################################
### OPTIONS ###    REMEMBER TO RUN cmake.sh WHEN    ###
#                   YOU MAKE CHANGES IN OPTIONS!    ###
#######################################################

# typical options for USB dongle pca10059 (comment next 3 lines if NOT USB dongle)
OPTION(DEBUG "Set this ON for debug mode (on SDK shows dbg_log messages)" OFF)
OPTION(USB_DONGLE "Set this ON for USB dongle version" ON)
OPTION(BT_MAC "Set this to force hardcoded BT MAC address" OFF)

# typical options for SDK board pca10056 (comment next 3 lines if NOT USDK board)
#OPTION(DEBUG "Set this ON for debug mode (on SDK shows dbg_log messages)" ON)
#OPTION(USB_DONGLE "Set this ON for USB dongle version" OFF)
#OPTION(BT_MAC "Set this to force hardcoded BT MAC address" ON)

IF(USB_DONGLE)
  SET(BOARD nrf52840_pca10059)
  ADD_DEFINITIONS(-DUSB_DONGLE)
ELSE()
  SET(BOARD nrf52840_pca10056)
  IF(DEBUG)
    ADD_DEFINITIONS(-DDEBUG)
    SET(CMAKE_BUILD_TYPE Debug)
  ENDIF()
ENDIF()

IF(BT_MAC)
  ADD_DEFINITIONS(-DBT_MAC)
ENDIF()

#######################################################

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE
	src/main.c
	src/fsm.c
)

# include core in the headers
include_directories ("${PROJECT_SOURCE_DIR}/../core")
include_directories ("${PROJECT_SOURCE_DIR}/../usn_api")
include_directories ("${PROJECT_SOURCE_DIR}/../eth_nano")
include_directories ("${PROJECT_SOURCE_DIR}/../eth_basic")
include_directories ("${PROJECT_SOURCE_DIR}/../eth_full")
include(../core/include.cmake)

# choose which module to include
#include(../eth_basic/include.cmake)
include(../eth_nano/include.cmake)
include(../eth_basic/include.cmake)
include(../eth_full/include.cmake)
include(../usn_api/include.cmake)
include(../core/compiler.cmake)
set_property(GLOBAL PROPERTY C_STANDARD 99)

# add dependency
# add_subdirectory( ../core "${PROJECT_BINARY_DIR}/core")
#target_link_libraries(app core)

