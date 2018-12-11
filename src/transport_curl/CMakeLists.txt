message(STATUS "Looking for CURL")
include(FindCURL)
find_package(CURL REQUIRED)

if(CURL_FOUND)
  message(STATUS "Found CURL version: ${CURL_VERSION_STRING}")
  message(STATUS "Using CURL include dir(s): ${CURL_INCLUDE_DIRS}")
  message(STATUS "Using CURL lib(s): ${CURL_LIBRARIES}")
  if(CURL_VERSION VERSION_LESS 7.61.1)
     message(STATUS "Could not use a dynamic CURL")
  endif()
else()
  message(FATAL_ERROR "Could not find CURL")
endif()

# All following targets should search these directories for headers
include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CURL_INCLUDE_DIRS}
)

add_library( in3_curl STATIC in3_curl.c )
set_property(TARGET in3_curl PROPERTY C_STANDARD 99)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")