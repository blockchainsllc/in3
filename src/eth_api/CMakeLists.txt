
# static lib
add_library( in3_eth_api STATIC 
  eth_api.c 
)


# include core in the headers
include_directories ("${PROJECT_SOURCE_DIR}/src/core" "${PROJECT_SOURCE_DIR}/src/eth_nano")
# add dependency
target_link_libraries(in3_eth_api in3_eth_nano core)

# props
include(../core/compiler.cmake)
