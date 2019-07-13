cmake_minimum_required(VERSION 3.7.2)

# project name
project(in3)

# options
OPTION(DEBUG "Turns on Debug output in the code. This would be required if the tests should output additional debug infos." OFF)
IF (DEBUG)
    ADD_DEFINITIONS(-DDEBUG)
    SET(CMAKE_BUILD_TYPE Debug)
ENDIF (DEBUG)

OPTION(EVM_GAS "if true the gas costs are verified when validating a eth_call. This is a optimization since most calls are only interessted in the result. EVM_GAS would be required if the contract uses gas-dependend op-codes." ON)
IF (EVM_GAS)
    MESSAGE(STATUS "Enable GAS in EVM")
    ADD_DEFINITIONS(-DEVM_GAS)
ENDIF (EVM_GAS)

OPTION(TEST "builds the tests and also adds special memory-management, which detects memory leaks, but will cause slower performance" OFF)
IF (TEST)
    MESSAGE(STATUS "Build Tests and add debug infos")
    ADD_DEFINITIONS(-DTEST)
    ADD_DEFINITIONS(-DLOG_USE_COLOR)
    SET(CMAKE_BUILD_TYPE Debug)
    enable_testing()
    add_subdirectory(test)
    add_custom_target(ptest COMMAND ${CMAKE_CTEST_COMMAND} -j 16)
ENDIF (TEST)

OPTION(FAST_MATH "Math optimizations used in the EVM. This will also increase the filesize." OFF)
IF (FAST_MATH)
    MESSAGE(STATUS "Enable math optimizations (excutable size may increase)")
    ADD_DEFINITIONS(-DIN3_MATH_FAST)
ELSE ()
    MESSAGE(STATUS "Disable math optimizations (optimised for executable size)")
    ADD_DEFINITIONS(-DIN3_MATH_LITE)
ENDIF (FAST_MATH)

# substiture __FILENAME__ as it is used in DEBUG
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include("${PROJECT_SOURCE_DIR}/compiler.cmake")

#  build modules
add_subdirectory(src/third-party)
add_subdirectory(src/core)
add_subdirectory(src/transport)
add_subdirectory(src/verifier)
add_subdirectory(src/bindings)
add_subdirectory(src/api)
add_subdirectory(src/cmd)
add_subdirectory(src/examples)
add_subdirectory( docs )



