
include_directories ("${PROJECT_SOURCE_DIR}/src/core")
include_directories ("${PROJECT_SOURCE_DIR}/src/transport_curl")
include_directories ("${PROJECT_SOURCE_DIR}/src/eth_full")
include_directories ("${PROJECT_SOURCE_DIR}/src/eth_basic")
include_directories ("${PROJECT_SOURCE_DIR}/src/eth_nano")
include_directories ("${PROJECT_SOURCE_DIR}/src/eth_api")


# add the executablex
add_executable(in3 main.c)
add_executable(json data.c ../../test/util/stack_diag.c)
add_executable(rlp rlp.c)
set_property(TARGET in3 PROPERTY C_STANDARD 99)
set_property(TARGET json PROPERTY C_STANDARD 99)
set_property(TARGET rlp PROPERTY C_STANDARD 99)
target_link_libraries (in3 core in3_curl in3_eth_full in3_eth_api)
target_link_libraries (rlp core in3_eth_nano)
target_link_libraries (json core)



