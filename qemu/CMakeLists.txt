cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(z_in3)

target_sources(app PRIVATE src/main.c)

ADD_DEFINITIONS(-DTEST)
SET(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_VERBOSE_MAKEFILE ON)
ADD_DEFINITIONS(-DZEPHYR_OS)
ADD_DEFINITIONS(-DZEPHYR)
ADD_DEFINITIONS(-DIN3_MATH_LITE)
#set(CMAKE_C_FLAGS "-fno-stack-protector")
foreach(dep ../src/core ../src/verifier/eth1/basic ../src/verifier/eth1/full ../src/verifier/eth1/evm  ../src/verifier/eth1/nano ../src/api/eth1 ../src/third-party)
  file(GLOB files "${dep}/**/*.c")
  file(GLOB files2 "${dep}/*.c")
  message("${files}")
  message("${files2}")
  target_sources(app PRIVATE "${files}" "${files2}")
  include_directories ("${dep}")
endforeach()
target_include_directories (app PRIVATE "../include")


