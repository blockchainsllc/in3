
# static lib
add_library( eth_api STATIC 
  eth_api.c 
  abi.c 
  aes/aes_modes.c
  aes/aescrypt.c
  aes/aeskey.c
  aes/aestab.c
  key.c
)

# add dependency
target_link_libraries(eth_api eth_nano core)

# props
include(../core/compiler.cmake)
target_include_directories(eth_api PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")