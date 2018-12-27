
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
include(../core/compiler.cmake)
