# add a option
option(LIBCURL_LINKTYPE "Link type for curl" SHARED)
if(${LIBCURL_LINKTYPE} MATCHES "static")
  add_definitions( -DCURL_STATICLIB )
  SET(CURL_SHARED "False")
  set(CURL_STATIC ON)
  SET(LIBCURL_LINKTYPE "STATIC_LIBRARY")
else()
  SET(CURL_SHARED "True")
  SET(LIBCURL_LINKTYPE "SHARED_LIBRARY")
endif()

# can we find curl installed?
message(STATUS "check for curl: $ENV{CURL_LIBRARY} ")
if(DEFINED ENV{CURL_LIBRARY})
  SET(CURL_LIBRARIES "$ENV{CURL_LIBRARY}/libcurl.a")
  SET(CURL_INCLUDE_DIRS $ENV{CURL_INCLUDE_DIR})
  message(STATUS "found a set curl:libs=$ENV{CURL_LIBRARIES} includeDirs= ${CURL_INCLUDE_DIRS} ")
  set(CURL_FOUND True)
else() 
   find_package(CURL)
endif()
message(STATUS "check for curl....: ${CURL_FOUND} ")

#ADD_DEFINITIONS(-DCURL_BLOCKING)

if( CURL_FOUND)
    message(STATUS "Found CURL version: ${CURL_VERSION_STRING} shoudl be type=${LIBCURL_LINKTYPE} but  ${LIBCURL_TYPE}")
    message(STATUS "Using CURL include dir(s): ${CURL_INCLUDE_DIRS}")
    message(STATUS "Using CURL lib(s): ${CURL_LIBRARIES}")

    add_library(CONAN_PKG::libcurl INTERFACE IMPORTED)

    # Property INTERFACE_LINK_FLAGS do not work, necessary to add to INTERFACE_LINK_LIBRARIES
    set_property(TARGET CONAN_PKG::libcurl PROPERTY INTERFACE_LINK_LIBRARIES ${CURL_LIBRARIES})
    set_property(TARGET CONAN_PKG::libcurl PROPERTY INTERFACE_INCLUDE_DIRECTORIES  ${CURL_INCLUDE_DIRS})
    if(${LIBCURL_LINKTYPE} MATCHES "STATIC_LIBRARY")
      set(CONAN_COMPILE_DEFINITIONS_LIBCURL "CURL_STATICLIB=1")
      set_property(TARGET CONAN_PKG::libcurl PROPERTY INTERFACE_COMPILE_DEFINITIONS ${CONAN_COMPILE_DEFINITIONS_LIBCURL})
    endif()
else()
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
    include(${CMAKE_BINARY_DIR}/conan.cmake)

    # run conan
    conan_cmake_run(REQUIRES libcurl/7.61.1@bincrafters/stable 
                    OPTIONS libcurl:shared=${CURL_SHARED}
                    BASIC_SETUP CMAKE_TARGETS
                    BUILD missing)

endif()

# static lib
add_library( transport_curl STATIC in3_curl.c in3_storage.c)

# props
include(../core/compiler.cmake)

# add dependency
target_link_libraries(transport_curl CONAN_PKG::libcurl)
if(MSVC OR MSYS OR MINGW)
    # for detecting Windows compilers
#    target_link_libraries(transport_curl ws2_32 wsock32 pthread )
    target_link_libraries(transport_curl ws2_32 wsock32 -static gcc stdc++ winpthread -dynamic )
endif()

target_include_directories(transport_curl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
