
# static lib
add_library(eth_basic STATIC
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
target_link_libraries(eth_basic eth_nano core)
target_include_directories(eth_basic PUBLIC "${PROJECT_SOURCE_DIR}/src/verifier/eth1/nano")