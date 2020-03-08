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
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test)
set_property(GLOBAL PROPERTY C_STANDARD 99)

#include("${PROJECT_SOURCE_DIR}/c/compiler.cmake")

include_directories(. ../src)

# handle codecoverage
if (CODE_COVERAGE)
  add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
  set (LDFLAGS "${LDFLAGS} -fprofile-instr-generate -fcoverage-mapping")
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-instr-generate -fcoverage-mapping")


  # we exclude all headers, tests and thridparty-code
  set(coverage_excludes "EXCLUDE" "c/test/*" "*.h" "c/src/third-party/*"  "../c/src/third-party/*")
endif()


# first we build the runners
add_executable(runner runner.c)
target_link_libraries(runner eth_full eth_api ipfs)

add_executable(junit util/junit.c)
target_link_libraries(junit eth_full)

add_executable(vmrunner vm_runner.c test_evm.c test_trie.c test_rlp.c)
target_link_libraries(vmrunner eth_full)

if(NOT TARGET tests)
  add_custom_target(tests)
  add_dependencies(tests runner vmrunner)
endif()

file(GLOB files "unit_tests/*.c")
foreach (file ${files})
     get_filename_component(testname "${file}" NAME_WE)
     add_executable("${testname}" "${file}" util/transport.c unity/unity.c)
     target_link_libraries("${testname}" core eth_full eth_api ipfs ipfs_api)
     add_test(
                NAME "in3_${testname}"
                COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${testname}
                WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/..
        )
      set_tests_properties("in3_${testname}" PROPERTIES 
        ENVIRONMENT LLVM_PROFILE_FILE=in3_${testname}.profraw
      )
      add_dependencies(tests "${testname}")

endforeach ()



if(TRANSPORTS)
        add_executable(test_libcurl test_libcurl.c test_utils.h unity/unity.c)
        target_link_libraries(test_libcurl transport_curl eth_nano)
endif()


# add evm-tests
file(GLOB files "testdata/requests/*.json")
foreach (file ${files})
    get_filename_component(testname "${file}" NAME_WE)
    add_test(
            NAME "in3_${testname}"
            COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/runner ${file}
            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/..
    )
    set_tests_properties("in3_${testname}" PROPERTIES 
      ENVIRONMENT LLVM_PROFILE_FILE=in3_${testname}.profraw
    )

endforeach ()


foreach (testdir
        vmTests/vmBitwiseLogicOperation
        vmTests/vmBlockInfoTest
        vmTests/vmEnviromentalInfo
        vmTests/vmLogTest
        vmTests/vmSha3Test
        vmTests/vmSystemOperations
        vmTests/vmTests
        vmTests/vmIOandFlowOperations
        vmTests/vmRandomTest
        vmTests/vmPushDupSwapTest
        vmTests/vmArithmeticTest
        GeneralStateTests/stRandom
        #GeneralStateTests/stRevertTest
        #GeneralStateTests/stInitCodeTest
        #GeneralStateTests/stCreateTest
        GeneralStateTests/stRandom2
        #GeneralStateTests/stWalletTest
        GeneralStateTests/stCodeCopyTest
        #GeneralStateTests/stRefundTest
        #GeneralStateTests/stRecursiveCreate
        #GeneralStateTests/stPreCompiledContracts
        #GeneralStateTests/stExtCodeHash
        #GeneralStateTests/stBugs
        GeneralStateTests/stExample
        #GeneralStateTests/stTransitionTest
        GeneralStateTests/stCallCodes
        #GeneralStateTests/stPreCompiledContracts2
        #GeneralStateTests/stZeroCallsTest
        #GeneralStateTests/stBadOpcode
        #GeneralStateTests/stMemoryStressTest
        GeneralStateTests/stShift
        #GeneralStateTests/stSpecialTest
        #GeneralStateTests/stCallCreateCallCodeTest
        #GeneralStateTests/stQuadraticComplexityTest
        #GeneralStateTests/stStackTests
        #GeneralStateTests/stChangedEIP150
        #GeneralStateTests/stSolidityTest
        GeneralStateTests/stMemoryTest
        #GeneralStateTests/stEWASMTests
        #GeneralStateTests/stNonZeroCallsTest
        #GeneralStateTests/stCodeSizeLimit
        #GeneralStateTests/stSystemOperationsTest
        #GeneralStateTests/stHomesteadSpecific
        #GeneralStateTests/stCreate2
        #GeneralStateTests/stCallDelegateCodesHomestead
        #GeneralStateTests/stSStoreTest
        #GeneralStateTests/stCallDelegateCodesCallCodeHomestead
        #GeneralStateTests/stDelegatecallTestHomestead
        #GeneralStateTests/stEIP150Specific
        #GeneralStateTests/stEIP150singleCodeGasPrices
        GeneralStateTests/stZeroKnowledge2
        GeneralStateTests/stLogTests
        #GeneralStateTests/stEIP158Specific
        #GeneralStateTests/stZeroCallsRevert
        #GeneralStateTests/stTransactionTest
        GeneralStateTests/stZeroKnowledge
        #GeneralStateTests/stStaticCall
        #GeneralStateTests/stMemExpandingEIP150Calls
        GeneralStateTests/stArgsZeroOneBalance
        #GeneralStateTests/stAttackTest
        #GeneralStateTests/stReturnDataTest
        TrieTests
        RLPTests
        )
    file(GLOB files "testdata/evm/${testdir}/*.json")

    # exclude tests, but fix them later    
    list(FILTER files EXCLUDE REGEX ".*randomStatetest(150|154|159|178|184|205|248|306|48|458|467|498|554|636|639).json$")
    list(FILTER files EXCLUDE REGEX ".*201503110226PYTHON_DUP6.json$")
    list(FILTER files EXCLUDE REGEX ".*ecpairing(.*).json$")
    list(FILTER files EXCLUDE REGEX ".*pairingTest.json$")
    list(FILTER files EXCLUDE REGEX ".*ecmul_0-3_5616_28000_96.json$")
    list(FILTER files EXCLUDE REGEX ".*(InInitcodeToExisContractWithVTransferNEMoney|DynamicCode|OOGE_valueTransfer|additionalGasCosts2|ExtCodeCopyTargetRangeLongerThanCodeTests|ExtCodeCopyTests).json$")

    list(FILTER files EXCLUDE REGEX ".*(pointMulAdd2).json$")

    foreach (file ${files})
        get_filename_component(testname "${file}" NAME_WE)
        add_test(
                NAME "evm/${testdir}/${testname}"
                COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vmrunner ${file}
                WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/..
        )
        set_tests_properties("evm/${testdir}/${testname}" PROPERTIES 
          TIMEOUT 9
          ENVIRONMENT LLVM_PROFILE_FILE=evm_${testdir}_${testname}.profraw
          )

    endforeach ()
endforeach ()

set_property(TARGET runner PROPERTY C_STANDARD 99)
set_property(TARGET vmrunner PROPERTY C_STANDARD 99)
