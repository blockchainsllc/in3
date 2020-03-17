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

IF (USE_CURL)
    message("using curl for comandline")
    set(LIBS ${LIBS} transport_curl)
else()
    message("using http for comandline")
    set(LIBS ${LIBS} transport_http)
ENDIF()

IF (IN3_SERVER)
    message("using server for comandline")
    ADD_DEFINITIONS(-DIN3_SERVER)
    set(LIBS ${LIBS} http_server)
ENDIF()

if (BTC)
    ADD_DEFINITIONS(-DBTC)
    set(LIBS ${LIBS} btc)
endif()

if (IPFS)
    ADD_DEFINITIONS(-DIPFS)
    set(LIBS ${LIBS} ipfs ipfs_api )
endif()


add_executable(in3 main.c in3_storage.c)
target_compile_definitions(in3 PRIVATE _XOPEN_SOURCE=600)
target_link_libraries(in3 ${LIBS} eth_full eth_api  -lm)

install(TARGETS in3
        DESTINATION /usr/local/bin/
        PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE)
