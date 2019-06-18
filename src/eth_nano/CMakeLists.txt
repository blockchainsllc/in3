
# static lib
add_library( eth_nano STATIC 
  eth_nano.c 
  rlp.c
  merkle.c
  serialize.c
  blockheader.c
  signature.c
  txreceipt.c
  registry.c
)

# add dependency
target_link_libraries(eth_nano core)

# props
include(../core/compiler.cmake)

target_include_directories(eth_nano PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")