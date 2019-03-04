
include(../eth_nano/include.cmake)

target_sources( app PRIVATE 
../eth_basic/eth_basic.c 
../eth_basic/eth_getTransaction.c 
../eth_basic/eth_getBlock.c 
../eth_basic/eth_account.c 
../eth_basic/trie.c 
../eth_basic/signer.c 
)
