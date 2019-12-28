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
include(cmake/code-coverage.cmake)

# handle codecoverage
if (CODE_COVERAGE)
  # we exclude all headers, tests and thridparty-code
  set(coverage_excludes "EXCLUDE" "test/*" "*.h" "src/third-party/*"  "../src/third-party/*")
  add_code_coverage_all_targets(${coverage_excludes})
endif()


# first we build the runners
add_executable(runner runner.c)
target_link_libraries(runner eth_full eth_api)

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
     target_code_coverage("${testname}" "" AUTO ALL ${coverage_excludes})
     target_link_libraries("${testname}" core eth_full eth_api)
     add_test(
                NAME "in3_${testname}"
                COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${testname}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
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
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/runner ${file}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
    )
endforeach ()
target_code_coverage(runner "${files}" AUTO ALL ${coverage_excludes})

file(GLOB files 
    "testdata/evm/TrieTests/*.json" 
    "testdata/evm/RLPTests/*.json" 
    "testdata/evm/vmTests/vmArithmeticTest/*.json" 
    "testdata/evm/vmTests/vmBitwiseLocigOperation/*.json" 
#    "testdata/evm/GeneralStateTests/stRandom2/*.json"
#    "testdata/evm/GeneralStateTests/stRandom/*.json"
    "testdata/evm/GeneralStateTests/stMemoryTest/*.json"
#    "testdata/evm/GeneralStateTests/stCodeCopyTest/*.json"
#    "testdata/evm/GeneralStateTests/stRefundTest/*.json"
#    "testdata/evm/GeneralStateTests/stLogTests/*.json"
)
target_code_coverage(vmrunner "${files}" AUTO ALL ${coverage_excludes})


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
                COMMAND ${CMAKE_CURRENT_BINARY_DIR}/vmrunner ${file}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
        )
        set_tests_properties("evm/${testdir}/${testname}" PROPERTIES TIMEOUT 9)
    endforeach ()
endforeach ()


set_property(TARGET runner PROPERTY C_STANDARD 99)
set_property(TARGET vmrunner PROPERTY C_STANDARD 99)
