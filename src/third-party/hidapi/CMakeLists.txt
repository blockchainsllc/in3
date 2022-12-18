###############################################################################
# This file is part of the Incubed project.
# Sources: https://github.com/blockchainsllc/in3
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
#cmake_minimum_required(VERSION 3.15)

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

add_compile_options(-Wno-unused-parameter -Wno-sign-compare -Wno-int-conversion -Wno-implicit-function-declaration -Wno-conditional-type-mismatch)

if(APPLE)
   add_static_library(
      NAME     hidapi 
      
      SOURCES 
         mac/hid.c

      DEPENDS
         "-framework CoreFoundation"
         "-framework IOKit"
         "-framework AppKit"
   )

elseif(WIN32 OR MSVC OR MSYS OR MINGW)
   add_static_library(
      NAME     hidapi 
      
      SOURCES 
         windows/hid.c

      DEPENDS
         -lsetupapi
   )
   
elseif(LINUX)

  find_path(LIBUSB_INCLUDE_DIR
  NAMES libusb.h
  PATH_SUFFIXES "include" "libusb" "libusb-1.0")

 # find_library(LIBUSB_LIBRARY
#  NAMES usb-1.0
 # PATH_SUFFIXES "lib" "lib32" "lib64")

  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

   add_static_library(
      NAME     hidapi 
      
      SOURCES 
         libusb/hid.c

      DEPENDS
         -lusb-1.0
         -lpthread
   )

   target_include_directories(hidapi_o PRIVATE ${LIBUSB_INCLUDE_DIR})
 
else()
   
endif()

