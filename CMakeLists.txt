cmake_minimum_required(VERSION 3.7.2)

# project name
project(in3)

set(PROJECT_VERSION "0.0.0-alpha.0")

string(REPLACE "." ";" VERSION_LIST ${PROJECT_VERSION})
list(GET VERSION_LIST 0 PROJECT_VERSION_MAJOR)
list(GET VERSION_LIST 1 PROJECT_VERSION_MINOR)
list(GET VERSION_LIST 2 PROJECT_VERSION_PATCH)

ADD_DEFINITIONS(-DIN3_VERSION=${PROJECT_VERSION})
ADD_DEFINITIONS(-DIN3_VERSION_MAJOR=${PROJECT_VERSION_MINOR})
ADD_DEFINITIONS(-DIN3_VERSION_MINOR=${PROJECT_VERSION_MINOR})
ADD_DEFINITIONS(-DIN3_VERSION_PATCH=${PROJECT_VERSION_PATCH})

# options
OPTION(DEBUG "Debug" OFF)
IF (DEBUG)
    ADD_DEFINITIONS(-DDEBUG)
    SET(CMAKE_BUILD_TYPE Debug)
ENDIF (DEBUG)

OPTION(EVM_GAS "Evm Gas" ON)
IF (EVM_GAS)
    MESSAGE(STATUS "Enable GAS in EVM")
    ADD_DEFINITIONS(-DEVM_GAS)
ENDIF (EVM_GAS)

OPTION(TEST "Test" OFF)
IF (TEST)
    MESSAGE(STATUS "Build Tests and add debug infos")
    ADD_DEFINITIONS(-DTEST)
    ADD_DEFINITIONS(-DLOG_USE_COLOR)
    SET(CMAKE_BUILD_TYPE Debug)
    enable_testing()
    add_subdirectory(test)
    add_custom_target(ptest COMMAND ${CMAKE_CTEST_COMMAND} -j 16)
ENDIF (TEST)

OPTION(FAST_MATH "Math optimizations" OFF)
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
add_subdirectory(docs)

add_library(in3 STATIC $<TARGET_OBJECTS:core> $<TARGET_OBJECTS:crypto> $<TARGET_OBJECTS:tommath> $<TARGET_OBJECTS:eth_full> $<TARGET_OBJECTS:eth_api>  $<TARGET_OBJECTS:eth_basic> $<TARGET_OBJECTS:eth_nano> )
target_link_libraries(in3 PUBLIC transport_curl)

