
# static lib
add_library(eth_nano OBJECT
        eth_nano.c
        rlp.c
        merkle.c
        serialize.c
        blockheader.c
        signature.c
        txreceipt.c
        registry.c
        vhist.c
        )

# add dependency
target_link_libraries(eth_nano core)
