###############################################################################
# This file is part of the IN3 project.
# Sources: https://github.com/blockchainsllc/in3
#
# Copyright (C) 2018-2021 slock.it GmbH, Blockchains LLC
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

set(EMC_PROPS "-s ALLOW_MEMORY_GROWTH=1 -s NODEJS_CATCH_REJECTION=0 -s EXPORT_NAME=in3w  -s FILESYSTEM=0 -s  'EXPORTED_RUNTIME_METHODS=[\"ccall\", \"cwrap\"]'")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wpointer-arith ")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -Wpointer-arith ")

IF (ASMJS)
  set(WASM_EMBED true)
  set(CMAKE_EXECUTABLE_SUFFIX ".js")
  set(EMC_PROPS "${EMC_PROPS} -s FINALIZE_ASM_JS=1 -s WASM=0 -s ASM_JS=1 ")
else(ASMJS)
  set(EMC_PROPS "${EMC_PROPS} -s WASM=1")
endif(ASMJS)


IF (CMAKE_BUILD_TYPE MATCHES Debug)
   set(EMC_PROPS "${EMC_PROPS} -g -s SAFE_HEAP=1 -s ASSERTIONS=1 -s STACK_OVERFLOW_CHECK=1 -s RUNTIME_LOGGING=1 ")
ENDIF (CMAKE_BUILD_TYPE MATCHES Debug)

if (WASM_EMBED)
   set(EMC_PROPS "${EMC_PROPS} -s SINGLE_FILE=1")
endif(WASM_EMBED)

if (WASM_EMMALLOC)
   set(EMC_PROPS "${EMC_PROPS} -s MALLOC=emmalloc")
endif(WASM_EMMALLOC)

if (WASM_SYNC)
   set(EMC_PROPS "${EMC_PROPS} -s WASM_ASYNC_COMPILATION=0")
endif(WASM_SYNC)

add_executable(in3w wasm.c)
target_link_libraries(in3w init)
set_target_properties(in3w PROPERTIES LINK_FLAGS "${EMC_PROPS}")


add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/bin/in3.js
    DEPENDS in3w  in3.js modules/eth.js modules/btc.js modules/ipfs.js in3_util.js index.d.ts modules/btc.d.ts modules/ipfs.d.ts modules/eth.d.ts
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/
    COMMAND  ${CMAKE_CURRENT_SOURCE_DIR}/build_js.sh ${CMAKE_CURRENT_SOURCE_DIR} ${ASMJS} ${WASM_MODULES}
)

add_custom_target(in3_wasm ALL DEPENDS ${CMAKE_BINARY_DIR}/bin/in3.js)
add_custom_target(test
   COMMAND npm test
   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../test
)
