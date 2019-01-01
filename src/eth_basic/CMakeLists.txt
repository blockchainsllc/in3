
# static lib
add_library( in3_eth_basic STATIC 
  eth_basic.c 
  eth_getTransaction.c 
  eth_getBlock.c 
  eth_account.c
  trie.c
)


# include core in the headers
include_directories ("${PROJECT_SOURCE_DIR}/src/core" "${PROJECT_SOURCE_DIR}/src/eth_nano")
# add dependency
target_link_libraries(in3_eth_basic in3_eth_nano core)

# props
include(../core/compiler.cmake)
