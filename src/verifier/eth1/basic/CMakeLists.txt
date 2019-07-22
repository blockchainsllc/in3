
# static lib
add_library(eth_basic_o OBJECT
        eth_basic.c
        eth_getTransaction.c
        eth_getBlock.c
        eth_account.c
        eth_getLog.c
        trie.c
        signer.c
        filter.c
        )

# add dependency
add_library(eth_basic STATIC $<TARGET_OBJECTS:eth_basic_o>)
target_link_libraries(eth_basic eth_nano)
