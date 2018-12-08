cmake_minimum_required(VERSION 3.8.2)

# project name
project(in3-core)

# configure a header file to pass some of the CMake settings
# to the source code
# The version number.
set (IN3_VERSION_MAJOR 1)
set (IN3_VERSION_MINOR 0)
configure_file (
  "${PROJECT_SOURCE_DIR}/src/util/config.h.in"
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

#include_directories ("${PROJECT_SOURCE_DIR}/src")

# add the executable
add_executable(in3-core src/main.c)

target_sources(in3-core PRIVATE
	src/main.c
	src/util/debug.c
	src/util/bytes.c
	src/util/utils.c
	src/jsmn/jsmn.c
	src/jsmn/jsmnutil.c
)

