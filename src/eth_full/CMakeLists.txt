
# static lib
add_library( in3_eth_full STATIC 
  eth_full.c 
  evm.c
  opcodes.c  
  big.c
  call.c
  code.c
  env.c
  
)

# include core in the headers
include_directories ("${PROJECT_SOURCE_DIR}/src/core")
include_directories ("${PROJECT_SOURCE_DIR}/src/eth_nano")
include_directories ("${PROJECT_SOURCE_DIR}/src/eth_basic")

# add dependency
target_link_libraries(in3_eth_full core in3_eth_basic)

# props
# props
include(../core/compiler.cmake)
