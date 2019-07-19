
# static lib
add_library(eth_nano_o OBJECT
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

add_library(eth_nano STATIC $<TARGET_OBJECTS:eth_nano_o>)
target_link_libraries(eth_nano core)
