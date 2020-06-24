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

if (LEDGER_NANO)
   if(UNIX AND NOT APPLE)
      set(LINUX TRUE)
   endif()


   add_static_library(
      NAME     ledger_signer 
      
      SOURCES 
        ledger_signer.c
        device_apdu_commands.c
        utility.c
        ethereum_apdu_client.c
   
      DEPENDS 
        eth_nano
   )
   
   target_compile_definitions(ledger_signer_o PRIVATE -D_POSIX_C_SOURCE=199309L)

   if(APPLE)
      target_link_libraries(ledger_signer ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_SHARED_LIBRARY_PREFIX}hidapi${CMAKE_SHARED_LIBRARY_SUFFIX} ${LIBS})
   elseif(LINUX)
      target_link_libraries(ledger_signer ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libhidapi-hidraw${CMAKE_SHARED_LIBRARY_SUFFIX} ${LIBS})
   elseif(WIN32)
   # not building for windows currently
   # target_link_libraries(ledger_signer ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_SHARED_LIBRARY_PREFIX}hidapi${CMAKE_SHARED_LIBRARY_SUFFIX} ${LIBS})
   else()
      target_link_libraries(ledger_signer ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libhidapi-hidraw${CMAKE_SHARED_LIBRARY_SUFFIX} ${LIBS})
   endif()

   add_dependencies(ledger_signer lib-hidapi)
endif()