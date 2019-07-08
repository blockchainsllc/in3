
include(../eth_basic/include.cmake)

target_sources( app PRIVATE 
../eth_full/eth_full.c 
../eth_full/evm.c
../eth_full/opcodes.c  
../eth_full/big.c
../eth_full/call.c
../eth_full/code.c
../eth_full/env.c
)

