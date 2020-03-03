###############################################################################
# This file is part of the Incubed project.
# Sources: https://github.com/slockit/in3-c
# 
# Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
# 
# 
# COMMERCIAL LICENSE USAGE
# 
# Licensees holding a valid commercial license may use this file in accordance 
# with the commercial license agreement provided with the Software or, alternatively, 
# in accordance with the terms contained in a written agreement between you and 
# slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
# information please contact slock.it at in3@slock.it.
# 	
# Alternatively, this file may be used under the AGPL license as follows:
#    
# AGPL LICENSE USAGE
# 
# This program is free software: you can redistribute it and/or modify it under the
# terms of the GNU Affero General Public License as published by the Free Software 
# Foundation, either version 3 of the License, or (at your option) any later version.
#  
# This program is distributed in the hope that it will be useful, but WITHOUT ANY 
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
# PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
# [Permissions of this strong copyleft license are conditioned on making available 
# complete source code of licensed works and modifications, which include larger 
# works using a licensed work, under the same license. Copyright and license notices 
# must be preserved. Contributors provide an express grant of patent rights.]
# You should have received a copy of the GNU Affero General Public License along 
# with this program. If not, see <https://www.gnu.org/licenses/>.
###############################################################################
include("${PROJECT_SOURCE_DIR}/c/compiler.cmake")

# add a option
option(LIBCURL_LINKTYPE "Link type for curl" SHARED)
if (${LIBCURL_LINKTYPE} MATCHES "static")
    add_definitions(-DCURL_STATICLIB)
    SET(CURL_SHARED "False")
    set(CURL_STATIC ON)
    SET(LIBCURL_LINKTYPE "STATIC_LIBRARY")
else ()
    SET(CURL_SHARED "True")
    SET(LIBCURL_LINKTYPE "SHARED_LIBRARY")
endif ()

# can we find curl installed?
message(STATUS "check for curl: $ENV{CURL_LIBRARY} ")
if (DEFINED ENV{CURL_LIBRARY})
    SET(CURL_LIBRARIES "$ENV{CURL_LIBRARY}/libcurl.a")
    SET(CURL_INCLUDE_DIRS $ENV{CURL_INCLUDE_DIR})
    message(STATUS "found a set curl:libs=$ENV{CURL_LIBRARIES} includeDirs= ${CURL_INCLUDE_DIRS} ")
    set(CURL_FOUND True)
else ()
    find_package(CURL)
endif ()
message(STATUS "check for curl....: ${CURL_FOUND} ")

if (CURL_FOUND)
    message(STATUS "Found CURL version: ${CURL_VERSION_STRING} shoudl be type=${LIBCURL_LINKTYPE} but  ${LIBCURL_TYPE}")
    message(STATUS "Using CURL include dir(s): ${CURL_INCLUDE_DIRS}")
    message(STATUS "Using CURL lib(s): ${CURL_LIBRARIES}")

    add_library(CONAN_PKG::libcurl INTERFACE IMPORTED)

    # Property INTERFACE_LINK_FLAGS do not work, necessary to add to INTERFACE_LINK_LIBRARIES
    set_property(TARGET CONAN_PKG::libcurl PROPERTY INTERFACE_LINK_LIBRARIES ${CURL_LIBRARIES})
    set_property(TARGET CONAN_PKG::libcurl PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CURL_INCLUDE_DIRS})
    if (${LIBCURL_LINKTYPE} MATCHES "STATIC_LIBRARY")
        set(CONAN_COMPILE_DEFINITIONS_LIBCURL "CURL_STATICLIB=1")
        set_property(TARGET CONAN_PKG::libcurl PROPERTY INTERFACE_COMPILE_DEFINITIONS ${CONAN_COMPILE_DEFINITIONS_LIBCURL})
    endif ()
else ()
    # Download automatically, you can also just copy the conan.cmake file
    if (NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
        message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
        file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.13/conan.cmake"
                "${CMAKE_BINARY_DIR}/conan.cmake")
    endif ()

    # stupid workaround since conan may not detect compilers correctly
    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(ENV{CC} "")
        set(ENV{CXX} "")
    endif ()


    # shared or static? 
    include(${CMAKE_BINARY_DIR}/conan.cmake)

    # run conan
    conan_cmake_run(REQUIRES libcurl/7.61.1@bincrafters/stable
            OPTIONS libcurl:shared=${CURL_SHARED}
            BASIC_SETUP CMAKE_TARGETS
            BUILD missing)
    set(CURL_INCLUDE_DIRS,$<TARGET_PROPERTY:CONAN_PKG::libcurl,INTERFACE_INCLUDE_DIRECTORIES>)

endif ()

# static lib
#add_library(transport_curl STATIC in3_curl.c)
add_library(transport_curl_o OBJECT in3_curl.c)
add_library(transport_curl STATIC $<TARGET_OBJECTS:transport_curl_o>)



# add dependency
target_link_libraries(transport_curl CONAN_PKG::libcurl)

if (CURL_INCLUDE_DIRS)
  target_include_directories(transport_curl_o PRIVATE ${CURL_INCLUDE_DIRS})
else()
  target_link_libraries(transport_curl_o CONAN_PKG::libcurl)
endif()
#target_compile_definitions(transport_curl_o $<TARGET_PROPERTY:CONAN_PKG::libcurl,INTERFACE_COMPILE_DEFINITIONS>)


if (MSVC OR MSYS OR MINGW)
    # for detecting Windows compilers
    target_link_libraries(transport_curl_o ws2_32 wsock32 -static gcc -Lcrypto stdc++ winpthread -L/home/conan/xc/mingw64/ssl/lib/libcrypto.a -L/home/conan/xc/mingw64/ssl/lib/libssl.a -dynamic)
    target_link_libraries(transport_curl ws2_32 wsock32 -static gcc stdc++ winpthread -L/home/conan/xc/mingw64/ssl/lib/libcrypto.a -L/home/conan/xc/mingw64/ssl/lib/libssl.a -dynamic)
endif ()
