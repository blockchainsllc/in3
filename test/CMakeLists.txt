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
TrieTests
)
    file(GLOB files "testdata/evm/${testdir}/*.json")
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

add_test(
    NAME              "rlp" 
    COMMAND           ${CMAKE_CURRENT_BINARY_DIR}/vmrunner ../test/testdata/evm/RLPTests/rlptest.json 
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
)




set_property(TARGET runner PROPERTY C_STANDARD 99)
set_property(TARGET vmrunner PROPERTY C_STANDARD 99)
