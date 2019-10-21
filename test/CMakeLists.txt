###############################################################################
# This file is part of the Incubed project.
# Sources: https://github.com/slockit/in3-c
# 
# Copyright (C) 2018-2019 slock.it GmbH, Blockchains LLC
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

include_directories(. ../src)
include(code-coverage.cmake)

if (CODE_COVERAGE)
  add_code_coverage_all_targets()
endif()

add_executable(runner runner.c)
target_link_libraries(runner eth_full)

add_executable(vmrunner vm_runner.c test_evm.c test_trie.c test_rlp.c)
target_code_coverage(vmrunner "" AUTO ALL)
target_link_libraries(vmrunner eth_full)

if(TRANSPORTS)
        add_executable(test_libcurl test_libcurl.c test_utils.h unity/unity.c)
        target_link_libraries(test_libcurl transport_curl eth_nano)
endif()

add_executable(test_key_hash test_key_hash.c unity/unity.c)
#target_code_coverage(test_key_hash AUTO ALL)
target_link_libraries(test_key_hash core)

add_test(
        NAME "in3_test_key_hash"
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_key_hash
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
)


add_executable(test_verify_eth_getLog test_verify_eth_getLog.c unity/unity.c)
target_code_coverage(test_verify_eth_getLog "" AUTO ALL)
target_link_libraries(test_verify_eth_getLog eth_basic)
add_test(
        NAME "test_verify_eth_getLog"
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_key_hash
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
)

add_executable(test_bitset test_bitset.c unity/unity.c)
target_code_coverage(test_bitset "" AUTO ALL)
target_link_libraries(test_bitset core)
add_test(
        NAME "in3_test_bitset"
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_bitset
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
)

if(TRANSPORTS)
        add_executable(test_vhist test_vhist.c unity/unity.c ../src/cmd/in3/in3_storage.c)
        target_link_libraries(test_vhist eth_nano transport_curl)
        add_test(
                NAME "in3_test_vhist"
                COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_bitset
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
        )
endif()

file(GLOB files "testdata/requests/*.json")
foreach (file ${files})
    get_filename_component(testname "${file}" NAME_WE)
    add_test(
            NAME "in3_${testname}"
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/runner ${file}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
    )
endforeach ()
target_code_coverage(runner "${files}" AUTO ALL)

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
        #GeneralStateTests/stZeroKnowledge2
        GeneralStateTests/stLogTests
        #GeneralStateTests/stEIP158Specific
        #GeneralStateTests/stZeroCallsRevert
        #GeneralStateTests/stTransactionTest
        #GeneralStateTests/stZeroKnowledge
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
    list(FILTER files EXCLUDE REGEX ".*randomStatetest(579|85|150|154|159|178|184|205|248|306|48|458|467|498|554|572|636|639).json$")
    list(FILTER files EXCLUDE REGEX ".*201503110226PYTHON_DUP6.json$")
    list(FILTER files EXCLUDE REGEX ".*(InInitcodeToExisContractWithVTransferNEMoney|DynamicCode|randomStatetest642|OOGE_valueTransfer|additionalGasCosts2|ExtCodeCopyTargetRangeLongerThanCodeTests|ExtCodeCopyTests).json$")


    foreach (file ${files})
        get_filename_component(testname "${file}" NAME_WE)
        add_test(
                NAME "evm/${testdir}/${testname}"
                COMMAND ${CMAKE_CURRENT_BINARY_DIR}/vmrunner ${file}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
        )
        set_tests_properties("evm/${testdir}/${testname}" PROPERTIES TIMEOUT 1)
    endforeach ()
endforeach ()


set_property(TARGET runner PROPERTY C_STANDARD 99)
set_property(TARGET vmrunner PROPERTY C_STANDARD 99)
