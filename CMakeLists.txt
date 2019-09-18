cmake_minimum_required(VERSION 3.6.1)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/scripts/cmake_modules/")
# project name
project(in3)
OPTION(TAG_VERSION "the tagged version, which should be used")

set(PROJECT_VERSION "0.1.0-alpha.0")
if (TAG_VERSION)
   set(PROJECT_VERSION "${TAG_VERSION}")
ENDIF(TAG_VERSION)
MESSAGE(STATUS "Building version ${PROJECT_VERSION}")

string(REPLACE "." ";" VERSION_LIST ${PROJECT_VERSION})
list(GET VERSION_LIST 0 PROJECT_VERSION_MAJOR)
list(GET VERSION_LIST 1 PROJECT_VERSION_MINOR)
list(GET VERSION_LIST 2 PROJECT_VERSION_PATCH)

ADD_DEFINITIONS("-DIN3_VERSION=\"${PROJECT_VERSION}\"")
ADD_DEFINITIONS(-DIN3_VERSION_MAJOR=${PROJECT_VERSION_MINOR})
ADD_DEFINITIONS(-DIN3_VERSION_MINOR=${PROJECT_VERSION_MINOR})
ADD_DEFINITIONS(-DIN3_VERSION_PATCH=${PROJECT_VERSION_PATCH})


IF (CMAKE_BUILD_TYPE MATCHES Debug)
    ADD_DEFINITIONS(-DDEBUG)
ENDIF (CMAKE_BUILD_TYPE MATCHES Debug)

OPTION(EVM_GAS "if true the gas costs are verified when validating a eth_call. This is a optimization since most calls are only interessted in the result. EVM_GAS would be required if the contract uses gas-dependend op-codes." ON)
IF (EVM_GAS)
    MESSAGE(STATUS "Enable GAS in EVM")
    ADD_DEFINITIONS(-DEVM_GAS)
ENDIF (EVM_GAS)

OPTION(IN3_LIB "if true a shared anmd static library with all in3-modules will be build." ON)

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

OPTION(SEGGER_RTT "Use the segger real time transfer terminal as the logging mechanism" OFF)
IF (SEGGER_RTT)
    MESSAGE(STATUS "Enable segger RTT for logging")
    ADD_DEFINITIONS(-DSEGGER_RTT)
ENDIF (SEGGER_RTT)

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
add_subdirectory(docs)


# create the library
if (IN3_LIB)
    set(IN3_LIBS
        $<TARGET_OBJECTS:core_o>
        $<TARGET_OBJECTS:crypto_o>
        $<TARGET_OBJECTS:tommath_o>
        $<TARGET_OBJECTS:evm_o>
        $<TARGET_OBJECTS:eth_full_o>
        $<TARGET_OBJECTS:eth_api_o>
        $<TARGET_OBJECTS:eth_basic_o>
        $<TARGET_OBJECTS:eth_nano_o>
        $<TARGET_OBJECTS:usn_api_o>
    )
    # if we use curl, we include curl transport as well
    if (USE_CURL)
        set(IN3_LIBS ${IN3_LIBS} $<TARGET_OBJECTS:transport_curl_o>)
    endif()

    # create the libraries
    add_library(in3_bundle STATIC ${IN3_LIBS} )
    add_library(in3_lib    SHARED ${IN3_LIBS} )
    set_target_properties(in3_bundle PROPERTIES OUTPUT_NAME "in3")
    set_target_properties(in3_lib PROPERTIES OUTPUT_NAME "in3")

    # for curl we need to add the dependency
    if (USE_CURL)
       target_link_libraries(in3_lib transport_curl)
    endif()

    # install
    INSTALL(TARGETS in3_bundle
      DESTINATION "lib"
    )
    INSTALL(TARGETS in3_lib
        DESTINATION lib
        PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE)
    INSTALL (
        DIRECTORY ${CMAKE_SOURCE_DIR}/include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h*")

endif()
