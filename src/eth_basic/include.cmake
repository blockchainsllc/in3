
include(../eth_nano/include.cmake)

target_sources( app PRIVATE 
../eth_basic/eth_basic.c 
../eth_basic/eth_getTransaction.c 
)
