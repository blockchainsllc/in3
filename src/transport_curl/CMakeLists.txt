# Download automatically, you can also just copy the conan.cmake file
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.13/conan.cmake"
                 "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

# stupid workaround since conan may not detect compilers correctly
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(ENV{CC} "")
  set(ENV{CXX} "")
endif()


# shared or static? 
option(LIBCURL_TYPE "Link type for curl" SHARED)
if(${LIBCURL_TYPE} MATCHES "static")
  SET(CURL_SHARED "False")
else()
  SET(CURL_SHARED "True")
endif()
include(${CMAKE_BINARY_DIR}/conan.cmake)

# run conan
conan_cmake_run(REQUIRES libcurl/7.61.1@bincrafters/stable 
                OPTIONS libcurl:shared=${CURL_SHARED}
                BASIC_SETUP CMAKE_TARGETS
                BUILD missing)

# static lib
add_library( in3_curl STATIC in3_curl.c in3_storage.c)

# add dependency
target_link_libraries(in3_curl CONAN_PKG::libcurl)

# props
include(../core/compiler.cmake)
