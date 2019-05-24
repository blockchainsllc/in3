cmake_minimum_required(VERSION 3.8.2)
set(QEMU_EXTRA_FLAGS -s)

#######################################################
### OPTIONS ###    REMEMBER TO RUN cmake.sh WHEN    ###
#                   YOU MAKE CHANGES IN OPTIONS!    ###
#######################################################

# typical options for USB dongle pca10059 (comment next 3 lines if NOT USB dongle)
OPTION(DEBUG "Set this ON for debug mode (on SDK shows dbg_log messages)" ON)
OPTION(USB_DONGLE "Set this ON for USB dongle version" ON)
OPTION(BT_MAC "Set this to force hardcoded BT MAC address" OFF)

# typical options for SDK board pca10056 (comment next 3 lines if NOT USDK board)
#OPTION(DEBUG "Set this ON for debug mode (on SDK shows dbg_log messages)" ON)
#OPTION(USB_DONGLE "Set this ON for USB dongle version" OFF)
#OPTION(BT_MAC "Set this to force hardcoded BT MAC address" ON)

IF(DEBUG)
  ADD_DEFINITIONS(-DDEBUG)
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

IF(USB_DONGLE)
  SET(BOARD nrf52840_pca10059)
  ADD_DEFINITIONS(-DUSB_DONGLE)
ELSE()
  SET(BOARD nrf52840_pca10056)
ENDIF()

IF(BT_MAC)
  ADD_DEFINITIONS(-DBT_MAC)
ENDIF()

ADD_DEFINITIONS(-DIN3_MATH_LITE)

#######################################################

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE
	src/main.c
	src/fsm.c
)


foreach(dep core eth_nano eth_basic eth_full usn_api )
file(GLOB files "../${dep}/**/*.c")
file(GLOB files2 "../${dep}/*.c")
target_sources(app PRIVATE "${files}" "${files2}")
    include_directories ("${PROJECT_SOURCE_DIR}/../${dep}")
  endforeach()


# include core in the headers
include(../core/compiler.cmake)
set_property(GLOBAL PROPERTY C_STANDARD 99)

