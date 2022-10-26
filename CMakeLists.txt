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

cmake_minimum_required(VERSION 3.5.1)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/scripts/cmake_modules/")

if (CRYPTOCELL)
    find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
endif()

if (CRYPTOCELL)
   zephyr_interface_library_named(cryptocell)
   set(CRYPTOCELL_SOURCE_DIR              ${CMAKE_CURRENT_SOURCE_DIR}/c/src/core/util/cryptocell)
else()
    project(in3)
endif()

include("c/options.cmake")

if (CRYPTOCELL)
    zephyr_get_include_directories_for_lang_as_string(C includes STRIP_PREFIX)
    zephyr_get_compile_definitions_for_lang_as_string(C definitions)
    zephyr_get_compile_options_for_lang_as_string(C options)
    zephyr_library_include_directories(${includes})
    zephyr_library_compile_definitions(${definitions})
    zephyr_library_include_directories(
        /ncs/mbedtls/include
        ${CMAKE_CURRENT_SOURCE_DIR}/c/include/in3
        ${CMAKE_CURRENT_SOURCE_DIR}/c/include
    )
    zephyr_library_sources(${CRYPTOCELL_SOURCE_DIR}/src/cryptocell.c)
    zephyr_library_link_libraries(cryptocell core crypto init eth_basic cryptocell_signer)
endif()

# build tests
if(TEST)
    add_subdirectory(c/test)
endif(TEST)

add_subdirectory(c)
