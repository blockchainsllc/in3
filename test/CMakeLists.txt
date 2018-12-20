set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/test/cmake/")

enable_testing()
find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIR} ${CHECK_INCLUDE_DIRS})

MESSAGE ( STATUS "Found CHECK: ${CHECK_LIBRARIES} :: ${CHECK_INCLUDE_DIR} ${LIBS}" )

set(LIBS ${LIBS} ${CHECK_LIBRARIES} core in3_eth_nano )
include_directories(. ../src)
add_executable(in3_tests core.c)
target_link_libraries(in3_tests ${CHECK_LIBRARIES} core in3_eth_nano)
add_test(in3_tests ${CMAKE_CURRENT_BINARY_DIR}/in3_tests)
set_property(TARGET in3_tests PROPERTY C_STANDARD 99)

