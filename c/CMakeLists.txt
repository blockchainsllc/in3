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

#  build modules
add_subdirectory(src/third-party)
add_subdirectory(src/core)
add_subdirectory(src/transport)
add_subdirectory(src/verifier)
add_subdirectory(src/api)
IF (ETH_FULL)
    add_subdirectory(src/cmd)
endif()
add_subdirectory(docs)



# create the library
if (IN3_LIB)
    set(IN3_LIBS
        $<TARGET_OBJECTS:core_o>
        $<TARGET_OBJECTS:init_o>
        $<TARGET_OBJECTS:crypto_o>
    )

    if (ETH_FULL)
      set(IN3_LIBS ${IN3_LIBS} 
         $<TARGET_OBJECTS:tommath_o> 
         $<TARGET_OBJECTS:evm_o> 
         $<TARGET_OBJECTS:eth_full_o>
      )
    endif()

    if (ETH_BASIC) 
        set(IN3_LIBS ${IN3_LIBS} 
            $<TARGET_OBJECTS:eth_basic_o>
        )
    endif()

    if (ETH_NANO) 
        set(IN3_LIBS ${IN3_LIBS} 
            $<TARGET_OBJECTS:eth_nano_o>
        )
    endif()

    if (IPFS) 
        set(IN3_LIBS ${IN3_LIBS} 
            $<TARGET_OBJECTS:b64_o>
            $<TARGET_OBJECTS:ipfs_o>
        )
        if (IN3API) 
            set(IN3_LIBS ${IN3_LIBS} 
                $<TARGET_OBJECTS:ipfs_api_o>
            )
        endif()
    endif()

    if (IN3API) 
        set(IN3_LIBS ${IN3_LIBS} 
            $<TARGET_OBJECTS:eth_api_o>
            $<TARGET_OBJECTS:usn_api_o>
            $<TARGET_OBJECTS:api_utils_o>
        )
    endif()

    if (USE_SCRYPT)
       set(IN3_LIBS ${IN3_LIBS} $<TARGET_OBJECTS:scrypt_o>)
    endif()

    # create the libraries
    add_library(in3_bundle STATIC ${IN3_LIBS} )
    add_library(in3_lib    SHARED ${IN3_LIBS} )
    set_target_properties(in3_bundle PROPERTIES OUTPUT_NAME "in3")
    set_target_properties(in3_lib PROPERTIES OUTPUT_NAME "in3")
    target_link_libraries(in3_lib ${IN3_TRANSPORT})

    # install
    INSTALL(TARGETS in3_bundle
      DESTINATION "lib"
    )
    INSTALL(TARGETS in3_lib
        DESTINATION lib
        PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE)
    INSTALL (
        DIRECTORY ${CMAKE_SOURCE_DIR}/include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h*")

endif()
