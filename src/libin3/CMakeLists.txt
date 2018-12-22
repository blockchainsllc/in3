include_directories ("${PROJECT_SOURCE_DIR}/src/core")
include_directories ("${PROJECT_SOURCE_DIR}/src/transport_curl")
include_directories ("${PROJECT_SOURCE_DIR}/src/eth_full")

# add the executablex
add_library( libin3 SHARED in3.c)
set_property(TARGET libin3 PROPERTY C_STANDARD 99)
target_link_libraries (libin3 core in3_curl in3_eth_full)



