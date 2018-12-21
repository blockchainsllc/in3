set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/test/cmake/")

find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIR} ${CHECK_INCLUDE_DIRS})

include_directories(. ../src)

add_executable(in3_tests core.c)
target_link_libraries(in3_tests ${CHECK_LIBRARIES} core in3_eth_nano)

add_executable(runner runner.c)
target_link_libraries(runner ${CHECK_LIBRARIES} core in3_eth_nano)


add_test(test_core ${CMAKE_CURRENT_BINARY_DIR}/in3_tests)
add_test(
    NAME eth_receipt 
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/runner receipt 
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
)

#add_test(eth_tx ${CMAKE_CURRENT_BINARY_DIR}/runner receipt)

set_property(TARGET in3_tests PROPERTY C_STANDARD 99)
set_property(TARGET runner PROPERTY C_STANDARD 99)

