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

include("macro.cmake")
include("compiler.cmake")

#  build modules
add_subdirectory(src/third-party)


if (GCC_ANALYZER) 
  ADD_DEFINITIONS(-DGCC_ANALYZER=1)
  add_compile_options(-fanalyzer -Wno-nonnull-compare)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fanalyzer")
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fanalyzer")
endif()

IF (TRANSPORTS AND NOT SWIFT)
    ADD_DEFINITIONS(-DTRANSPORTS)
    if (USE_CURL)
        ADD_DEFINITIONS(-DUSE_CURL)
        set(IN3_TRANSPORT ${IN3_TRANSPORT} transport_curl)
        if (CURL_BLOCKING)
            ADD_DEFINITIONS(-DCURL_BLOCKING)
        endif (CURL_BLOCKING)
    elseif(USE_WINHTTP)
       ADD_DEFINITIONS(-DUSE_WINHTTP)
       set(IN3_TRANSPORT ${IN3_TRANSPORT} transport_winhttp)
    else ()
        set(IN3_TRANSPORT ${IN3_TRANSPORT} transport_http)
    endif (USE_CURL)
    add_subdirectory(src/transport)
ENDIF (TRANSPORTS AND NOT SWIFT)

ADD_DEFINITIONS("-DIN3_AUTOINIT_PATH=\"${CMAKE_BINARY_DIR}/autoregister.h\"")

add_subdirectory(src/core)
add_subdirectory(src/init)
add_subdirectory(src/verifier)
add_subdirectory(src/signer)
add_subdirectory(src/pay)
add_subdirectory(src/api)
add_subdirectory(src/cmd)
add_subdirectory(src/tools)
add_subdirectory(src/nodeselect)
add_subdirectory(docs)

# generate the init-module
get_property(modules GLOBAL PROPERTY IN3_REGISTERS)
foreach(m IN LISTS modules)
  set(REGISTER_DEF "${REGISTER_DEF}in3_ret_t ${m} (in3_t*);\n")
  set(REGISTER_CALL "${REGISTER_CALL}  in3_register_default(${m});\n")
endforeach()
file(WRITE ${CMAKE_BINARY_DIR}/autoregister.h "${REGISTER_DEF}\n\nstatic void auto_init() {\n${REGISTER_CALL}}\n")

# define lib dir
link_directories(${CMAKE_BINARY_DIR}/lib/)

# create the library
if (IN3_LIB)
   get_property(IN3_LIBS GLOBAL PROPERTY IN3_OBJECTS)

    # create static libraries
    add_library(in3_bundle STATIC ${IN3_LIBS} )
    set_target_properties(in3_bundle PROPERTIES OUTPUT_NAME "in3")
    if (ZKSYNC AND ZKCRYPTO_LIB )
        target_link_libraries(in3_bundle zk_crypto_rs )
    endif()
    INSTALL(TARGETS in3_bundle
        DESTINATION "lib"
    )

    IF (NOT SWIFT)

        # create shared libraries
        add_library(in3_lib    SHARED ${IN3_LIBS} )
        set_target_properties(in3_lib PROPERTIES OUTPUT_NAME "in3")
        target_link_libraries(in3_lib ${IN3_TRANSPORT})  

        if( LEDGER_NANO)
            target_link_libraries(in3_lib  ledger_signer) 
        endif()
        if(SENTRY)
            target_link_libraries(in3_lib sentry) 
        endif()

        if (ZKSYNC AND ZKCRYPTO_LIB )
            target_link_libraries(in3_lib zk_crypto_rs )
        endif()

        if (VADE)
            if(APPLE)
                target_link_libraries(in3_lib "-framework Security")
            elseif(LINUX)
                target_link_libraries(in3_lib "-lpthread")
            endif()

            target_link_libraries(in3_lib vade_evan )
            
        endif()


        # install
        INSTALL(TARGETS in3_lib
            DESTINATION lib
            PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE)
    ENDIF(NOT SWIFT)


    INSTALL (
        DIRECTORY ${CMAKE_SOURCE_DIR}/c/include/in3
        DESTINATION include
        FILES_MATCHING PATTERN "*.h*")

endif()

