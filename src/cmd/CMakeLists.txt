
include_directories ("${PROJECT_SOURCE_DIR}/src/core")
include_directories ("${PROJECT_SOURCE_DIR}/src/transport_curl")
include_directories ("${PROJECT_SOURCE_DIR}/src/eth_full")


# add the executablex
add_executable(in3 main.c)
add_executable(json data.c)
set_property(TARGET in3 PROPERTY C_STANDARD 99)
target_link_libraries (in3 core in3_curl in3_eth_full)
target_link_libraries (json core)



