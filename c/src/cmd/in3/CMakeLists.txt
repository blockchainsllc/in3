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

if ( NOT CMD_NAME )
  set(CMD_NAME in3)
endif ()

ADD_DEFINITIONS("-DCMD_NAME=\"${CMD_NAME}\"")

IF (IN3_SERVER)
    ADD_DEFINITIONS(-DIN3_SERVER)
    set(LIBS ${LIBS} http_server)
ENDIF()

if (LEDGER_NANO)
    set(LIBS ${LIBS} ledger_signer)
endif()

if (MULTISIG)
    set(LIBS ${LIBS} multisig)
endif()


add_executable(${CMD_NAME} 
  helper.c 
  main.c 
  option_handler.c 
  rpc_handler.c 
  req_exec.c 
  transport.c 
  tx.c 
  weights.c 
  in3_storage.c 
  ../../tools/recorder/recorder.c
)
target_compile_definitions(${CMD_NAME} PRIVATE _XOPEN_SOURCE=600)

target_link_libraries(${CMD_NAME} init pk_signer ${LIBS} -lm)

install(TARGETS ${CMD_NAME}
        DESTINATION /usr/local/bin/
        PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE)
