
# static lib
add_executable( usn_device  usn_device.c )

# include core in the headers
include_directories ("${PROJECT_SOURCE_DIR}/src/core")

# add dependency
target_link_libraries(usn_device usn_api core in3_curl eth_full)

# props
include(../core/compiler.cmake)


include_directories ("${PROJECT_SOURCE_DIR}/src/core")
include_directories ("${PROJECT_SOURCE_DIR}/src/transport_curl")
include_directories ("${PROJECT_SOURCE_DIR}/src/eth_full")
include_directories ("${PROJECT_SOURCE_DIR}/src/eth_basic")
include_directories ("${PROJECT_SOURCE_DIR}/src/eth_nano")
include_directories ("${PROJECT_SOURCE_DIR}/src/eth_api")
include_directories ("${PROJECT_SOURCE_DIR}/src/usn_api")

set_property(TARGET usn_device PROPERTY C_STANDARD 99)



