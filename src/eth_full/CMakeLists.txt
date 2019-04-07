
# static lib
add_library( eth_full STATIC 
  eth_full.c 
  evm.c
  opcodes.c  
  big.c
  call.c
  code.c
  env.c
  mem.c
  mini-gmp.c
  precompiled.c
)

# add gas-calculation
#ADD_DEFINITIONS(-DEVM_GAS)


# add dependency
target_link_libraries(eth_full core eth_basic)

# props
# props
include(../core/compiler.cmake)

target_include_directories(eth_full PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")