
# static lib
add_library( in3_eth_basic STATIC 
  eth_basic.c 
  ../eth_basic/eth_getTransaction.c 

)

# include core in the headers
include_directories ("${PROJECT_SOURCE_DIR}/src/core" "${PROJECT_SOURCE_DIR}/src/eth_nano")
# add dependency
target_link_libraries(in3_eth_basic in3_eth_nano core)

# props
set_property(TARGET in3_eth_basic PROPERTY C_STANDARD 99)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")