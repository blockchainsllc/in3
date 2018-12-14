
# static lib
add_library( in3_eth_nano STATIC 
  eth_nano.c 
  rlp.c
)

# include core in the headers
include_directories ("${PROJECT_SOURCE_DIR}/src/core")

# props
set_property(TARGET in3_eth_nano PROPERTY C_STANDARD 99)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")