
# static lib
add_library( in3_eth_nano STATIC 
  eth_nano.c 
  rlp.c
  merkle.c
  serialize.c
  blockheader.c
  signature.c
  txreceipt.c
)

# include core in the headers
include_directories ("${PROJECT_SOURCE_DIR}/src/core")
# add dependency
target_link_libraries(in3_eth_nano core)

# props
set_property(TARGET in3_eth_nano PROPERTY C_STANDARD 99)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")