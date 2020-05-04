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
#cmake_minimum_required(VERSION 3.15)
include("${PROJECT_SOURCE_DIR}/c/compiler.cmake")

set(HIDAPI_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(HIDAPI_BIN ${CMAKE_CURRENT_BINARY_DIR})

add_library(ledger_signer_o OBJECT ledger_signer.c device_apdu_commands.c)
target_compile_definitions(ledger_signer_o PRIVATE -D_POSIX_C_SOURCE=199309L)
add_library(ledger_signer STATIC $<TARGET_OBJECTS:ledger_signer_o>)
add_dependencies(ledger_signer lib-hidapi)
target_link_directories(ledger_signer PUBLIC ${CMAKE_BINARY_DIR}/lib)
target_link_libraries(ledger_signer hidapi ${LIBS})
