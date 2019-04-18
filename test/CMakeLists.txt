set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/test/cmake/")

#find_package(Check REQUIRED)
#include_directories(${CHECK_INCLUDE_DIR} ${CHECK_INCLUDE_DIRS})

include_directories(. ../src)

#add_executable(in3_tests core.c)
#target_link_libraries(in3_tests ${CHECK_LIBRARIES} core in3_eth_nano)

add_executable(runner runner.c)
target_link_libraries(runner core in3_eth_full)

add_executable(vmrunner vm_runner.c test_evm.c test_trie.c test_rlp.c)
target_link_libraries(vmrunner core in3_eth_full)


file(GLOB files "testdata/requests/*.json")
foreach(file ${files})
  get_filename_component(testname "${file}" NAME_WE)
  add_test(
    NAME              "in3_${testname}" 
    COMMAND           ${CMAKE_CURRENT_BINARY_DIR}/runner ${file} 
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
)
endforeach()

foreach(testdir 
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
#GeneralStateTests/stCallCodes
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
    list(FILTER files EXCLUDE REGEX ".*randomStatetest(150|154|159|178|184|205|248|306|48|458|467|498|554|572|636|639).json$")
    list(FILTER files EXCLUDE REGEX ".*201503110226PYTHON_DUP6.json$")
    list(FILTER files EXCLUDE REGEX ".*(ExtCodeCopyTargetRangeLongerThanCodeTests|ExtCodeCopyTests).json$")

    


    foreach(file ${files})
    get_filename_component(testname "${file}" NAME_WE)
    add_test(
        NAME              "evm/${testdir}/${testname}" 
        COMMAND           ${CMAKE_CURRENT_BINARY_DIR}/vmrunner ${file} 
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
    )
    set_tests_properties("evm/${testdir}/${testname}" PROPERTIES TIMEOUT 1) 
    endforeach()
endforeach()




set_property(TARGET runner PROPERTY C_STANDARD 99)
set_property(TARGET vmrunner PROPERTY C_STANDARD 99)
