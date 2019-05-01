
# static lib
add_library( eth_basic STATIC 
  eth_basic.c 
  eth_getTransaction.c 
  eth_getBlock.c 
  eth_account.c
  eth_getLog.c
  trie.c
  signer.c
)

# add dependency
target_link_libraries(eth_basic eth_nano core)

# props
include(../core/compiler.cmake)

target_include_directories(eth_basic PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")