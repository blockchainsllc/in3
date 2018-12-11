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

# add core
add_subdirectory( src/core )




set_property(TARGET in3-core PROPERTY C_STANDARD 99)
