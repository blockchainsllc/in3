
# static lib
add_library( in3_eth_full STATIC 
  eth_full.c 
)

# include core in the headers
include_directories ("${PROJECT_SOURCE_DIR}/src/core")
include_directories ("${PROJECT_SOURCE_DIR}/src/eth_nano")
include_directories ("${PROJECT_SOURCE_DIR}/src/eth_basic")

# add dependency
target_link_libraries(in3_eth_full core in3_eth_basic)

# props
set_property(TARGET in3_eth_full PROPERTY C_STANDARD 99)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")