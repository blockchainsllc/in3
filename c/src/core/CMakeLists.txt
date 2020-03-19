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

# main incubed module defining the interfaces for transport, verifier and storage. 
#
# This module does not have any dependencies and cannot be used without additional modules providing verification and transport.
OPTION(IN3_STAGING "if true, the client will use the staging-network instead of the live ones" OFF)
IF (IN3_STAGING)
    ADD_DEFINITIONS(-DIN3_STAGING)
ENDIF (IN3_STAGING)

add_library(core_o OBJECT
        client/context.c
        client/client.c
        client/cache.c
        client/nodelist.c
        client/verifier.c
        client/execute.c
        client/client_init.c
        util/debug.c
        util/bytes.c
        util/utils.c
        util/scache.c
        util/data.c
        util/log.c
        util/mem.c
        util/stringbuilder.c
        util/bitset.c
        )
add_library(core STATIC $<TARGET_OBJECTS:core_o>)
target_link_libraries(core crypto)

add_library(init_o OBJECT ../verifier/in3_init.c)
add_library(init STATIC $<TARGET_OBJECTS:init_o>)
target_link_libraries(init ${IN3_VERIFIER} ${IN3_API} ${IN3_TRANSPORT})
